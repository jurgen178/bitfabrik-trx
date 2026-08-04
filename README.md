# Modular HF SSB Transceiver (Superhet, 9 MHz IF, ESP32 Web Interface)

⚠️ **Work in Progress (WIP) - Active Development**

This repository contains a modular shortwave (HF) amateur radio transceiver currently under development. The project balances a classic, stable analog hardware design with modern network control. 

* **Current State:** Pre-alpha / Experimental. The prototype is initially built for the **40m band**, but engineered from scratch to scale across all HF bands from 160m to 10m.
* **Documentation:** Detailed wiring, schematics, and setup instructions will be published once the hardware architecture and software interfaces stabilize.

---

## 🔍 Key Technical Features (In Progress)

* **Architecture:** Classic Superhet design featuring a 9 MHz Intermediate Frequency (IF) with discrete IF amplification and built-in AGC.
* **Mixing System:** Fully bidirectional passive ADE-1 ring mixer utilized for both Receive (RX) and Transmit (TX) switching via relays.
* **Signal Chain:** 
  * **RX:** SA612 product detector paired with an TDA7052A audio amplifier.
  * **TX:** TL072 microphone preamp, SA612 SSB modulator, and a single-ended RD16HHF1 PA stage aiming for 15W output with custom L-matching.
* **Control & Oscillators:** Dual AD9850 DDS modules (independent LO and BFO generation) orchestrated by an ESP32.
* **Web UI Control:** Integrated standalone Wi-Fi network interface hosted on the ESP32. Enables platform-independent remote tuning, band switching, mode selection (USB/LSB/CW/RTTY), and a built-in Morse/RTTY interface which will be expanded to support common digital transfer modes.

---

## 📁 Repository Structure

* `/src` — Contains the early-stage ESP32 control software, DDS configuration routines, and the wireless web server interface code.
* *(Upcoming)* `/hardware` — Schematics, component lists, and layout guides tailored for the Manhattan-style build approach will be uploaded here later.
