**Overview**

This project implements real-time hand tracking and finger counting using MediaPipe and OpenCV in Python. The program accesses your webcam, detects up to two hands simultaneously, and displays a live finger count on screen — no external hardware required beyond a standard camera.

**How It Works**

MediaPipe identifies 21 landmarks on each detected hand, covering fingertips, knuckles, and the wrist. Finger states are determined by comparing the vertical position of each fingertip against its corresponding PIP joint. The thumb is handled separately using horizontal position logic, with left and right hands treated differently to ensure accurate counting regardless of orientation.

**Requirements**

Python 3.10 or 3.11 (MediaPipe is not fully compatible with 3.12+)
A working webcam
Dependencies: mediapipe==0.10.11, opencv-python

**Getting Started**

Create and activate a virtual environment, install the required libraries, then run python3 Finger_count.py. Press q to quit the camera window.
Possible Extensions
This finger detection system can serve as a foundation for gesture-based robot control, touchless UI navigation, or sign language recognition — making it a practical starting point for more advanced human-computer interaction projects.

[Demo Link](https://youtu.be/sD983FxYhew)
