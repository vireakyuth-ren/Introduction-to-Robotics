from flask import Flask, Response
import cv2
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
time.sleep(2)

# =========================
# State
# =========================
last_command = ""
last_decision = "Forward"

# FPS tracking
fps_counter = 0
fps_timer = time.time()
current_fps = 0

app = Flask(__name__)
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
    global last_command, last_decision
    global fps_counter, fps_timer, current_fps

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        h, w = frame.shape[:2]

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
        # Line following
        # =========================
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
            print("Sent:", command, "| decision:", decision)
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
        cv2.putText(frame, f"FPS: {current_fps}", (10, 75),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')


@app.route('/')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000)
