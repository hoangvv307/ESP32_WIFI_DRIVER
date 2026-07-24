# ESP32 Wifi Driver Project

A custom WiFi driver implementation and configuration built on top of ESP-IDF for the ESP32 platform.

<img width="1672" height="941" alt="ChatGPT Image Jul 23, 2026, 11_10_01 PM" src="https://github.com/user-attachments/assets/a68e55b1-54e8-4107-8155-0512d0d3b956" />
credit: Chat-GPT.

## Table of contents
- [Overall](#overall)
- [Components](#components)
- [Hardware Used](#hareware_used)
- [Software](#software)
- [Mistake & fix](#mistake--fix)

# Overall

This project implements a WiFi connectivity module on the ESP32 using the ESP-IDF framework. It handles station mode connection, event network state management and IP process. Additionally, this project delves into low-level microcontroller concepts, using internal API of ESP manufacturer to handle every signal or event that occurs. 

By the way, to help me build this cool feature I need to reference Shawn Hymel's Youtube channel, which helped me a lot during implementation. I strongly recommend visiting his channel, trust me it's worth checking.

To begin with, Wifi [DRIVER](DRIVER.md) help with processing number of task event, such as scanning available networks, establishing a connection, handling disconnection/reconnection, and forwarding data between the application layer and the underlying hardware. The driver also works closely with the ESP-IDF event loop, which notifies the application whenever a change in WiFi or IP state happens — for example, when the device successfully connects, loses connection, or receives a new IP address.

Beyond just connecting to WiFi, this project aims to give a clearer picture of what actually happens under the hood: how the ESP32 negotiates with an access point, how packets are queued and processed, and how the driver abstracts away the complexity of the 802.11 protocol so the application code stays simple and readable.

By working through this project, the goal is not only to get a stable WiFi connection working, but also to build a solid understanding of embedded networking concepts that can be reused in future projects — whether that's IoT devices, sensor networks, or anything else that needs reliable wireless communication.

## Components 

** THIS IS MENUCONFIG OF WIFI STA 
<img width="1857" height="1092" alt="Screenshot 2026-07-21 231757" src="https://github.com/user-attachments/assets/f1bc48c7-62ba-4b0f-81e8-9b6386c8bc15" />
<img width="1459" height="929" alt="Screenshot 2026-07-21 231845" src="https://github.com/user-attachments/assets/0195d3f3-8331-434f-a1d8-a82303a83815" />
** THIS IS THE LOGGING OF WIFI
<img width="1919" height="1115" alt="Screenshot 2026-07-21 230744" src="https://github.com/user-attachments/assets/e053d985-d3ee-438d-b3b5-4aca12047d5b" />
<img width="1919" height="1060" alt="Screenshot 2026-07-21 230720" src="https://github.com/user-attachments/assets/60eb6192-06aa-4392-9026-4eeda2b168cb" />
** THIS IS THE FIGURE ILLUSTATES ESP32 
<img width="2560" height="1152" alt="1784650638577_215874031936542501_587270705343841789_616dd036ac320258f05919f5e0d2c8c3" src="https://github.com/user-attachments/assets/7bd78685-8ab4-477a-8dd4-b43edd7764ed" />




