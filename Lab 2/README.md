**Overview**

This project implements a 4-wheel robot control system using an ESP32 microcontroller and an IR remote controller. The robot decodes IR signals in real time and maps each button to a specific action — arrow keys drive the robot forward, backward, left, and right, while the OK button brings it to a full stop. Speed is managed through a flexible control system: the * and # buttons decrease or increase speed in steps of 5, and numeric buttons (1–9 followed by 0 to confirm) allow direct speed entry. All speed values are clamped within a 0–100 range, with a default starting speed of 50.

**Flowchart Explanation**

The program begins by initializing the IR receiver, motor driver, PWM channels, and setting the default speed. It then enters a continuous loop where it reads incoming IR input and identifies the button type. If a motion button is pressed (UP, DOWN, LEFT, RIGHT, or OK), the robot immediately executes the corresponding movement. If a speed-related button is pressed instead, the system either adjusts the speed incrementally or accumulates numeric digits until 0 is pressed to confirm the new value. In both branches, the speed is clamped to the valid range before being applied to the motors via PWM. The loop then repeats, keeping the robot continuously responsive to new remote inputs.

[Demo video link](https://youtube.com/shorts/DTZSaQiR0h0?feature=share)
