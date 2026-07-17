Mini Project – Autonomous Line-Following Robot

**Overview**

This is the mini project for the Introduction to Robotics course: a four-wheeled differential-drive robot that autonomously follows a black line marked on the floor from the starting point to the ending point with multiple turns in the track to test the robot's capabilities.

**How It Works**

The system is split across two boards that talk to each other over a serial link. A Raspberry Pi handles the "thinking": it grabs frames from a USB camera, runs OpenCV-based line detection, decides what the robot should do next, and streams the annotated camera feed as an MJPEG video over a small Flask server on port 5000 so the decision state can be watched live in a browser. The Pi sends the result as short text commands (Forward, Left, Right, SearchLeft, SearchRight, Stop) over USB serial at 115200 baud. An ESP32 handles the "doing": it receives those commands and drives four independently PWM-controlled DC motors through H-bridges so the Pi always has a fresh read on what is directly ahead.

Line following itself is straightforward: the bottom ~45% of each frame is thresholded in HSV to isolate the black line, and the horizontal centroid of that mask decides whether the robot goes left, right, or forward; if the line disappears, the robot searches by turning opposite its last known direction until the line reappears.

**Requirements**

On the hardware side, the project needs an ESP32 dev board wired to four DC motors through H-bridge drivers (one PWM channel and two direction pins per motor), an HC-SR04 ultrasonic distance sensor, and a Raspberry Pi with a USB camera connected to the ESP32 over USB serial. On the software side, the Pi needs Python 3 with Flask, OpenCV (`opencv-python`), `pyserial`, and Ultralytics installed, plus the included `best.pt` weights (YOLOv8n, trained for 10 epochs at 320×320 on a Roboflow greenbox/redbox dataset). The ESP32 needs the Arduino IDE with the ESP32 board package and the ESP32Servo library to flash `sketch.ino`. Finally, the robot needs a track: a continuous black line on a light-colored floor.

**Challenges**

The mini project wasn't really hard but it wasn't smooth either. We had a small problem with the robot tripping over the line or turning to fast which causes it to lose the line even on a straight path. The fix was simple, we implemented it to turn the opposite side if it loses the line after turning one side, (For example: Last command = turn right -> loses line -> turn left -> line is back in sight). We also spent some time fine-tuning the speed of the robot to get the smoothest movement which we achieved after a few days.
