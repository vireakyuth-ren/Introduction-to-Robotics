from flask import Flask, Response
import cv2
import serial
import threading
import time
from ultralytics import YOLO

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
time.sleep(2)

# =========================
# YOLO
# =========================
model = YOLO('best.pt')
print("Model classes:", model.names)
BOX_CONF_THRESHOLD = 0.35
YOLO_IMGSZ = 320            # smaller inference size = much faster on Pi CPU
YOLO_LOOP_SLEEP = 0.05      # throttle YOLO thread to ~20Hz so it stops fighting
                            # the main video loop for CPU; box detection doesn't
                            # need to run every single video frame
BOX_TURN_MAP = {
    "greenbox": "Left",
    "redbox": "Right",
}

AVOID_COMMAND_MAP = {
    "Left": "AvoidLeft",
    "Right": "AvoidRight",
}

yolo_frame = None
yolo_result_command = None
yolo_result_label = None
yolo_lock = threading.Lock()

def yolo_worker():
    global yolo_result_command, yolo_result_label
    while True:
        with yolo_lock:
            frame = yolo_frame.copy() if yolo_frame is not None else None
        if frame is None:
            time.sleep(0.01)
            continue
        results = model.predict(frame, verbose=False, conf=BOX_CONF_THRESHOLD, imgsz=YOLO_IMGSZ)
        best_command, best_label, best_conf = None, None, 0.0
        for r in results:
            for box in r.boxes:
                cls_id = int(box.cls[0])
                conf = float(box.conf[0])
                label = model.names[cls_id]
                if label in BOX_TURN_MAP and conf > best_conf:
                    best_conf = conf
                    best_label = label
                    best_command = BOX_TURN_MAP[label]
        with yolo_lock:
            yolo_result_command = best_command
            yolo_result_label = best_label
        time.sleep(YOLO_LOOP_SLEEP)

threading.Thread(target=yolo_worker, daemon=True).start()

# =========================
# Ultrasonic
# =========================
distance_lock = threading.Lock()
last_distance_cm = None
last_distance_time = 0

OBSTACLE_TRIGGER_DISTANCE = 28
MIN_TURN_TIME = 0.6        # minimum seconds to keep turning, even if the box leaves view early
AVOID_FORWARD_TIME = 0.8   # seconds to drive forward after turning past a box, before turning back
AVOID_RECOVER_TIME = 1.6   # seconds to force-drive straight after turnback, before allowing search

def serial_reader():
    global last_distance_cm, last_distance_time
    while True:
        try:
            line = ser.readline().decode(errors="ignore").strip()
            if line.startswith("DIST:"):
                try:
                    val = int(line.split(":")[1])
                    with distance_lock:
                        last_distance_cm = val if val > 0 else None
                        last_distance_time = time.time()
                except ValueError:
                    pass
        except serial.SerialException:
            break

threading.Thread(target=serial_reader, daemon=True).start()

def get_distance():
    with distance_lock:
        if last_distance_cm is None or time.time() - last_distance_time > 1.0:
            return None
        return last_distance_cm

# =========================
# State
# =========================
last_command = ""
last_decision = "Forward"
avoiding = False
avoid_direction = None
avoid_phase = None          # "turn" -> "forward" -> "turnback" -> "recover"
avoid_phase_start = 0
avoid_turn_start = 0
avoid_turn_duration = 0     # how long the outward turn actually lasted
avoid_turnback_start = 0    # when the turn-back phase started
avoid_recover_start = 0     # when the post-turnback forward recovery started
obstacle_scanning = False
obstacle_scan_start_time = 0

# FPS tracking
fps_counter = 0
fps_timer = time.time()
current_fps = 0

app = Flask(name)
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))  # MJPG usually unlocks much
                                                                # higher FPS than default YUYV
cap.set(cv2.CAP_PROP_FPS, 30)
cap.set(3, 640)
cap.set(4, 480)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

JPEG_QUALITY = 70  # lower quality = faster encode + smaller stream, little visible
                    # difference on a live low-res feed


def get_mask(roi):
    hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)
    return cv2.inRange(hsv, (0, 0, 0), (180, 255, 80))


def generate_frames():
    global last_command, last_decision, avoiding, avoid_direction
    global avoid_phase, avoid_phase_start, avoid_turn_start
    global avoid_turn_duration, avoid_turnback_start, avoid_recover_start
    global obstacle_scanning, obstacle_scan_start_time
    global yolo_frame
    global fps_counter, fps_timer, current_fps

    while True:
        # NOTE: previously did 2x cap.grab() + cap.read() (3 real frame waits
        # per loop, only 1 processed) to flush stale buffered frames. With
        # CAP_PROP_BUFFERSIZE=1 already set, a single read() should be enough
        # on most drivers. If you notice the feed reacting to obstacles late
        # (visibly stale frames), your driver may be ignoring BUFFERSIZE - add
        # back a single cap.grab() before this line as a middle ground.
        ret, frame = cap.read()
        if not ret:
            break

        h, w = frame.shape[:2]
        distance = get_distance()

        # feed YOLO thread
        with yolo_lock:
            yolo_frame = frame.copy()
            box_command = yolo_result_command
            box_label = yolo_result_label

        # =========================
        # Line detection
        # =========================
        top = int(0.55 * h)
        roi = frame[top:h, :]
        mask = get_mask(roi)
        M = cv2.moments(mask)
        line_visible = M["m00"] > 0

        if line_visible:
            cx = int(M["m10"] / M["m00"])
            cv2.circle(roi, (cx, 50), 8, (0, 0, 255), -1)
        else:
            cx = None

        decision = "FORWARD"
        command = "Forward"

        # =========================
        # Priority 1: obstacle stop and scan
        # =========================
        if not obstacle_scanning and not avoiding:
            if distance is not None and distance <= OBSTACLE_TRIGGER_DISTANCE:
                obstacle_scanning = True
                obstacle_scan_start_time = time.time()

        if obstacle_scanning:
            if box_command is not None:
                obstacle_scanning = False
                avoiding = True
                avoid_direction = box_command
                avoid_phase = "turn"
                avoid_turn_start = time.time()
                command = AVOID_COMMAND_MAP[box_command]
                decision = f"OBSTACLE -> {command}"
                last_command = ""        # force resend

            else:
                command = "Stop"
                decision = "SCANNING FOR BOX (STOPPED)"

                # if the obstacle itself is gone (distance no longer
                # close/unknown), give up scanning and return to normal
                # line following instead of staying stopped forever.
                if distance is None or distance > OBSTACLE_TRIGGER_DISTANCE:
                    obstacle_scanning = False
                    decision = "OBSTACLE CLEARED"
                  # =========================
        # Priority 2: box avoidance (turn phase)
        # Only allowed to (re)trigger while NOT already committed to the
        # "forward"/"turnback"/"recover" sub-phases of an avoidance maneuver,
        # so a flickering YOLO detection can't hijack control mid-maneuver
        # and add untracked extra rotation.
        # =========================
        elif box_command is not None and avoid_phase not in ("forward", "turnback", "recover"):
            if avoid_phase != "turn":
                avoid_turn_start = time.time()
            avoiding = True
            avoid_direction = box_command
            avoid_phase = "turn"
            command = AVOID_COMMAND_MAP[box_command]
            decision = f"AVOIDING ({command})"
            last_command = ""            # force resend

        # =========================
        # Priority 3: avoidance sequence - turn -> forward -> turnback -> recover
        # =========================
        elif avoiding:
            if avoid_phase == "turn":
                if time.time() - avoid_turn_start < MIN_TURN_TIME:
                    # box left the view early - keep turning until the minimum time is met
                    command = AVOID_COMMAND_MAP[avoid_direction]
                    decision = f"AVOIDING ({command})"
                else:
                    avoid_turn_duration = time.time() - avoid_turn_start  # remember how long we turned
                    avoid_phase = "forward"
                    avoid_phase_start = time.time()
                    command = "Forward"
                    decision = "AVOID FORWARD"
                    last_command = ""

            elif avoid_phase == "forward":
                if time.time() - avoid_phase_start > AVOID_FORWARD_TIME:
                    avoid_phase = "turnback"
                    avoid_turnback_start = time.time()
                    turnback_direction = "Left" if avoid_direction == "Right" else "Right"
                    command = AVOID_COMMAND_MAP[turnback_direction]
                    decision = f"AVOID TURNBACK ({command})"
                    last_command = ""
                else:
                    command = "Forward"
                    decision = "AVOID FORWARD"

            elif avoid_phase == "turnback":
                turnback_direction = "Left" if avoid_direction == "Right" else "Right"
                if time.time() - avoid_turnback_start < avoid_turn_duration:
                    # pure timer, mirrors the outward turn using the SAME
                    # avoid command type so the rotation angle actually matches
                    command = AVOID_COMMAND_MAP[turnback_direction]
                    decision = f"AVOID TURNBACK ({command})"
                else:
                    # matched the outward turn duration - heading should be
                    # parallel again, but DON'T hand off to line-following
                    # yet. Go into a forced-forward recovery phase first so
                    # the robot actually drives (for real time, not one
                    # frame) instead of instantly hitting "line not seen"
                    # and jumping into a search turn.
                    avoid_phase = "recover"
                    avoid_recover_start = time.time()
                    command = "Forward"
                    decision = "AVOID RECOVER (FORWARD)"
                    last_command = ""
                  else:  # avoid_phase == "recover"
                if line_visible:
                    # found the line early - done, hand off to line following
                    avoiding = False
                    avoid_phase = None
                    last_decision = "Forward"   # reset stale pre-avoidance state
                    command = "Forward"
                    decision = "AVOID RECOVER DONE - FORWARD"
                    last_command = ""
                elif time.time() - avoid_recover_start < AVOID_RECOVER_TIME:
                    command = "Forward"
                    decision = "AVOID RECOVER (FORWARD)"
                else:
                    # recovery window elapsed without seeing the line yet -
                    # hand off to Priority 4, which will search from here
                    avoiding = False
                    avoid_phase = None
                    last_decision = "Forward"   # reset stale pre-avoidance state
                    command = "Forward"
                    decision = "AVOID RECOVER TIMEOUT - FORWARD"
                    last_command = ""

        # =========================
        # Priority 4: line following
        # =========================
        elif not obstacle_scanning and not avoiding and box_command is None:
            if line_visible:
                if cx < w // 3:
                    decision, command = "LEFT", "Left"
                    last_decision = "Left"
                elif cx > 2 * w // 3:
                    decision, command = "RIGHT", "Right"
                    last_decision = "Right"
                else:
                    decision, command = "FORWARD", "Forward"
                    last_decision = "Forward"
            else:
                # lost the line - search opposite of last known direction
                if last_decision == "Right":
                    decision, command = "SEARCH LEFT", "SearchLeft"
                elif last_decision == "Left":
                    decision, command = "SEARCH RIGHT", "SearchRight"
                else:
                    decision, command = "SEARCH LEFT", "SearchLeft"

        # =========================
        # Send
        # =========================
        if command != last_command:
            ser.write((command + "\n").encode())
            print("Sent:", command, "| decision:", decision, "| dist:", distance)
            last_command = command

        # =========================
        # FPS tracking
        # =========================
        fps_counter += 1
        if time.time() - fps_timer >= 1.0:
            current_fps = fps_counter
            fps_counter = 0
            fps_timer = time.time()

        # =========================
        # Overlay
        # =========================
        cv2.rectangle(frame, (0, top), (w, h), (0, 255, 0), 2)
        cv2.line(frame, (w // 3, top), (w // 3, h), (0, 0, 255), 2)
        cv2.line(frame, (2 * w // 3, top), (2 * w // 3, h), (255, 0, 0), 2)
        cv2.putText(frame, decision, (10, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        if box_label:
            cv2.putText(frame, f"Box: {box_label}", (10, 75),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 165, 255), 2)
        if distance is not None:
            cv2.putText(frame, f"Dist: {distance}cm", (10, 105),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
        cv2.putText(frame, f"FPS: {current_fps}", (10, 135),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')


@app.route('/')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


if name == "main":
    app.run(host='0.0.0.0', port=5000)
