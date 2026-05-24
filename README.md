# Pill Dispenser - Aplicație BLE pentru Dozator de Pastile

Aplicație Flutter completă pentru controlul unui dozator de pastile prin Bluetooth Low Energy (BLE) folosind modulul HM-10.

## 🎯 Funcționalități

✅ **Conectare BLE** - Scanare și conectare la modulul HM-10  
✅ **Setare Alarme** - Selectare zi, moment și oră pentru alarme  
✅ **Trimitere Comenzi** - Trimitere comenzi text prin BLE (format: "ZI MOMENT ORA")  
✅ **Istoric Local** - Salvare și vizualizare istoric pastilelor în baza de date locală  
✅ **UI Modern** - Interfață intuitivă și responsivă  
✅ **Cross-Platform** - Funcționează pe iOS și Android  

---

## 📱 Structura Proiectului

```
pill_dispenser_app/
├── lib/
│   ├── main.dart                    # Punct de intrare
│   ├── providers/
│   │   ├── ble_provider.dart        # Gestionare BLE
│   │   └── history_provider.dart    # Gestionare istoric
│   ├── models/
│   │   └── history_entry.dart       # Model pentru intrări istoric
│   └── screens/
│       ├── home_screen.dart         # Ecran principal cu navigare
│       ├── set_alarm_screen.dart    # Setare alarme
│       ├── history_screen.dart      # Vizualizare istoric
│       └── ble_connection_screen.dart # Conectare BLE
├── ios/
│   └── Runner/
│       └── Info.plist               # Configurare iOS + permisiuni BLE
├── android/
│   └── app/src/main/
│       └── AndroidManifest.xml      # Configurare Android + permisiuni
├── pubspec.yaml                     # Dependențe Flutter
└── README.md                        # Documentație
```

---

## 🚀 Instalare și Rulare

### Cerințe Preliminare

- **Flutter SDK** (versiunea 3.0+)
- **Dart SDK** (inclus cu Flutter)
- **Xcode** (pentru iOS) sau **Android Studio** (pentru Android)

### Pași de Instalare

1. **Clonează/Descarcă proiectul**
   ```bash
   cd pill_dispenser_app
   ```

2. **Instalează dependențele**
   ```bash
   flutter pub get
   ```

3. **Rulează pe emulator/dispozitiv**
   ```bash
   # iOS
   flutter run -d ios
   
   # Android
   flutter run -d android
   ```

---

## 🔧 Configurare BLE

### Modulul HM-10

- **UUID Serviciu**: `FFE0`
- **UUID Caracteristică**: `FFE1`
- **Baud Rate**: 9600
- **Format Mesaje**: `ZI MOMENT ORA` (ex: "Luni Dimineata 08:30")

### Permisiuni iOS

Permisiunile sunt deja configurate în `ios/Runner/Info.plist`:
- `NSBluetoothCentralUsageDescription` - Acces la BLE
- `NSBluetoothPeripheralUsageDescription` - Acces periferic BLE

### Permisiuni Android

Permisiunile sunt deja configurate în `android/app/src/main/AndroidManifest.xml`:
- `BLUETOOTH` - Acces Bluetooth
- `BLUETOOTH_ADMIN` - Administrare Bluetooth
- `BLUETOOTH_SCAN` - Scanare BLE (Android 12+)
- `BLUETOOTH_CONNECT` - Conectare BLE (Android 12+)
- `ACCESS_FINE_LOCATION` - Locație precisă (necesară pentru BLE)

---

## 📋 Ecrane și Funcționalități

### 1. **SetAlarmScreen** - Setarea Alarmei

**Elemente UI:**
- Selector pentru zi (Luni - Duminică)
- Selector pentru moment (Dimineață, Prânz, Seară)
- Time Picker pentru oră
- Buton "Conectare BLE"
- Buton "Trimite la Dispozitiv"
- Status conexiune BLE
- Mesaj confirmare

**Logică:**
```
1. Utilizatorul selectează ziua → salvată în _selectedDay
2. Selectează momentul → salvat în _selectedMoment
3. Selectează ora → salvată în _selectedTime
4. Apasă "Trimite la Dispozitiv"
   → Creează comanda: "ZI MOMENT ORA"
   → Trimite prin BLE la UUID FFE1
   → Salvează în istoric
   → Afișează confirmare
```

### 2. **HistoryScreen** - Istoric Pastile

**Elemente UI:**
- Listă cu intrări din istoric
- Buton "Resetare Istoric"
- Buton ștergere pentru fiecare intrare

**Format Afișare:**
```
18/05/2026 22:55 - Pastilă luată
```

**Logică:**
```
1. La deschidere → încarcă istoric din baza de date
2. Când HM-10 trimite mesaj → salvează + afișează
3. Apasă "Resetare" → șterge tot istoricul
4. Apasă ștergere pe intrare → șterge doar acea intrare
```

### 3. **BleConnectionScreen** - Conectare BLE

**Elemente UI:**
- Status scanare
- Buton "Rescannare"
- Listă dispozitive BLE

**Logică:**
```
1. Pornește scanare automat la deschidere
2. Afișează dispozitive găsite
3. Utilizator selectează HM-10
4. Se conectează la serviciul FFE0/FFE1
5. Revine la SetAlarmScreen
```

---

## 💾 Baza de Date Locală

Aplicația folosește **SQLite** pentru salvarea istoricului.

**Tabel: `history`**
```sql
CREATE TABLE history (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  message TEXT NOT NULL,
  timestamp TEXT NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP
)
```

**Exemplu Intrare:**
```
id: 1
message: "Alarmă trimisă: Luni Dimineata 08:30"
timestamp: "2026-05-18T22:55:00.000000"
created_at: "2026-05-18 22:55:00"
```

---

## 📡 Fluxul BLE

### Trimitere Comandă

```
SetAlarmScreen
    ↓
Utilizator apasă "Trimite"
    ↓
BleProvider.sendCommand("Luni Dimineata 08:30")
    ↓
Convertire string → bytes
    ↓
Scriere pe caracteristica FFE1
    ↓
HM-10 primește comanda
    ↓
Confirmare în HistoryProvider
```

### Primire Mesaj

```
HM-10 trimite mesaj
    ↓
BleProvider ascultă notificări pe FFE1
    ↓
_handleReceivedData() procesează bytes
    ↓
Convertire bytes → string
    ↓
HistoryProvider.addEntry() salvează în BD
    ↓
UI se actualizează automat (Provider)
```

---

## 🎨 Designul UI

- **Culori**: Albastru (primary), Verde (succes), Roșu (ștergere)
- **Font**: Roboto (implicit Material Design)
- **Responsive**: Adaptează la orice dimensiune ecran
- **Dark Mode**: Suportat automat prin Material 3

---

## 🐛 Troubleshooting

### Problema: "Niciun dispozitiv găsit"
**Soluție:**
- Asigură-te că HM-10 este pornit
- Verifică dacă Bluetooth este activat pe telefon
- Apasă "Rescannare"

### Problema: "Eroare conexiune"
**Soluție:**
- Deconectează și reconectează HM-10
- Restartează aplicația
- Verifică dacă UUID-urile sunt corecte (FFE0/FFE1)

### Problema: "Caracteristica FFE1 nu a fost găsită"
**Soluție:**
- Verifică firmware-ul HM-10
- Asigură-te că modulul este în modul periferic (slave)

### Problema: Permisiuni refuzate pe Android
**Soluție:**
- Mergi în Setări → Aplicații → Pill Dispenser → Permisiuni
- Activează: Locație, Bluetooth
- Restartează aplicația

---

## 📝 Exemplu Utilizare

1. **Deschide aplicația**
2. **Apasă "Conectare BLE"**
3. **Selectează "HM-10" din listă**
4. **Revine la "Setare Alarmă"**
5. **Selectează:**
   - Zi: "Luni"
   - Moment: "Dimineață"
   - Oră: "08:30"
6. **Apasă "Trimite la Dispozitiv"**
7. **Confirmarea apare pe ecran**
8. **Mergi la "Istoric" pentru a vedea mesajele**

---

## 📦 Dependențe

```yaml
flutter_blue_plus: ^1.31.0    # BLE
sqflite: ^2.3.0               # Bază de date locală
provider: ^6.0.0              # State management
intl: ^0.19.0                 # Internacionalizare
path: ^1.8.3                  # Manipulare căi
uuid: ^4.0.0                  # Generare UUID
```

---

## 🔐 Securitate

- ✅ Validare comenzi înainte de trimitere
- ✅ Salvare sigură în baza de date locală
- ✅ Permisiuni BLE conform standardelor iOS/Android
- ✅ Fără transmitere date la servere externe

---

## 📄 Licență

Proiect educațional - Liber de folosit și modificat.

---

## 👨‍💻 Autor

Creat cu Flutter și ❤️ pentru controlul dozatorului de pastile.

---

## 📞 Suport

Pentru probleme sau întrebări, consultă documentația Flutter:
- https://flutter.dev/docs
- https://pub.dev/packages/flutter_blue_plus
- https://pub.dev/packages/sqflite
