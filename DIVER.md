# 1. DRIVER IN PROJECT

First, we need to know what is driver and what it can do to our project.

Driver such as a translator between hardware and operating system. Without driver, your OS or specific your chip Esp32 would not know how to "talk to" a part of hardware.

I will demonstrate to you throughout this diagram.

<img width="856" height="733" alt="Screenshot 2026-07-23 223459" src="https://github.com/user-attachments/assets/8a1b2a68-0033-41e4-92cf-47a333fcf8d8" />

The source of the figure is from Artful Bytes, which is a channel on youtube, he is an Embedded engineer, posting many useful video relevant to Embedded topic. I highly recommend you to watch this channel it's greatly increased knowledge.

As you can see, in his project of SUMOBOT show us where driver is located between the application which is our code connect to the components via the protocols, all of this thing is carefully pack in to microcontroller.

Regarding, my work is primary to build such as this driver for wifi feature, having recently Integrated in microcontroller ESP32-DEVKIT-V1.

