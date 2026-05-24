# 🚀 Ghid Complet de Configurare

## 1️⃣ Instalare Flutter

### Windows
```bash
# Descarcă Flutter SDK de la https://flutter.dev/docs/get-started/install/windows
# Extrage în C:\flutter

# Adaugă Flutter la PATH:
# Variabile de mediu → PATH → Adaugă C:\flutter\bin

# Verifică instalarea
flutter --version
flutter doctor
```

### macOS
```bash
# Cu Homebrew
brew install flutter

# Sau manual de la https://flutter.dev/docs/get-started/install/macos
flutter --version
flutter doctor
```

### Linux
```bash
# Descarcă de la https://flutter.dev/docs/get-started/install/linux
# Extrage și adaugă la PATH

flutter --version
flutter doctor
```

---

## 2️⃣ Configurare Proiect

### Clonează/Descarcă Proiectul
```bash
cd pill_dispenser_app
```

### Instalează Dependențele
```bash
flutter pub get
```

### Verifică Configurația
```bash
flutter doctor
```

Ar trebui să vezi:
```
✓ Flutter (Channel stable)
✓ Android toolchain
✓ Xcode (pentru iOS)
✓ VS Code
```

---

## 3️⃣ Configurare iOS

### Cerințe
- macOS 10.15+
- Xcode 12+
- CocoaPods

### Pași

1. **Deschide proiectul iOS**
   ```bash
   open ios/Runner.xcworkspace
   ```

2. **Configurează Bundle ID**
   - Selectează "Runner" în Xcode
   - Mergi la "General"
   - Schimbă "Bundle Identifier" din `com.example.pill_dispenser` în `com.tuNume.pill_dispenser`

3. **Selectează Team**
   - Mergi la "Signing & Capabilities"
   - Selectează echipa ta Apple Developer

4. **Verifică Permisiuni**
   - Mergi la "Info" tab
   - Verifică dacă sunt prezente:
     - `NSBluetoothCentralUsageDescription`
     - `NSBluetoothPeripheralUsageDescription`

5. **Rulează pe Simulator/Dispozitiv**
   ```bash
   flutter run -d ios
   ```

---

## 4️⃣ Configurare Android

### Cerințe
- Android SDK 21+
- Android Studio
- Emulator sau dispozitiv fizic

### Pași

1. **Deschide proiectul Android**
   ```bash
   open -a "Android Studio" android/
   ```

2. **Configurează Package Name**
   - Deschide `android/app/build.gradle`
   - Schimbă `applicationId` din `com.example.pill_dispenser` în `com.tuNume.pill_dispenser`

3. **Verifică Permisiuni**
   - Deschide `android/app/src/main/AndroidManifest.xml`
   - Verifică dacă sunt prezente permisiunile BLE

4. **Rulează pe Emulator/Dispozitiv**
   ```bash
   flutter run -d android
   ```

---

## 5️⃣ Configurare HM-10

### Hardware Necesar
- Modul HM-10 (BLE)
- Alimentare 3.3V
- Conexiuni UART (TX/RX)

### Configurare Modul

1. **Conexiuni**
   ```
   HM-10 VCC  → 3.3V
   HM-10 GND  → GND
   HM-10 TX   → RX (microcontroller)
   HM-10 RX   → TX (microcontroller)
   ```

2. **Comenzi AT (Serial Monitor)**
   ```
   AT+ROLE0        # Modul periferic (slave)
   AT+NAME=HM-10   # Setează nume
   AT+BAUD8        # 115200 baud
   AT+RESET        # Restart
   ```

3. **Verificare**
   ```
   AT              # Ar trebui să răspundă "OK"
   AT+ADDR?        # Afișează adresa MAC
   ```

---

## 6️⃣ Testare BLE

### Test 1: Scanare Dispozitive
```bash
flutter run -d android
# Apasă "Conectare BLE"
# Ar trebui să vezi HM-10 în listă
```

### Test 2: Conectare
```
1. Selectează HM-10
2. Ar trebui să se conecteze
3. Status ar trebui să arate "Conectat"
```

### Test 3: Trimitere Comandă
```
1. Selectează zi, moment, oră
2. Apasă "Trimite la Dispozitiv"
3. Ar trebui să apară confirmare
4. Mergi la "Istoric" pentru a vedea mesajul
```

---

## 7️⃣ Build pentru Producție

### iOS

1. **Creează Certificate**
   - Mergi pe https://developer.apple.com
   - Creează Certificate și Provisioning Profile

2. **Build Release**
   ```bash
   flutter build ios --release
   ```

3. **Upload pe App Store**
   ```bash
   open ios/Runner.xcworkspace
   # Selectează "Product" → "Archive"
   # Apasă "Distribute App"
   ```

### Android

1. **Creează Keystore**
   ```bash
   keytool -genkey -v -keystore ~/key.jks -keyalg RSA -keysize 2048 -validity 10000 -alias key
   ```

2. **Configurează Signing**
   - Deschide `android/app/build.gradle`
   - Adaugă:
   ```gradle
   signingConfigs {
       release {
           keyAlias 'key'
           keyPassword 'parola'
           storeFile file('/path/to/key.jks')
           storePassword 'parola'
       }
   }
   ```

3. **Build Release**
   ```bash
   flutter build apk --release
   ```

4. **Upload pe Google Play**
   - Mergi pe https://play.google.com/console
   - Creează aplicație nouă
   - Upload APK

---

## 8️⃣ Troubleshooting

### Flutter nu se găsește
```bash
# Adaugă Flutter la PATH
export PATH="$PATH:/path/to/flutter/bin"
```

### Eroare "CocoaPods not installed"
```bash
sudo gem install cocoapods
```

### Eroare "Android SDK not found"
```bash
flutter config --android-sdk /path/to/android/sdk
```

### Emulator nu pornește
```bash
flutter emulators
flutter emulators launch emulator_name
```

### Permisiuni refuzate pe Android
```bash
# Resetează permisiuni
adb shell pm reset-permissions
```

---

## 9️⃣ Comenzi Utile

```bash
# Verifică versiune Flutter
flutter --version

# Verifică configurație
flutter doctor

# Instalează dependențe
flutter pub get

# Rulează pe dispozitiv
flutter run

# Rulează în debug mode
flutter run -d android

# Rulează în release mode
flutter run --release

# Build APK
flutter build apk

# Build iOS
flutter build ios

# Curăță build
flutter clean

# Analizează cod
flutter analyze

# Formatează cod
flutter format lib/

# Testează
flutter test
```

---

## 🔟 Resurse Utile

- **Flutter Docs**: https://flutter.dev/docs
- **Dart Docs**: https://dart.dev/guides
- **flutter_blue_plus**: https://pub.dev/packages/flutter_blue_plus
- **SQLite**: https://pub.dev/packages/sqflite
- **Provider**: https://pub.dev/packages/provider

---

## ✅ Checklist Final

- [ ] Flutter instalat și configurat
- [ ] Proiect clonat/descărcat
- [ ] Dependențe instalate (`flutter pub get`)
- [ ] iOS configurat (Bundle ID, Team)
- [ ] Android configurat (Package Name)
- [ ] HM-10 configurat și testat
- [ ] Aplicația rulează pe emulator/dispozitiv
- [ ] BLE conectare funcționează
- [ ] Trimitere comenzi funcționează
- [ ] Istoric salvează corect

---

Gata! 🎉 Aplicația este pregătită pentru utilizare.
