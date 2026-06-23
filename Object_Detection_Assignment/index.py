from ultralytics import YOLO
import cv2

# --- Load your trained YOLOv8 model ---
model = YOLO("runs/detect/train-3/weights/best.pt")   # make sure best.pt is in the same folder as this script

# --- Open webcam (0 = default cam, or replace with video path or image folder) ---
cap = cv2.VideoCapture(0)   # use 0, 1, or a file path like 'video.mp4'

if not cap.isOpened():
    print("Cannot open webcam")
    exit()

print("✅ Webcam opened successfully. Press 'q' to quit.")

while True:
    ret, frame = cap.read()
    if not ret:
        print("Frame grab failed.")
        break
    frame = cv2.flip(frame, 1)
    # --- Run YOLOv8 inference on the frame ---
    results = model(frame, stream=True)

    # --- Process detections ---
    for r in results:
        boxes = r.boxes
        for box in boxes:
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            label = model.names[cls_id]

            # Get coordinates
            x1, y1, x2, y2 = map(int, box.xyxy[0])

            # Draw box and label
            color = (0, 255, 0) if label == "greenbox" else (0, 0, 255)
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            cv2.putText(frame, f"{label} {conf:.2f}", (x1, y1 - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

            print(f"Detected: {label} (conf {conf:.2f})")

    # --- Show the result ---
    cv2.imshow("YOLOv8 Detection", frame)

    # Exit on 'q'
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
