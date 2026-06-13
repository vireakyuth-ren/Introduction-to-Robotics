**Overview**

This lab involved building a robot control system using an ESP32 microcontroller, a motor driver, a servo motor, and an ultrasonic sensor. The robot is controlled wirelessly through the Dabble application via Bluetooth and is designed to operate in two distinct modes: **Manual Mode** and **Automatic Mode**. A core requirement of the system is that the robot remains completely idle on startup and does not move until the user explicitly selects a mode using the gamepad buttons. Mode selection is handled through a global variable that is continuously checked in the main loop — pressing **SELECT** activates **Manual Mode** while pressing **START** activates **Automatic Mode**, ensuring only one mode is ever active at a time.

**Explanation**

The robot first powered on in its idle state and waited for a button press to enter a mode. In Manual Mode, the buttons were used to drive the robot in all four directions, and the servo was adjusted incrementally using Square and Circle, as well as snapped to preset positions using Cross and Triangle, with the angle clamped within the 80 to 120 degree range defined in our implementation. In Automatic Mode, the robot moved forward autonomously and successfully detected obstacles using the ultrasonic sensor. When an object was detected within 30 cm, the robot stopped, rotated left by approximately 90 degrees, and continued rotating until a clear path was found before resuming forward movement.

**Flowchart explanation**

The flowchart illustrates the full system structure across both modes. It begins with the initialization of the ESP32, Dabble, motors, servo, and ultrasonic sensor, followed by an idle state that waits for either SELECT or START to be pressed. The left branch covers Manual Mode, showing the joystick input being read and mapped to directional motor commands, followed by servo command reading with its defined angle limits. The right branch covers Automatic Mode, where the robot moves forward, reads the ultrasonic distance, and checks whether an obstacle is within 30 cm. If an obstacle is detected, it rotates left roughly 90 degrees, checks the distance again, and repeats the rotation if the path is still blocked. Both branches loop back to the initialization block, keeping the system running continuously.

[Demo Video](https://youtu.be/CS64wwPohCE)
