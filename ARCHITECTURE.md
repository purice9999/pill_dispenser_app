# 🏗️ Arhitectura Aplicației

## 📊 Diagrama Componentelor

```
┌─────────────────────────────────────────────────────────────────┐
│                    FLUTTER APPLICATION                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    UI LAYER (Screens)                    │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • HomeScreen (Navigare)                                 │   │
│  │  • SetAlarmScreen (Setare alarme)                        │   │
│  │  • HistoryScreen (Vizualizare istoric)                   │   │
│  │  • BleConnectionScreen (Conectare BLE)                   │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              STATE MANAGEMENT (Providers)                │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • BleProvider (Gestionare BLE)                          │   │
│  │  • HistoryProvider (Gestionare istoric)                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                  BUSINESS LOGIC                          │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • Parsare comenzi                                       │   │
│  │  • Validare date                                         │   │
│  │  • Formatare mesaje                                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                  DATA LAYER                              │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • SQLite Database (Istoric)                             │   │
│  │  • BLE Communication (flutter_blue_plus)                 │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                    HARDWARE LAYER                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              HM-10 BLE MODULE                            │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • Service: FFE0                                         │   │
│  │  • Characteristic: FFE1 (Read/Write/Notify)             │   │
│  │  • Baud Rate: 9600                                       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │           MICROCONTROLLER (Arduino/PIC)                 │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • UART Communication                                    │   │
│  │  • Command Processing                                    │   │
│  │  • Motor/Solenoid Control                                │   │
│  └──────────────────────────────────────────────────────────┘   │
│                           ↓                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              PILL DISPENSER HARDWARE                     │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │  • Motor/Solenoid                                        │   │
│  │  • Mechanical Mechanism                                  │   │
│  │  • Sensors (Optional)                                    │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Fluxul Datelor - Trimitere Comandă

```
┌─────────────────────────────────────────────────────────────────┐
│ 1. UTILIZATOR SELECTEAZĂ PARAMETRI                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  SetAlarmScreen:                                                 │
│  ├─ _selectedDay = "Luni"                                        │
│  ├─ _selectedMoment = "Dimineață"                                │
│  └─ _selectedTime = TimeOfDay(08, 30)                            │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 2. UTILIZATOR APASĂ "TRIMITE LA DISPOZITIV"                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  _sendAlarm() function:                                          │
│  ├─ Construiește comanda: "Luni Dimineata 08:30"                │
│  └─ Apelează BleProvider.sendCommand(command)                   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 3. BLE PROVIDER TRIMITE COMANDA                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  BleProvider.sendCommand():                                      │
│  ├─ Convertește string → bytes: [76, 117, 110, 105, ...]        │
│  ├─ Apelează characteristic.write(bytes)                        │
│  └─ Actualizează statusMessage                                  │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 4. HM-10 PRIMEȘTE COMANDA                                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  HM-10 Module:                                                   │
│  ├─ Primește bytes pe caracteristica FFE1                       │
│  ├─ Transmite la microcontroller prin UART                      │
│  └─ Asteaptă confirmare                                         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 5. MICROCONTROLLER PROCESEAZĂ COMANDA                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Arduino/PIC:                                                    │
│  ├─ Primește: "Luni Dimineata 08:30"                            │
│  ├─ Parsează: day="Luni", moment="Dimineata", time="08:30"      │
│  ├─ Activează motor/solenoid                                    │
│  ├─ Eliberează pastila                                          │
│  └─ Trimite confirmare: "Pastila luata"                         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 6. HM-10 PRIMEȘTE CONFIRMARE                                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  HM-10 Module:                                                   │
│  ├─ Primește: "Pastila luata"                                   │
│  ├─ Transmite pe caracteristica FFE1 (notify)                   │
│  └─ Aplicația primește notificare                               │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 7. APLICAȚIE PRIMEȘTE CONFIRMARE                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  BleProvider._handleReceivedData():                              │
│  ├─ Primește bytes: [80, 97, 115, 116, 105, 108, 97, ...]      │
│  ├─ Convertește în string: "Pastila luata"                      │
│  ├─ Adaugă în _receivedMessages                                 │
│  └─ Notifică listeners (UI se actualizează)                     │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 8. ISTORIC SALVEAZĂ MESAJUL                                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  HistoryProvider.addEntry():                                     │
│  ├─ Creează HistoryEntry cu mesaj și timestamp                  │
│  ├─ Salvează în SQLite database                                 │
│  ├─ Adaugă în lista _history                                    │
│  └─ Notifică listeners (UI se actualizează)                     │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ 9. UI AFIȘEAZĂ CONFIRMARE                                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  SetAlarmScreen:                                                 │
│  ├─ Afișează: "✓ Alarmă trimisă!"                               │
│  ├─ Mesajul dispare după 3 secunde                              │
│  └─ Utilizatorul poate vedea intrarea în "Istoric"              │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📦 Structura Claselor

### BleProvider
```
BleProvider (ChangeNotifier)
├── Properties:
│   ├── _flutterBlue: FlutterBluePlus
│   ├── _scanResults: List<ScanResult>
│   ├── _connectedDevice: BluetoothDevice?
│   ├── _writeCharacteristic: BluetoothCharacteristic?
│   ├── _notifyCharacteristic: BluetoothCharacteristic?
│   ├── _isScanning: bool
│   ├── _isConnected: bool
│   ├── _statusMessage: String
│   └── _receivedMessages: List<String>
│
├── Public Methods:
│   ├── startScan(): Future<void>
│   ├── stopScan(): Future<void>
│   ├── connectToDevice(device): Future<bool>
│   ├── disconnectDevice(): Future<void>
│   └── sendCommand(command): Future<bool>
│
└── Private Methods:
    ├── _discoverServices(device): Future<void>
    └── _handleReceivedData(value): void
```

### HistoryProvider
```
HistoryProvider (ChangeNotifier)
├── Properties:
│   ├── _database: Database
│   ├── _history: List<HistoryEntry>
│   └── _isInitialized: bool
│
├── Public Methods:
│   ├── _initializeDatabase(): Future<void>
│   ├── _loadHistory(): Future<void>
│   ├── addEntry(message): Future<void>
│   ├── clearHistory(): Future<void>
│   └── deleteEntry(id): Future<void>
│
└── Private Methods:
    └── (none)
```

### HistoryEntry
```
HistoryEntry
├── Properties:
│   ├── id: int?
│   ├── message: String
│   └── timestamp: DateTime
│
├── Methods:
│   ├── toMap(): Map<String, dynamic>
│   ├── fromMap(map): HistoryEntry (factory)
│   └── formattedDateTime: String (getter)
│
└── Database Mapping:
    ├── id → INTEGER PRIMARY KEY
    ├── message → TEXT
    ├── timestamp → TEXT (ISO8601)
    └── created_at → DATETIME
```

---

## 🔌 Interfața BLE

### Serviciu FFE0
```
Service UUID: FFE0
├── Characteristic FFE1
│   ├── UUID: FFE1
│   ├── Properties:
│   │   ├── Read (citire)
│   │   ├── Write (scriere)
│   │   └── Notify (notificări)
│   │
│   ├── Write Format:
│   │   └── "ZI MOMENT ORA" (ex: "Luni Dimineata 08:30")
│   │
│   └── Notify Format:
│       └── Orice text (ex: "Pastila luata")
```

### Fluxul BLE
```
┌─────────────────────────────────────────────────────────────────┐
│ APLICAȚIE                                                        │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [BLE Write]
                    "Luni Dimineata 08:30"
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ HM-10 MODULE (FFE0/FFE1)                                        │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [UART TX]
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ MICROCONTROLLER                                                  │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [Procesare]
                    [Activare Motor]
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ HM-10 MODULE (FFE0/FFE1)                                        │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [BLE Notify]
                    "Pastila luata"
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ APLICAȚIE                                                        │
└─────────────────────────────────────────────────────────────────┘
```

---

## 💾 Baza de Date

### Tabel: history
```sql
CREATE TABLE history (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  message TEXT NOT NULL,
  timestamp TEXT NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP
)
```

### Exemplu Date
```
id | message                              | timestamp                    | created_at
---|--------------------------------------|------------------------------|-------------------
1  | Alarmă trimisă: Luni Dimineata 08:30 | 2026-05-18T22:55:00.000000  | 2026-05-18 22:55:00
2  | Pastila luata                        | 2026-05-18T08:30:15.000000  | 2026-05-18 08:30:15
3  | Alarmă trimisă: Marți Prânz 12:00    | 2026-05-19T10:20:00.000000  | 2026-05-19 10:20:00
```

---

## 🔐 Fluxul Permisiuni

### iOS
```
Utilizator deschide aplicația
        ↓
iOS cere permisiune Bluetooth
        ↓
Utilizator acceptă
        ↓
Aplicația poate accesa BLE
```

### Android
```
Utilizator deschide aplicația
        ↓
Android cere permisiuni (runtime):
├─ BLUETOOTH
├─ BLUETOOTH_ADMIN
├─ BLUETOOTH_SCAN
├─ BLUETOOTH_CONNECT
└─ ACCESS_FINE_LOCATION
        ↓
Utilizator acceptă
        ↓
Aplicația poate accesa BLE
```

---

## 🎯 State Management Flow

```
┌─────────────────────────────────────────────────────────────────┐
│ UI LAYER (Widgets)                                              │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [Consumer<Provider>]
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ PROVIDER LAYER (ChangeNotifier)                                 │
├─────────────────────────────────────────────────────────────────┤
│ • BleProvider                                                    │
│ • HistoryProvider                                               │
└─────────────────────────────────────────────────────────────────┘
                           ↓
                    [notifyListeners()]
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│ UI LAYER (Widgets) - Rebuild                                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📱 Ecrane și Navigare

```
┌─────────────────────────────────────────────────────────────────┐
│ HomeScreen (BottomNavigationBar)                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────────┐      ┌──────────────────────┐         │
│  │  SetAlarmScreen      │      │  HistoryScreen       │         │
│  ├──────────────────────┤      ├──────────────────────┤         │
│  │ • Day Selector       │      │ • History List       │         │
│  │ • Moment Selector    │      │ • Clear Button       │         │
│  │ • Time Picker        │      │ • Delete Buttons     │         │
│  │ • BLE Connect Button │      │                      │         │
│  │ • Send Button        │      │                      │         │
│  │ • Status Label       │      │                      │         │
│  └──────────────────────┘      └──────────────────────┘         │
│         ↓                                                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  BleConnectionScreen (Modal)                            │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ • Scan Status                                            │   │
│  │ • Rescan Button                                          │   │
│  │ • Device List                                            │   │
│  │ • Connect Buttons                                        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Lifecycle

### Aplicație
```
main()
  ↓
PillDispenserApp (MultiProvider)
  ├─ BleProvider (init)
  └─ HistoryProvider (init)
      ├─ _initializeDatabase()
      └─ _loadHistory()
  ↓
HomeScreen
  ├─ SetAlarmScreen (default)
  └─ HistoryScreen (tab 2)
```

### BleProvider
```
init
  ↓
Asteaptă utilizator
  ↓
startScan()
  ├─ Scanează 10 secunde
  └─ Afișează rezultate
  ↓
connectToDevice()
  ├─ Se conectează
  ├─ Descoperă servicii
  └─ Activează notificări
  ↓
Asteaptă comenzi
  ↓
sendCommand()
  ├─ Trimite bytes
  └─ Asteaptă confirmare
  ↓
dispose()
  └─ Deconectează
```

### HistoryProvider
```
init
  ↓
_initializeDatabase()
  ├─ Creează tabel dacă nu există
  └─ Deschide conexiune
  ↓
_loadHistory()
  ├─ Citește din baza de date
  └─ Populează lista
  ↓
Asteaptă adăugări
  ↓
addEntry()
  ├─ Salvează în BD
  └─ Actualizează lista
  ↓
dispose()
  └─ Închide baza de date
```

---

Gata! 🎉 Arhitectura este completă și bine documentată.
