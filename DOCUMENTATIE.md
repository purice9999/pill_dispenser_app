# Dozator Automat de Pastile — Documentație Tehnică

## Cuprins

1. [Descriere Generală](#1-descriere-generală)
2. [Arhitectura Sistemului](#2-arhitectura-sistemului)
3. [Componente Hardware](#3-componente-hardware)
4. [Firmware PIC16F887](#4-firmware-pic16f887)
5. [Aplicația Mobilă Flutter](#5-aplicația-mobilă-flutter)
6. [Providers — Gestiunea Stării](#6-providers--gestiunea-stării)
7. [Ecrane (UI)](#7-ecrane-ui)
8. [Baze de Date Locale](#8-baze-de-date-locale)
9. [Protocolul de Comunicare BLE](#9-protocolul-de-comunicare-ble)
10. [Fluxul Complet de Utilizare](#10-fluxul-complet-de-utilizare)
11. [Persistența Datelor](#11-persistența-datelor)
12. [Configurare Platformă](#12-configurare-platformă)
13. [Limitări](#13-limitări)

---

## 1. Descriere Generală

Sistemul reprezintă un dozator automat de pastile controlat wireless, destinat persoanelor care urmează tratamente medicale regulate. Utilizatorul setează alarme zilnice (dimineața, prânz, seara) prin intermediul aplicației mobile, iar dispozitivul fizic semnalizează vizual și auditiv momentul administrării medicamentului. Confirmarea este transmisă înapoi la telefon și înregistrată în istoricul aplicației.

**Componentele sistemului:**
- **Aplicația mobilă** — dezvoltată în Flutter, disponibilă pe Android și iOS
- **Dispozitivul hardware** — bazat pe microcontrolerul PIC16F887, cu modul BLE HM-10

---

## 2. Arhitectura Sistemului

```
┌─────────────────┐       BLE (UART transparent)        ┌──────────────────────┐
│  Telefon mobil  │ ◄──────────────────────────────►   │   Modul HM-10 (BLE)  │
│  (Flutter App)  │   "Luni Dimineata 08:30\r\n" ──►   │                      │
└─────────────────┘   ◄── "LUAT 08:31\n"               │   UART 9600 baud     │
                      ◄── "NELUAT 13:00\n"              └──────────┬───────────┘
                                                                   │ TX→RC7 / RX→RC6
                                                        ┌──────────▼───────────┐
                                                        │    PIC16F887         │
                                                        │                      │
                                                        │  DS3231 RTC (I2C)    │
                                                        │  LCD 16x2 (4-bit)    │
                                                        │  21 LED-uri          │
                                                        │  Buzzer              │
                                                        │  Buton confirmare    │
                                                        └──────────────────────┘
```

### Arhitectura aplicației Flutter (MVVM cu Provider)

```
┌──────────────────────────────────────────────────────────┐
│                       UI (Views)                         │
│  HomeScreen · SetAlarmScreen · AlarmsScreen              │
│  ManageAlarmsScreen · BleConnectionScreen · HistoryScreen│
└──────────────────┬───────────────────────────────────────┘
                   │  Consumer / context.read
┌──────────────────▼───────────────────────────────────────┐
│                Providers (ViewModels)                    │
│   BleProvider  │  AlarmProvider  │  HistoryProvider      │
│       ↑                                   ↑             │
│       └─────── _AppBridge (callback) ─────┘             │
└──────────────┬──────────────────┬───────────────────────┘
               │                  │
       ┌───────▼──────┐   ┌───────▼──────┐
       │ flutter_     │   │  SQLite DB   │
       │ blue_plus    │   │  (sqflite)   │
       └──────────────┘   └──────────────┘
```

---

## 3. Componente Hardware

| Componentă | Rol |
|------------|-----|
| PIC16F887 | Microcontroler principal (8 MHz, oscilator extern HS) |
| HM-10 | Modul Bluetooth Low Energy (UART transparent, 9600 baud) |
| DS3231 | Ceas în timp real (RTC) cu comunicare I2C |
| LCD 16×2 | Afișaj cu interfață 4-bit pe PORTB |
| 21 LED-uri | 7 zile × 3 momente (Dimineață / Prânz / Seară) |
| 74HC595 | Registru de deplasare pentru LED-urile Sâmbătă / Duminică |
| Buzzer | Semnalizare sonoră la declanșarea alarmei |
| Buton (RA0) | Confirmare administrare pastilă |

### Distribuția LED-urilor pe porturi

| Zi | Dimineață | Prânz | Seară |
|----|-----------|-------|-------|
| Luni | RA1 | RA2 | RA3 |
| Marți | RA4 | RA5 | RE0 |
| Miercuri | RE1 | RE2 | RD0 |
| Joi | RD1 | RD2 | RD3 |
| Vineri | RD4 | RD5 | RD6 |
| Sâmbătă | 74HC595 Q0 | Q1 | Q2 |
| Duminică | 74HC595 Q3 | Q4 | Q5 |

### Conexiuni HM-10

| Pin HM-10 | Conectat la |
|-----------|-------------|
| VCC | 3.3V (regulator AMS1117) |
| GND | Masă comună |
| TX | RC7 (RX PIC) — direct |
| RX | RC6 (TX PIC) — prin divizor de tensiune 1kΩ/2kΩ (5V → 3.3V) |

---

## 4. Firmware PIC16F887

### 4.1 Configurație

- Oscilator: HS extern 8 MHz
- Watchdog Timer: dezactivat
- MCLR: activat
- Low Voltage Programming: dezactivat

### 4.2 Recepție UART prin întrerupere

Firmware-ul utilizează un **buffer circular de 64 bytes** populat prin întrerupere (ISR) la fiecare byte recepționat. Această abordare elimină pierderea datelor cauzată de depășirea FIFO-ului hardware (2 bytes) când bucla principală este ocupată cu actualizarea LCD-ului sau citirea RTC.

```
HM-10 UART TX → ISR (la fiecare byte) → circBuf[64] → UART_ReadLine() → ParseCommand()
```

### 4.3 Parsarea comenzilor

`ParseCommand()` procesează șirul primit și extrage:
1. Ziua săptămânii → index 0–6 (Luni=0, Duminică=6)
2. Momentul zilei → index 0–2 (Dimineața=0, Prânz=1, Seară=2)
3. Ora și minutul

Alarma este salvată la poziția `index = zi × 3 + moment` în tabloul de 21 alarme.

### 4.4 Ceasul în timp real DS3231

DS3231 comunică prin I2C (100 kHz, RC3=SCL, RC4=SDA). Registrul 3 (ziua săptămânii) folosește convenția 1=Duminică. Conversia la convenția europeană (0=Luni) se realizează prin:

```c
dow = (dow + 5) % 7;
```

### 4.5 Stările LCD

| Stare | Rând 0 | Rând 1 | Durată |
|-------|--------|--------|--------|
| Normal | `Luni   26/05   ` | `    14:20:04   ` | Permanent |
| Alarmă setată | `Alarma setata:  ` | `Luni Dim 08:30 ` | 3 secunde |
| Alarmă activă | `!! IA PASTILA !!` | `Luni Dim 08:30 ` | Până la confirmare |
| Confirmat | `Luni Dim 08:30 ` | `   Confirmat!  ` | 3 secunde |

### 4.6 Declanșarea alarmei

La fiecare secundă 0 a minutului, firmware-ul verifică toate cele 21 alarme. O alarmă se declanșează dacă:
- Ziua săptămânii curentă coincide cu ziua alarmei
- Ora și minutul curent coincid cu alarma
- Alarma nu a fost deja declanșată în acest minut (protecție re-declanșare)

La declanșare: buzzerul pornește și LED-ul clipește cu perioadă de ~500 ms.

### 4.7 Confirmarea administrării

La apăsarea butonului (RA0):
1. Buzzerul se oprește imediat
2. LED-ul rămâne **aprins 30 de minute**, apoi se stinge automat
3. LCD afișează mesajul de confirmare 3 secunde
4. PIC trimite `LUAT HH:MM\n` către aplicație prin BLE

Dacă alarma expiră fără confirmare (utilizatorul nu apasă butonul), PIC trimite `NELUAT HH:MM\n`.

---

## 5. Aplicația Mobilă Flutter

### 5.1 Tehnologii utilizate

| Pachet | Versiune | Rol |
|--------|----------|-----|
| `flutter` | SDK | Framework UI cross-platform |
| `flutter_blue_plus` | ^1.31.0 | Comunicare Bluetooth Low Energy |
| `sqflite` | ^2.3.0 | Baze de date SQLite locale |
| `path` | ^1.8.3 | Manipulare căi fișiere |
| `provider` | ^6.0.0 | Gestiunea stării (MVVM) |

**Platforme:** Android (API 21+), iOS (13.0+)

### 5.2 Structura fișierelor

```
lib/
├── main.dart                      # Entry point, inițializare provideri, AppBridge
├── models/
│   ├── alarm.dart                 # Model Alarm (zi, moment, oră, activ)
│   └── history_entry.dart         # Model HistoryEntry (mesaj, timestamp)
├── providers/
│   ├── ble_provider.dart          # Gestiunea conexiunii BLE și a comunicării
│   ├── alarm_provider.dart        # CRUD alarme + SQLite
│   └── history_provider.dart      # CRUD istoric + SQLite
└── screens/
    ├── home_screen.dart           # Navigare principală (BottomNavigationBar)
    ├── set_alarm_screen.dart      # Setare alarmă individuală
    ├── alarms_screen.dart         # Setare alarme multiple (grid zi × moment)
    ├── manage_alarms_screen.dart  # Lista alarmelor salvate (editare, ștergere)
    ├── ble_connection_screen.dart # Scan și conectare BLE
    └── history_screen.dart        # Istoricul pastilelor luate/neluate
```

### 5.3 Inițializarea aplicației (main.dart)

```dart
main()
  → WidgetsFlutterBinding.ensureInitialized()
  → runApp(PillDispenserApp)
      → MultiProvider
          ├── BleProvider()                    // gestionează BLE
          ├── HistoryProvider()                // deschide history.db
          └── AlarmProvider()..initDatabase()  // deschide alarms.db
      → MaterialApp
          → _AppBridge
              → initState: ble.onPillEvent = (msg) => history.addEntry(msg)
              → HomeScreen
```

`_AppBridge` este un `StatefulWidget` care conectează `BleProvider` cu `HistoryProvider` fără a crea o dependență directă între ele: când `BleProvider` detectează un mesaj `LUAT`/`NELUAT` de la dispozitiv, apelează callback-ul care adaugă automat intrarea în istoricul perseverent.

---

## 6. Providers — Gestiunea Stării

### BleProvider — `lib/providers/ble_provider.dart`

Gestionează tot ciclul de viață al conexiunii BLE.

| Proprietate / Metodă | Descriere |
|----------------------|-----------|
| `isConnected` | `true` dacă dispozitivul este conectat |
| `isScanning` | `true` în timpul scanării BLE |
| `statusMessage` | Mesaj de status afișat în UI |
| `scanResults` | Lista dispozitivelor BLE găsite |
| `onPillEvent` | Callback setat din `_AppBridge` → `HistoryProvider.addEntry` |
| `connectToKnownDevice()` | Conectare directă la UUID-ul cunoscut al HM-10 |
| `startScan()` / `stopScan()` | Pornire / oprire scan BLE (timeout 10 secunde) |
| `connectToDevice(device)` | Conectare la un dispozitiv ales din listă |
| `disconnectDevice()` | Deconectare curată (anulează toate subscripțiile) |
| `sendCommand(command)` | Trimite string la PIC cu chunking automat MTU 20 bytes |

**Chunking MTU:**

```dart
final bytes = command.codeUnits;
for (int i = 0; i < bytes.length; i += 20) {
  final chunk = bytes.sublist(i, min(i + 20, bytes.length));
  await characteristic.write(chunk, withoutResponse: false);
}
```

**Asamblarea mesajelor fragmentate la recepție:**

Pachetele BLE primite pot fi fragmentate. `BleProvider` folosește un `StringBuffer` intern care acumulează fragmentele și procesează mesajele abia când întâlnește caracterul `\n`:

```
Fragment 1: "NELU"
Fragment 2: "AT 08:30\n"
  → mesaj complet: "NELUAT 08:30" → detectat, logat în istoric
```

### AlarmProvider — `lib/providers/alarm_provider.dart`

Gestionează persistența alarmelor în SQLite.

| Metodă | Descriere |
|--------|-----------|
| `addAlarm(day, moment, time)` | Inserează alarmă nouă în DB |
| `updateAlarm(id, day, moment, time, active)` | Actualizează alarmă existentă |
| `deleteAlarm(id)` | Șterge o alarmă din DB |
| `toggleAlarm(id, active)` | Activează / dezactivează fără ștergere |
| `deleteAllAlarms()` | Șterge toate alarmele |
| `getAlarmsForDay(day)` | Filtrează alarmele după zi |
| `getActiveAlarms()` | Returnează doar alarmele active |
| `hasAlarm(day, moment)` | Verifică existența unui slot |

**Constantele statice (două variante — ASCII pentru BLE, Unicode pentru UI):**

| Tip | ASCII (BLE / DB) | Unicode (UI) |
|-----|-------------------|--------------|
| Zile | `Luni Marti Miercuri Joi Vineri Sambata Duminica` | `Luni Marți Miercuri Joi Vineri Sâmbătă Duminică` |
| Momente | `Dimineata Pranz Seara` | `Dimineață Prânz Seară` |

> PIC-ul nu procesează corect caracterele Unicode. Comenzile BLE și datele din DB folosesc ASCII pur; UI-ul afișează forma cu diacritice.

### HistoryProvider — `lib/providers/history_provider.dart`

Gestionează istoricul evenimentelor de pastile în SQLite.

| Metodă | Descriere |
|--------|-----------|
| `addEntry(message)` | Adaugă intrare nouă cu timestamp curent |
| `deleteEntry(id)` | Șterge o intrare specifică |
| `clearHistory()` | Șterge tot istoricul |

Intrările sunt populate **exclusiv** din răspunsurile dispozitivului hardware (`LUAT`/`NELUAT`). Trimiterea unei alarme **nu** generează intrare în istoric.

---

## 7. Ecrane (UI)

### HomeScreen

Ecranul principal cu `BottomNavigationBar` cu 4 tab-uri:

| Index | Iconiță | Label | Ecran |
|-------|---------|-------|-------|
| 0 | `alarm` | Setare | `SetAlarmScreen` |
| 1 | `alarm_add` | Alarmă Multiplă | `AlarmsScreen` |
| 2 | `edit` | Gestionare | `ManageAlarmsScreen` |
| 3 | `history` | Istoric | `HistoryScreen` |

Pe tab-ul **Gestionare** apare un `FloatingActionButton` roșu pentru ștergerea tuturor alarmelor.

---

### SetAlarmScreen

Setare alarmă individuală:

1. `DropdownButton` — selecție zi (Luni–Duminică)
2. `DropdownButton` — selecție moment (Dimineață / Prânz / Seară)
3. `showTimePicker` — selecție oră exactă
4. Buton **Conectare BLE** → deschide `BleConnectionScreen`
5. Buton **Trimite la Dispozitiv** → construiește și trimite comanda

**Comanda generată:**
```
"Joi Seara 21:00\r\n"
```
Format: `<ZI_ASCII> <MOMENT_ASCII> <HH:MM>\r\n`

---

### AlarmsScreen

Setare batch — mai multe alarme simultan:

- Grid cu toate combinațiile Zi × Moment (7 × 3 = 21 sloturi)
- Fiecare slot: `Checkbox` (bifat = trimis) + time picker
- **Trimite Alarmele** → iterează prin sloturile bifate, salvează în DB și trimite BLE pentru fiecare

```dart
for (String day in AlarmProvider.days) {
  for (String moment in AlarmProvider.moments) {
    if (_selectedAlarms[day]![moment]!) {
      await alarmProvider.addAlarm(day, moment, time);
      await bleProvider.sendCommand('$day $moment $time');
    }
  }
}
```

---

### ManageAlarmsScreen

Lista alarmelor salvate în DB:

- **Checkbox** per alarmă → `toggleAlarm()` (activează / dezactivează)
- Alarme inactive → text tăiat (`TextDecoration.lineThrough`), culoare gri
- **Popup menu** → Editare sau Ștergere
- Dialog editare → `DropdownButton` zi/moment + `showTimePicker`

---

### BleConnectionScreen

Gestionarea conexiunii Bluetooth:

1. La deschidere (după 500 ms delay) → încearcă conectare directă la UUID-ul cunoscut
2. Dacă eșuează → pornește automat scanarea BLE (10 secunde)
3. Lista dispozitivelor: HMSoft apare primul (bordură albastră + badge "HM-10"), restul sortate descrescător după RSSI
4. La conectare reușită → `SnackBar` verde + închidere automată după 1 secundă
5. La eșec → `SnackBar` roșu + reluare scan

---

### HistoryScreen

Istoricul evenimentelor înregistrate **automat** de la dispozitiv:

| Tip eveniment | Aspect card | Iconiță |
|---------------|-------------|---------|
| Pastilă **luată** | Fundal verde deschis | `check_circle` verde |
| Pastilă **neluată** | Fundal roșu deschis | `cancel` roșu |

Fiecare card afișează:
- **Titlu:** mesajul evenimentului (ex: *"Pastilă luată la 08:30"*)
- **Subtitlu:** data și ora înregistrării (ex: *"03/06/2026 08:31"*)
- **Buton ștergere** individuală

Butonul **Resetare Istoric** șterge toate intrările (cu confirmare dialog).

> Istoricul nu mai conține intrări de tip „alarmă trimisă". Se înregistrează exclusiv confirmările primite de la hardware.

---

## 8. Baze de Date Locale

### alarms.db

```sql
CREATE TABLE alarms (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  day        TEXT    NOT NULL,   -- ex: "Luni"
  moment     TEXT    NOT NULL,   -- ex: "Dimineata"
  time       TEXT    NOT NULL,   -- ex: "08:30"
  active     INTEGER NOT NULL DEFAULT 1,  -- 1=activ, 0=inactiv
  created_at TEXT    NOT NULL    -- ISO 8601 timestamp
);
```

### history.db

```sql
CREATE TABLE history (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  message    TEXT     NOT NULL,  -- ex: "Pastilă luată la 08:30"
  timestamp  TEXT     NOT NULL,  -- ISO 8601 timestamp
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

**Locație pe dispozitiv:**
- Android: `/data/data/com.example.pill_dispenser/databases/`
- iOS: directorul `Documents/` al aplicației

---

## 9. Protocolul de Comunicare BLE

**Serviciu HM-10:**
- Service UUID: `FFE0`
- Characteristic UUID: `FFE1` (Read / Write / Notify)
- Device UUID hardcodat: `9CDA8749-2042-78B8-6DF3-35A71DF625B1`

### Comenzi trimise de aplicație → PIC

```
FORMAT:   <ZI> <MOMENT> <HH:MM>\r\n

EXEMPLE:
  "Luni Dimineata 08:30\r\n"
  "Joi Pranz 13:00\r\n"
  "Duminica Seara 21:15\r\n"
  "TEST\r\n"                      ← test hardware (buzzer + LED + LCD)
```

- Doar caractere ASCII (fără diacritice)
- Terminat cu `\r\n` (detectat de firmware ca end-of-command)
- Pachete BLE max 20 bytes — fragmentare automată în aplicație

### Răspunsuri primite de la PIC → aplicație

```
FORMAT RECOMANDAT:  <STARE> <HH:MM>\n

EXEMPLE:
  "LUAT 08:30\n"       ← utilizatorul a confirmat administrarea
  "NELUAT 13:00\n"     ← alarma a expirat fără confirmare
  "OK\r\n"             ← alarmă salvată cu succes
  "TEST OK\r\n"        ← confirmare test hardware
```

**Detecție în aplicație:**

| Mesaj conține | Intrare creată în istoric |
|---------------|--------------------------|
| `LUAT` (cu oră) | *"Pastilă luată la HH:MM"* — card verde |
| `LUAT` (fără oră) | *"Pastilă luată"* — card verde |
| `NELUAT` (cu oră) | *"Pastilă neluată la HH:MM"* — card roșu |
| `NELUAT` (fără oră) | *"Pastilă neluată"* — card roșu |
| Altceva (`OK`, `TEST OK`) | Ignorat (nu se loghează) |

Detecția este case-insensitive și funcționează chiar dacă mesajul ajunge fragmentat în mai multe pachete BLE.

---

## 10. Fluxul Complet de Utilizare

```
1. Utilizatorul pornește dispozitivul (PIC alimentat)
   └─► LCD afișează ora și data curentă

2. Deschide aplicația pe telefon
   └─► Conectare automată BLE la HMSoft (UUID cunoscut)
   └─► Dacă nu se găsește → scan 10 secunde → selectare din listă

3. Setează alarma: Marți / Prânz / 14:30 → "Trimite"
   └─► Aplicația trimite: "Marti Pranz 14:30\r\n"
   └─► PIC răspunde: "OK\r\n"
   └─► LCD: "Alarma setata: / Mar. Prz 14:30" (3 sec)
   └─► Alarma salvată în RAM PIC + în alarms.db pe telefon

4. La ora 14:30:00 (Marți) — fără conexiune BLE necesară
   └─► Buzzer pornit + LED clipește
   └─► LCD: "!! IA PASTILA !! / Mar. Prz 14:30"

5a. Utilizatorul apasă butonul de confirmare
   └─► Buzzer oprit
   └─► LED aprins 30 de minute
   └─► LCD: "Mar. Prz 14:30 / Confirmat!" (3 sec)
   └─► PIC trimite: "LUAT 14:30\n" → aplicație
   └─► HistoryScreen: card verde "Pastilă luată la 14:30"

5b. Alarma expiră fără confirmare
   └─► PIC trimite: "NELUAT 14:30\n" → aplicație
   └─► HistoryScreen: card roșu "Pastilă neluată la 14:30"
```

---

## 11. Persistența Datelor

| Date | Unde se salvează | Persistă după oprire? |
|------|-----------------|----------------------|
| Alarme setate (din app) | SQLite `alarms.db` pe telefon | Da |
| Alarme pe PIC | RAM (SRAM) | **Nu** — se pierd la oprire |
| Istoric pastile | SQLite `history.db` pe telefon | Da |

> La fiecare repornire a dispozitivului, alarmele trebuie retrimise din aplicație.

---

## 12. Configurare Platformă

### Android — `android/app/src/main/AndroidManifest.xml`

```xml
<!-- BLE clasic (API < 31) -->
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />

<!-- BLE modern (Android 12+ / API 31+) -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />

<!-- Necesare pentru scan BLE pe Android 6–11 -->
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
```

**Target SDK:** 36 | **Min SDK:** 21 (Android 5.0+)

### iOS — `ios/Runner/Info.plist`

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>Aplicația folosește Bluetooth pentru a comunica cu dozatorul de pastile HM-10.</string>

<key>NSBluetoothPeripheralUsageDescription</key>
<string>Aplicația folosește Bluetooth pentru a comunica cu dozatorul de pastile HM-10.</string>
```

**Deployment target:** iOS 13.0+ | **Swift:** 5.0

Build iOS necesită un Mac cu Xcode. Build Android este configurat în `codemagic.yaml`.

---

## 13. Limitări

- Certificatul de dezvoltator iOS (cont gratuit Apple) expiră la **7 zile** și necesită reinstalare prin Sideloadly.
- Raza de acțiune BLE: ~10 metri în spații deschise.
- Maximum **21 alarme** simultane (7 zile × 3 momente).
- Alarmele din PIC **nu persistă** la oprirea dispozitivului — trebuie retrimise.
- O singură alarmă activă pe slot (zi + moment) la un moment dat.
