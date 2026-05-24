# ⚡ Quick Start - Pornire Rapidă

## 🎯 În 5 Minute

### 1. Instalare Flutter (dacă nu ai)
```bash
# Windows
# Descarcă de la https://flutter.dev/docs/get-started/install/windows
# Extrage și adaugă la PATH

# Verifică
flutter --version
```

### 2. Clonează Proiectul
```bash
cd pill_dispenser_app
```

### 3. Instalează Dependențe
```bash
flutter pub get
```

### 4. Rulează pe Dispozitiv
```bash
# Android
flutter run -d android

# iOS
flutter run -d ios
```

### 5. Testează
- Apasă "Conectare BLE"
- Selectează HM-10
- Setează zi, moment, oră
- Apasă "Trimite la Dispozitiv"
- Mergi la "Istoric" pentru a vedea mesajele

---

## 📁 Fișiere Importante

| Fișier | Descriere |
|--------|-----------|
| `lib/main.dart` | Punct de intrare |
| `lib/providers/ble_provider.dart` | Gestionare BLE |
| `lib/providers/history_provider.dart` | Gestionare istoric |
| `lib/screens/set_alarm_screen.dart` | Setare alarme |
| `lib/screens/history_screen.dart` | Vizualizare istoric |
| `pubspec.yaml` | Dependențe |
| `README.md` | Documentație completă |
| `SETUP_GUIDE.md` | Ghid detaliat |
| `ARCHITECTURE.md` | Arhitectura |

---

## 🔧 Comenzi Utile

```bash
# Instalează dependențe
flutter pub get

# Rulează
flutter run

# Build APK (Android)
flutter build apk --release

# Build iOS
flutter build ios --release

# Curăță build
flutter clean

# Analizează cod
flutter analyze

# Formatează cod
flutter format lib/
```

---

## 🐛 Probleme Comune

### "Flutter nu se găsește"
```bash
# Adaugă la PATH
export PATH="$PATH:/path/to/flutter/bin"
```

### "Niciun dispozitiv găsit"
- Asigură-te că HM-10 este pornit
- Verifică dacă Bluetooth este activat
- Apasă "Rescannare"

### "Eroare conexiune BLE"
- Deconectează și reconectează HM-10
- Restartează aplicația
- Verifică UUID-urile (FFE0/FFE1)

---

## 📱 Structura Ecranelor

```
HomeScreen
├── SetAlarmScreen (Tab 1)
│   ├── Day Selector
│   ├── Moment Selector
│   ├── Time Picker
│   ├── BLE Connect Button
│   └── Send Button
│       └── BleConnectionScreen (Modal)
│           ├── Scan Status
│           ├── Device List
│           └── Connect Buttons
│
└── HistoryScreen (Tab 2)
    ├── History List
    ├── Clear Button
    └── Delete Buttons
```

---

## 🔌 Format Mesaje

### Trimis (Aplicație → HM-10)
```
"Luni Dimineata 08:30"
"Marți Prânz 12:00"
"Miercuri Seară 20:30"
```

### Primit (HM-10 → Aplicație)
```
"Pastila luata"
"Confirmare"
"Eroare"
```

---

## 📊 Baza de Date

Salvează automat în SQLite:
```
18/05/2026 22:55 - Pastilă luată
18/05/2026 08:30 - Alarmă trimisă: Luni Dimineata 08:30
```

---

## ✅ Checklist

- [ ] Flutter instalat
- [ ] Proiect clonat
- [ ] Dependențe instalate (`flutter pub get`)
- [ ] HM-10 configurat
- [ ] Aplicația rulează
- [ ] BLE conectare funcționează
- [ ] Trimitere comenzi funcționează
- [ ] Istoric salvează

---

## 🚀 Următorii Pași

1. **Citește README.md** pentru documentație completă
2. **Citește SETUP_GUIDE.md** pentru configurare detaliată
3. **Citește ARCHITECTURE.md** pentru înțelegerea codului
4. **Modifică HM10_FIRMWARE_EXAMPLE.ino** pentru microcontroller-ul tău
5. **Build și deploy** pe App Store / Google Play

---

## 📞 Resurse

- Flutter: https://flutter.dev
- flutter_blue_plus: https://pub.dev/packages/flutter_blue_plus
- SQLite: https://pub.dev/packages/sqflite
- Provider: https://pub.dev/packages/provider

---

Gata! 🎉 Aplicația este pregătită pentru utilizare.
