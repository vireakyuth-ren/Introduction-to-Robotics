import cv2
import mediapipe as mp
from mediapipe.tasks.python.vision.drawing_utils import draw_landmarks
from mediapipe.tasks.python.vision.drawing_styles import get_default_hand_connections_style, get_default_hand_landmarks_style

BaseOptions = mp.tasks.BaseOptions
HandLandmarker = mp.tasks.vision.HandLandmarker
HandLandmarkerOptions = mp.tasks.vision.HandLandmarkerOptions
VisionRunningMode = mp.tasks.vision.RunningMode

FINGER_TIPS = [4, 8, 12, 16, 20]
FINGER_PIPS = [3, 6, 10, 14, 18]

HAND_CONNECTIONS = [
    (0,1),(1,2),(2,3),(3,4),
    (0,5),(5,6),(6,7),(7,8),
    (5,9),(9,10),(10,11),(11,12),
    (9,13),(13,14),(14,15),(15,16),
    (13,17),(17,18),(18,19),(19,20),
    (0,17)
]

def count_fingers(hand_landmarks, handedness):
    fingers_up = 0

    # Thumb
    if handedness == "Left":
        if hand_landmarks[4].x < hand_landmarks[3].x:
            fingers_up += 1
    else:  # Right
        if hand_landmarks[4].x > hand_landmarks[3].x:
            fingers_up += 1

    # Other 4 fingers
    for tip, pip in zip(FINGER_TIPS[1:], FINGER_PIPS[1:]):
        if hand_landmarks[tip].y < hand_landmarks[pip].y:
            fingers_up += 1

    return fingers_up


def draw_hand(frame, hand_landmarks):
    h, w, _ = frame.shape
    for start, end in HAND_CONNECTIONS:
        x0 = int(hand_landmarks[start].x * w)
        y0 = int(hand_landmarks[start].y * h)
        x1 = int(hand_landmarks[end].x * w)
        y1 = int(hand_landmarks[end].y * h)
        cv2.line(frame, (x0, y0), (x1, y1), (255, 255, 255), 2)
    for lm in hand_landmarks:
        cx = int(lm.x * w)
        cy = int(lm.y * h)
        cv2.circle(frame, (cx, cy), 5, (0, 255, 0), -1)


def main():
    import urllib.request
    import os

    model_path = "hand_landmarker.task"
    if not os.path.exists(model_path):
        print("Downloading hand_landmarker.task model...")
        urllib.request.urlretrieve(
            "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task",
            model_path
        )
        print("Download complete.")

    options = HandLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=model_path),
        running_mode=VisionRunningMode.IMAGE,
        num_hands=2,
        min_hand_detection_confidence=0.5,
        min_hand_presence_confidence=0.5,
        min_tracking_confidence=0.5
    )

    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    with HandLandmarker.create_from_options(options) as landmarker:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            frame = cv2.flip(frame, 1)
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)

            result = landmarker.detect(mp_image)    

            if result.hand_landmarks:
                for i, hand_landmarks in enumerate(result.hand_landmarks):
                    handedness = result.handedness[i][0].display_name
                    
                    # Swap display label: Left → Right, Right → Left
                    if handedness == "Left":
                        display_name = "Right"
                    else:  # Right
                        display_name = "Left"

                    draw_hand(frame, hand_landmarks)

                    num_fingers = count_fingers(hand_landmarks, handedness)
                    print(f"Hand: {handedness}, Fingers up: {num_fingers}")

                    cv2.putText(
                        frame,
                        f"{display_name}: {num_fingers}",
                        (10, 60 if handedness == "Left" else 120),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        1.5,
                        (0, 255, 0),
                        3
                    )

            cv2.imshow("Finger Count (0-5)", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()