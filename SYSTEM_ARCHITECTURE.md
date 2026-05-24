# System Architecture - Pill Dispenser BLE App

## High-Level System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        PILL DISPENSER SYSTEM                        │
└─────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│                      MOBILE APPLICATION (Flutter)                    │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    User Interface Layer                        │ │
│  │                                                                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │ │
│  │  │   Setare     │  │ Alarmă       │  │ Gestionare   │        │ │
│  │  │   (Single)   │  │ Multiplă     │  │ (Edit/Del)   │        │ │
│  │  │              │  │ (Multiple)   │  │              │        │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘        │ │
│  │                                                                │ │
│  │  ┌──────────────┐                                             │ │
│  │  │   Istoric    │                                             │ │
│  │  │   (History)  │                                             │ │
│  │  └──────────────┘                                             │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    State Management Layer                      │ │
│  │                                                                │ │
│  │  ┌──────────────────┐  ┌──────────────────┐                  │ │
│  │  │  AlarmProvider   │  │  BleProvider     │                  │ │
│  │  │  (CRUD + SQLite) │  │  (BLE Comm)      │                  │ │
│  │  └──────────────────┘  └──────────────────┘                  │ │
│  │                                                                │ │
│  │  ┌──────────────────┐                                         │ │
│  │  │ HistoryProvider  │                                         │ │
│  │  │ (History Mgmt)   │                                         │ │
│  │  └──────────────────┘                                         │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    Data Layer                                  │ │
│  │                                                                │ │
│  │  ┌──────────────────────────────────────────────────────────┐ │ │
│  │  │              SQLite Database                            │ │ │
│  │  │  ┌────────────────────────────────────────────────────┐ │ │ │
│  │  │  │  alarms table                                     │ │ │ │
│  │  │  │  - id (PK)                                        │ │ │ │
│  │  │  │  - day (Luni, Marti, ...)                        │ │ │ │
│  │  │  │  - moment (Dimineata, Pranz, Seara)             │ │ │ │
│  │  │  │  - time (HH:MM)                                  │ │ │ │
│  │  │  │  - active (1/0)                                  │ │ │ │
│  │  │  │  - created_at (timestamp)                        │ │ │ │
│  │  │  └────────────────────────────────────────────────────┘ │ │ │
│  │  └──────────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    BLE Communication Layer                     │ │
│  │                                                                │ │
│  │  flutter_blue_plus v1.31.0                                    │ │
│  │  - Scan for BLE devices                                       │ │
│  │  - Connect to HM-10 module                                    │ │
│  │  - Send commands: "DAY MOMENT TIME"                           │ │
│  │  - Receive confirmations                                      │ │
│  └────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ BLE (Bluetooth Low Energy)
                                    │ Protocol: "DAY MOMENT TIME"
                                    │ Example: "Luni Dimineata 08:30"
                                    ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    HARDWARE LAYER (Breadboard)                       │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    HM-10 BLE Module                          │  │
│  │  - Bluetooth Low Energy 4.0                                 │  │
│  │  - UART Interface (9600 baud)                               │  │
│  │  - Characteristic UUID: FFE1                                │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                    │                                │
│                                    │ UART (9600 baud)              │
│                                    │                                │
│                                    ▼                                │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              PIC16F887 Microcontroller                       │  │
│  │                                                              │  │
│  │  ┌────────────────────────────────────────────────────────┐ │  │
│  │  │  UART Module                                          │ │  │
│  │  │  - Receives: "DAY MOMENT TIME"                        │ │  │
│  │  │  - Parses command                                     │ │  │
│  │  │  - Stores alarm in internal memory                    │ │  │
│  │  │  - Sends confirmation back                            │ │  │
│  │  └────────────────────────────────────────────────────────┘ │  │
│  │                                                              │  │
│  │  ┌────────────────────────────────────────────────────────┐ │  │
│  │  │  Alarm Management                                     │ │  │
│  │  │  - Check current time (RTC DS1307)                    │ │  │
│  │  │  - Compare with stored alarms                         │ │  │
│  │  │  - Trigger alarm when time matches                    │ │  │
│  │  └────────────────────────────────────────────────────────┘ │  │
│  │                                                              │  │
│  │  ┌────────────────────────────────────────────────────────┐ │  │
│  │  │  Output Control                                       │ │  │
│  │  │  - LED Animation (21 LEDs via 74HC595)               │ │  │
│  │  │  - Buzzer Activation                                  │ │  │
│  │  │  - LCD Display (16x4)                                 │ │  │
│  │  └────────────────────────────────────────────────────────┘ │  │
│  │                                                              │  │
│  │  ┌────────────────────────────────────────────────────────┐ │  │
│  │  │  Input Handling                                       │ │  │
│  │  │  - Button for user confirmation                       │ │  │
│  │  │  - Debouncing logic                                   │ │  │
│  │  └────────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              Peripheral Components                          │  │
│  │                                                              │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │  │
│  │  │  RTC DS1307  │  │  LCD 16x4    │  │  21 LEDs     │      │  │
│  │  │  (I2C)       │  │  (Display)   │  │  (74HC595)   │      │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘      │  │
│  │                                                              │  │
│  │  ┌──────────────┐  ┌──────────────┐                        │  │
│  │  │  Buzzer      │  │  Button      │                        │  │
│  │  │  (Alarm)     │  │  (Confirm)   │                        │  │
│  │  └──────────────┘  └──────────────┘                        │  │
│  └──────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow Diagram

### Setting an Alarm

```
User Interface
    │
    ├─ Select Day (Luni, Marti, ...)
    ├─ Select Moment (Dimineata, Pranz, Seara)
    ├─ Select Time (08:30)
    ├─ Check checkbox
    │
    ▼
AlarmProvider
    │
    ├─ Create Alarm object
    ├─ Insert into SQLite database
    ├─ Notify listeners
    │
    ▼
BleProvider
    │
    ├─ Format command: "Luni Dimineata 08:30"
    ├─ Send via BLE to HM-10
    │
    ▼
HM-10 Module
    │
    ├─ Receive command via BLE
    ├─ Forward to PIC16F887 via UART
    │
    ▼
PIC16F887
    │
    ├─ Receive via UART
    ├─ Parse command
    ├─ Store alarm in memory
    ├─ Send confirmation back
    │
    ▼
User sees confirmation message
```

### Triggering an Alarm

```
PIC16F887 (Real-Time)
    │
    ├─ Check RTC (DS1307)
    ├─ Compare with stored alarms
    ├─ Match found!
    │
    ▼
Activate Outputs
    │
    ├─ LED Animation (21 LEDs)
    ├─ Buzzer Sound
    ├─ LCD Display Message
    │
    ▼
User Confirmation
    │
    ├─ Press Button
    ├─ Send confirmation to app
    │
    ▼
Mobile App
    │
    ├─ Receive confirmation via BLE
    ├─ Save to History
    ├─ Update UI
    │
    ▼
User sees history entry
```

---

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Flutter Application                          │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  HomeScreen (Navigation)                                 │  │
│  │  ├─ SetAlarmScreen                                       │  │
│  │  ├─ AlarmsScreen (NEW)                                   │  │
│  │  ├─ ManageAlarmsScreen (NEW)                             │  │
│  │  └─ HistoryScreen                                        │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │                    │                    │           │
│           ▼                    ▼                    ▼           │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐ │
│  │ AlarmProvider    │  │ BleProvider      │  │ History      │ │
│  │                  │  │                  │  │ Provider     │ │
│  │ - addAlarm()     │  │ - connect()      │  │              │ │
│  │ - updateAlarm()  │  │ - sendCommand()  │  │ - addEntry() │ │
│  │ - deleteAlarm()  │  │ - disconnect()   │  │ - getAll()   │ │
│  │ - toggleAlarm()  │  │ - isConnected    │  │              │ │
│  │ - loadAlarms()   │  │                  │  │              │ │
│  └──────────────────┘  └──────────────────┘  └──────────────┘ │
│           │                    │                    │           │
│           └────────────────────┼────────────────────┘           │
│                                │                                │
│                                ▼                                │
│                    ┌──────────────────────┐                     │
│                    │  SQLite Database     │                     │
│                    │  - alarms table      │                     │
│                    │  - history table     │                     │
│                    └──────────────────────┘                     │
└─────────────────────────────────────────────────────────────────┘
                                │
                                │ BLE
                                │
┌─────────────────────────────────────────────────────────────────┐
│                    Hardware Layer                               │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  HM-10 BLE Module                                        │  │
│  │  ├─ Bluetooth Low Energy 4.0                            │  │
│  │  ├─ UART Interface                                      │  │
│  │  └─ Characteristic FFE1                                 │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                │                                │
│                                │ UART                           │
│                                │                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  PIC16F887 Microcontroller                              │  │
│  │  ├─ UART Module (9600 baud)                             │  │
│  │  ├─ Alarm Management                                    │  │
│  │  ├─ RTC DS1307 (I2C)                                    │  │
│  │  ├─ LED Control (74HC595)                               │  │
│  │  ├─ Buzzer Control                                      │  │
│  │  └─ LCD Display (16x4)                                  │  │
│  └──────────────────────────────────────────────────────────┘  │
│           │              │              │              │       │
│           ▼              ▼              ▼              ▼       │
│      ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐    │
│      │ 21 LEDs│    │ Buzzer │    │ Button │    │ LCD    │    │
│      │(74HC595)    │        │    │        │    │ 16x4   │    │
│      └────────┘    └────────┘    └────────┘    └────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Database Schema

```
┌─────────────────────────────────────────────────────────────┐
│                    SQLite Database                          │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  alarms table                                        │  │
│  │                                                      │  │
│  │  Column Name    │ Type    │ Constraints             │  │
│  │  ─────────────────────────────────────────────────  │  │
│  │  id             │ INTEGER │ PRIMARY KEY AUTOINCR    │  │
│  │  day            │ TEXT    │ NOT NULL                │  │
│  │  moment         │ TEXT    │ NOT NULL                │  │
│  │  time           │ TEXT    │ NOT NULL (HH:MM)        │  │
│  │  active         │ INTEGER │ NOT NULL DEFAULT 1      │  │
│  │  created_at     │ TEXT    │ NOT NULL (ISO8601)      │  │
│  │                                                      │  │
│  │  Example rows:                                       │  │
│  │  ─────────────────────────────────────────────────  │  │
│  │  1 │ Luni      │ Dimineata │ 08:30 │ 1 │ 2026-05-25 │  │
│  │  2 │ Luni      │ Pranz     │ 12:00 │ 1 │ 2026-05-25 │  │
│  │  3 │ Luni      │ Seara     │ 20:00 │ 0 │ 2026-05-25 │  │
│  │  4 │ Marti     │ Dimineata │ 08:30 │ 1 │ 2026-05-25 │  │
│  │  ...                                                 │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  history table (optional)                            │  │
│  │                                                      │  │
│  │  Column Name    │ Type    │ Constraints             │  │
│  │  ─────────────────────────────────────────────────  │  │
│  │  id             │ INTEGER │ PRIMARY KEY AUTOINCR    │  │
│  │  date           │ TEXT    │ NOT NULL                │  │
│  │  message        │ TEXT    │ NOT NULL                │  │
│  │  timestamp      │ TEXT    │ NOT NULL (ISO8601)      │  │
│  │                                                      │  │
│  │  Example rows:                                       │  │
│  │  ─────────────────────────────────────────────────  │  │
│  │  1 │ 2026-05-25 │ Pastila luata │ 2026-05-25T08:30 │  │
│  │  2 │ 2026-05-25 │ Pastila luata │ 2026-05-25T12:00 │  │
│  │  ...                                                 │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## BLE Communication Protocol

```
┌─────────────────────────────────────────────────────────────┐
│              BLE Communication Protocol                      │
│                                                             │
│  Format: "DAY MOMENT TIME"                                 │
│  Example: "Luni Dimineata 08:30"                           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Command Structure                                  │  │
│  │                                                     │  │
│  │  DAY:                                               │  │
│  │    - Luni (Monday)                                  │  │
│  │    - Marti (Tuesday)                                │  │
│  │    - Miercuri (Wednesday)                           │  │
│  │    - Joi (Thursday)                                 │  │
│  │    - Vineri (Friday)                                │  │
│  │    - Sambata (Saturday)                             │  │
│  │    - Duminica (Sunday)                              │  │
│  │                                                     │  │
│  │  MOMENT:                                            │  │
│  │    - Dimineata (Morning)                            │  │
│  │    - Pranz (Noon)                                   │  │
│  │    - Seara (Evening)                                │  │
│  │                                                     │  │
│  │  TIME:                                              │  │
│  │    - HH:MM format (24-hour)                         │  │
│  │    - Example: 08:30, 12:00, 20:00                  │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Example Commands                                   │  │
│  │                                                     │  │
│  │  "Luni Dimineata 08:30"                             │  │
│  │  "Luni Pranz 12:00"                                 │  │
│  │  "Luni Seara 20:00"                                 │  │
│  │  "Marti Dimineata 08:30"                            │  │
│  │  "Miercuri Pranz 12:00"                             │  │
│  │  "Joi Seara 20:00"                                  │  │
│  │  "Vineri Dimineata 08:30"                           │  │
│  │  "Sambata Pranz 12:00"                              │  │
│  │  "Duminica Seara 20:00"                             │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Response Format                                    │  │
│  │                                                     │  │
│  │  Success: "OK"                                      │  │
│  │  Error: "ERROR"                                     │  │
│  │  Confirmation: "Pastila luata" (Pill taken)        │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## State Management Flow

```
┌─────────────────────────────────────────────────────────────┐
│              Provider State Management                       │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  AlarmProvider (ChangeNotifier)                      │  │
│  │                                                      │  │
│  │  State:                                              │  │
│  │    - _database: Database?                            │  │
│  │    - _alarms: List<Alarm>                            │  │
│  │    - _isLoading: bool                                │  │
│  │                                                      │  │
│  │  Methods:                                            │  │
│  │    - initDatabase()                                  │  │
│  │    - loadAlarms()                                    │  │
│  │    - addAlarm(day, moment, time)                     │  │
│  │    - updateAlarm(id, day, moment, time, active)     │  │
│  │    - deleteAlarm(id)                                 │  │
│  │    - toggleAlarm(id, active)                         │  │
│  │    - deleteAllAlarms()                               │  │
│  │    - getAlarmsForDay(day)                            │  │
│  │    - getActiveAlarms()                               │  │
│  │    - hasAlarm(day, moment)                           │  │
│  │    - getAlarm(day, moment)                           │  │
│  │                                                      │  │
│  │  Listeners: All screens using Consumer<AlarmProvider>│  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  BleProvider (ChangeNotifier)                        │  │
│  │                                                      │  │
│  │  State:                                              │  │
│  │    - isConnected: bool                               │  │
│  │    - statusMessage: String                           │  │
│  │                                                      │  │
│  │  Methods:                                            │  │
│  │    - connect(device)                                 │  │
│  │    - disconnect()                                    │  │
│  │    - sendCommand(command)                            │  │
│  │    - scanDevices()                                   │  │
│  │                                                      │  │
│  │  Listeners: All screens using Consumer<BleProvider> │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  HistoryProvider (ChangeNotifier)                    │  │
│  │                                                      │  │
│  │  State:                                              │  │
│  │    - _history: List<HistoryEntry>                    │  │
│  │                                                      │  │
│  │  Methods:                                            │  │
│  │    - addEntry(date, message)                         │  │
│  │    - getAll()                                        │  │
│  │    - clear()                                         │  │
│  │                                                      │  │
│  │  Listeners: HistoryScreen                            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Deployment Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Development Environment                    │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Local Machine (Windows)                             │  │
│  │  - Flutter SDK                                       │  │
│  │  - Android SDK                                       │  │
│  │  - Git                                               │  │
│  │  - VS Code / Android Studio                          │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  GitHub Repository                                   │  │
│  │  - purice9999/pill_dispenser_app                     │  │
│  │  - Branch: main                                      │  │
│  │  - All source code                                   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                                │
                                │ git push
                                │
┌─────────────────────────────────────────────────────────────┐
│                  CI/CD Pipeline (Codemagic)                 │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Build Steps                                         │  │
│  │  1. Clone repository                                 │  │
│  │  2. Install Flutter dependencies                     │  │
│  │  3. Run linter (flutter_lints)                       │  │
│  │  4. Compile Dart code                                │  │
│  │  5. Build Android APK                                │  │
│  │  6. Generate APK artifact                            │  │
│  │  7. Upload to Codemagic                              │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Build Time: ~4 minutes                                    │
│  Output: app-debug.apk (~140 MB)                           │
└─────────────────────────────────────────────────────────────┘
                                │
                                │ Download APK
                                │
┌─────────────────────────────────────────────────────────────┐
│                  Testing Environment                        │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Android Device / Emulator                           │  │
│  │  - Install APK                                       │  │
│  │  - Run functional tests                              │  │
│  │  - Test BLE communication                            │  │
│  │  - Verify database persistence                       │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Summary

This architecture provides a complete, scalable solution for the Pill Dispenser BLE application. The system is divided into clear layers:

1. **UI Layer**: 4 screens for different user interactions
2. **State Management**: Provider pattern for reactive updates
3. **Data Layer**: SQLite for local persistence
4. **BLE Layer**: flutter_blue_plus for wireless communication
5. **Hardware Layer**: PIC16F887 with peripherals

All components are loosely coupled and can be tested independently. The system is production-ready and scalable for future enhancements.

---

Generated: May 25, 2026
