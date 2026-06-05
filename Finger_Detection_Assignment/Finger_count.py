import cv2
import mediapipe as mp
import time

mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils


def count_fingers(hand_landmarks, handedness):
    """
    Return how many fingers are up (0–5).
    handedness: 'Left' or 'Right'
    """
    lm = hand_landmarks.landmark
    fingers_up = 0

    # ---- Thumb ----
    thumb_tip = lm[mp_hands.HandLandmark.THUMB_TIP]
    thumb_ip  = lm[mp_hands.HandLandmark.THUMB_IP]

    if handedness == "Right": 
        if thumb_tip.x < thumb_ip.x:
            fingers_up += 1
    else:  # Left hand
        if thumb_tip.x > thumb_ip.x:
            fingers_up += 1

    # ---- Other fingers ----
    finger_tips = [
        mp_hands.HandLandmark.INDEX_FINGER_TIP,
        mp_hands.HandLandmark.MIDDLE_FINGER_TIP,
        mp_hands.HandLandmark.RING_FINGER_TIP,
        mp_hands.HandLandmark.PINKY_TIP,
    ]
    finger_pips = [
        mp_hands.HandLandmark.INDEX_FINGER_PIP,
        mp_hands.HandLandmark.MIDDLE_FINGER_PIP,
        mp_hands.HandLandmark.RING_FINGER_PIP,
        mp_hands.HandLandmark.PINKY_PIP,
    ]

    for tip_id, pip_id in zip(finger_tips, finger_pips):
        if lm[tip_id].y < lm[pip_id].y:
            fingers_up += 1

    return fingers_up


def main():
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)


    with mp_hands.Hands(
        max_num_hands=2,
        model_complexity=1,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5
    ) as hands:

        while True:
            ret, frame = cap.read()
            if not ret:
                break

            frame = cv2.flip(frame, 1)
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = hands.process(rgb)

            if results.multi_hand_landmarks:
                for hand_landmarks, handedness in zip(
                        results.multi_hand_landmarks,
                        results.multi_handedness):

                    mp_drawing.draw_landmarks(
                        frame,
                        hand_landmarks,
                        mp_hands.HAND_CONNECTIONS
                    )

                    label = handedness.classification[0].label
                    num_fingers = count_fingers(hand_landmarks, label)

                    print(f"Hand: {label}, Fingers up: {num_fingers}")

                    cv2.putText(
                        frame,
                        f"{label}: {num_fingers}",
                        (10, 60 if label == "Right" else 120),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        1.5,
                        (0, 255, 0),
                        3
                    )

            cv2.imshow("Finger Count (0–5)", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()