# Hardware Schema - Pill Dispenser System

## 📐 Diagrama Conexiunilor

### Conexiuni PIC16F887 - HM-10

```
PIC16F887                          HM-10 BLE Module
┌─────────────────┐               ┌──────────────────┐
│                 │               │                  │
│  RC6 (TX) ──────┼───────────────┼─ RX              │
│  RC7 (RX) ──────┼───────────────┼─ TX              │
│  GND ───────────┼───────────────┼─ GND             │
│  5V ────────────┼─ (via 3.3V)──┼─ VCC             │
│                 │               │                  │
└─────────────────┘               └──────────────────┘
```

### Conexiuni PIC16F887 - Periferice

```
PIC16F887
┌──────────────────────────────────────────────────┐
│                                                  │
│  RB0 ──────────┬─ LED Day 1 (Monday)            │
│  RB1 ──────────┬─ LED Day 2 (Tuesday)           │
│  RB2 ──────────┬─ LED Day 3 (Wednesday)         │
│  RB3 ──────────┬─ LED Day 4 (Thursday)          │
│  RB4 ──────────┬─ LED Day 5 (Friday)            │
│  RB5 ──────────┬─ LED Day 6 (Saturday)          │
│  RB6 ──────────┬─ LED Day 7 (Sunday)            │
│  RB7 ──────────┬─ Buzzer                        │
│                │                                 │
│  GND ──────────┴─ Common Ground                 │
│                                                  │
└──────────────────────────────────────────────────┘
```

### Circuitul LED

```
PIC Output (RB0-RB6)
        │
        ├─ 220Ω Resistor
        │
        ├─ LED (Red/Green)
        │
        ├─ GND
        │
    (Repeat for each LED)
```

### Circuitul Buzzer

```
PIC Output (RB7)
        │
        ├─ 1kΩ Resistor
        │
        ├─ NPN Transistor (2N2222)
        │
        ├─ Buzzer (5V)
        │
        ├─ GND
```

### Circuitul UART (PIC to HM-10)

```
PIC16F887 (5V)              HM-10 (3.3V)
    │                           │
    ├─ RC6 (TX) ─────────────── RX
    │
    ├─ RC7 (RX) ─────────────── TX
    │
    └─ GND ─────────────────── GND
    
Note: Use voltage divider for TX line (5V to 3.3V)
      RC6 → 1kΩ → HM-10 RX
      RC6 → 2kΩ → GND
```

---

## 🔌 Pinout PIC16F887

```
        ┌─────────────────┐
    MCLR│1            40  │VSS (GND)
    RA0 │2            39  │RB7 (Buzzer)
    RA1 │3            38  │RB6 (LED Day 7)
    RA2 │4            37  │RB5 (LED Day 6)
    RA3 │5            36  │RB4 (LED Day 5)
    RA4 │6            35  │RB3 (LED Day 4)
    RA5 │7            34  │RB2 (LED Day 3)
    RE0 │8            33  │RB1 (LED Day 2)
    RE1 │9            32  │RB0 (LED Day 1)
    RE2 │10           31  │VDD (+5V)
    VDD │11           30  │VSS (GND)
    VSS │12           29  │RC7 (RX from HM-10)
    OSC1│13           28  │RC6 (TX to HM-10)
    OSC2│14           27  │RC5
    RC0 │15           26  │RC4
    RC1 │16           25  │RC3
    RC2 │17           24  │RD7
    RD0 │18           23  │RD6
    RD1 │19           22  │RD5
    RD2 │20           21  │RD4
        └─────────────────┘
```

---

## 🔋 Alimentare și Putere

### Sursa de Alimentare
```
┌─────────────────────────────────────┐
│  Baterie 5V (4x AA = 6V)            │
│  sau                                │
│  Alimentator 5V / 2A                │
└─────────────────────────────────────┘
         │
         ├─ Regulator 5V (LM7805)
         │
         ├─ Regulator 3.3V (LM1117)
         │
         ├─ PIC16F887 (5V)
         ├─ HM-10 (3.3V)
         ├─ LEDs (5V)
         └─ Buzzer (5V)
```

### Consum Energetic
```
Component           Curent      Notă
─────────────────────────────────────
PIC16F887 (Active)  ~5 mA       Normal operation
PIC16F887 (Sleep)   ~20 µA      Low power mode
HM-10 (Connected)   ~10 mA      BLE active
HM-10 (Standby)     ~1 mA       Waiting for connection
LED (On)            ~20 mA      Per LED
Buzzer (On)         ~50 mA      Active buzzing
─────────────────────────────────────
Total (Active)      ~100 mA     All components on
Total (Standby)     ~15 mA      Waiting for alarm
```

---

## 📊 Diagrama Stărilor

```
┌─────────────────────────────────────────────────────┐
│                    SISTEM INACTIV                   │
│  - PIC în sleep mode                                │
│  - HM-10 în standby                                 │
│  - Consum: ~15 mA                                   │
└────────────────────┬────────────────────────────────┘
                     │
                     │ Telefon se conectează
                     ↓
┌─────────────────────────────────────────────────────┐
│                  CONECTAT LA BLE                     │
│  - PIC activ                                        │
│  - HM-10 conectat                                   │
│  - Consum: ~50 mA                                   │
└────────────────────┬────────────────────────────────┘
                     │
                     │ Utilizator setează alarmă
                     ↓
┌─────────────────────────────────────────────────────┐
│                  ALARMĂ SETATĂ                       │
│  - PIC asteaptă ora setată                          │
│  - LED indicator aprins                             │
│  - Consum: ~30 mA                                   │
└────────────────────┬────────────────────────────────┘
                     │
                     │ Ora alarmei atinge
                     ↓
┌─────────────────────────────────────────────────────┐
│                  ALARMĂ DECLANȘATĂ                   │
│  - Buzzer activ (3 beeps)                           │
│  - LED indicator pâlpâie                            │
│  - Consum: ~100 mA                                  │
└────────────────────┬────────────────────────────────┘
                     │
                     │ Utilizator confirmă
                     ↓
┌─────────────────────────────────────────────────────┐
│                  ALARMĂ CONFIRMATĂ                   │
│  - Buzzer oprit                                     │
│  - Mesaj trimis la telefon                          │
│  - Alarmă dezactivată pentru ziua respectivă        │
└────────────────────┬────────────────────────────────┘
                     │
                     │ Revenire la stare inactivă
                     ↓
┌─────────────────────────────────────────────────────┐
│                    SISTEM INACTIV                   │
└─────────────────────────────────────────────────────┘
```

---

## 🔧 Componente Necesare

### Lista Componente (BOM - Bill of Materials)

| Componenta | Cantitate | Valoare | Notă |
|-----------|-----------|---------|------|
| PIC16F887 | 1 | - | Microcontroller |
| HM-10 | 1 | - | BLE Module |
| Cristal oscilator | 1 | 8 MHz | Pentru PIC |
| Condensator | 2 | 20 pF | Pentru cristal |
| Condensator | 2 | 100 nF | Decuplare |
| Rezistor | 1 | 10 kΩ | Pull-up MCLR |
| Rezistor | 7 | 220 Ω | Limitare curent LED |
| Rezistor | 1 | 1 kΩ | Bază transistor |
| Rezistor | 1 | 2 kΩ | Voltage divider |
| LED | 7 | - | Indicare zile |
| Buzzer | 1 | 5V | Semnal sonor |
| Transistor | 1 | 2N2222 | Driver buzzer |
| Baterie | 4 | AA | Alimentare |
| Holder baterie | 1 | - | Pentru 4x AA |
| Regulator 5V | 1 | LM7805 | Stabilizare tensiune |
| Regulator 3.3V | 1 | LM1117 | Pentru HM-10 |
| PCB | 1 | - | Circuit board |
| Conectori | - | - | Diverse |

---

## 📱 Interfață Mobilă - Fluxul Datelor

```
┌──────────────────────────────────────────────────────┐
│           APLICAȚIA MOBILĂ (Flutter)                 │
│                                                      │
│  ┌────────────────────────────────────────────────┐  │
│  │  UI Layer                                      │  │
│  │  - SetAlarmScreen                              │  │
│  │  - HistoryScreen                               │  │
│  └────────────────────────────────────────────────┘  │
│                      ↓                                │
│  ┌────────────────────────────────────────────────┐  │
│  │  State Management (Provider)                   │  │
│  │  - BleProvider                                 │  │
│  │  - HistoryProvider                             │  │
│  └────────────────────────────────────────────────┘  │
│                      ↓                                │
│  ┌────────────────────────────────────────────────┐  │
│  │  BLE Communication                             │  │
│  │  - flutter_blue_plus                           │  │
│  │  - UUID FFE0/FFE1                              │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
                      ↓ (BLE)
┌──────────────────────────────────────────────────────┐
│           MODUL HM-10 (BLE 4.0)                      │
│  - Primește comenzi text                             │
│  - Transmite confirmări                              │
└──────────────────────────────────────────────────────┘
                      ↓ (UART)
┌──────────────────────────────────────────────────────┐
│           MICROCONTROLER PIC16F887                   │
│  - Parsează comenzi                                  │
│  - Setează alarmă                                    │
│  - Declanșează alarmă                                │
│  - Controlează LED și Buzzer                         │
└──────────────────────────────────────────────────────┘
```

---

## 🧪 Testare Hardware

### Checklist Testare

- [ ] Verificare alimentare (5V și 3.3V)
- [ ] Testare UART comunicare (TX/RX)
- [ ] Testare LED-uri (fiecare LED aprins)
- [ ] Testare Buzzer (sunet clar)
- [ ] Testare conectare HM-10
- [ ] Testare trimitere comandă
- [ ] Testare primire confirmare
- [ ] Testare declanșare alarmă
- [ ] Testare consum energetic
- [ ] Testare autonomie baterie

### Procedura Testare UART

```
1. Conectează PIC la computer via USB-UART adapter
2. Deschide terminal (PuTTY, Arduino IDE, etc.)
3. Setează baud rate: 9600
4. Trimite: "Luni Dimineata 08:30\r\n"
5. Așteptă răspuns: "Alarmă setată\r\n"
6. Verifică LED-ul pentru Luni aprins
```

---

## 📈 Performanță și Specificații

### Specificații Sistem

| Parametru | Valoare |
|-----------|---------|
| Tensiune alimentare | 5V |
| Curent maxim | 150 mA |
| Autonomie baterie | 7-10 zile |
| Rază BLE | ~10 metri |
| Latență comunicare | <500 ms |
| Precizie timp | ±1 minut/zi |
| Număr alarmă/zi | 1 |
| Număr zile | 7 |
| Temperatura operare | 0-50°C |
| Umiditate | 10-90% |

---

## 🔐 Siguranță și Protecție

### Protecții Implementate

1. **Protecție supratensiune**
   - Diode Schottky pe intrări
   - Condensatori de decuplare

2. **Protecție curent**
   - Rezistori limitare curent LED
   - Transistor driver pentru buzzer

3. **Protecție termică**
   - Radiator pe regulator 5V (dacă necesar)
   - Ventilație adecvată

4. **Protecție software**
   - Validare comenzi
   - Timeout comunicare
   - Watchdog timer

---

## 📚 Referințe și Resurse

- [PIC16F887 Datasheet](https://www.microchip.com)
- [HM-10 BLE Module Documentation](https://www.jnhuamao.cn)
- [UART Communication Guide](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter)
- [Bluetooth Low Energy Specification](https://www.bluetooth.com)

---

**Data**: 24 Mai 2026  
**Status**: ✅ Complet  
**Versiune**: 1.0
