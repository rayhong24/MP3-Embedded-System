# 43proj-mp

## Disclaimer
This is a copy of the repository used during development. The original repository is private to preserve academic integrity for future iterations of the embedded systems course.

## Description
This project implements an embedded MP3 player on the BeagleY-AI single-board computer.

It was developed using custom C modules to interface with various hardware components through GPIO, I²C, and PWM signals, including the joystick, rotary encoder, and LED display.
The system uses multithreading (pthreads) to manage concurrent tasks such as input handling, audio playback, and UI updates, ensuring smooth and responsive performance.
Audio playback is handled using the ALSA (Advanced Linux Sound Architecture) library, which decodes and plays MP3 files downloaded or stored locally on the device.

A short demonstration of one of the intermediate development checkpoints can be found here:

🔗 https://youtu.be/uZKQCKCmj3E
