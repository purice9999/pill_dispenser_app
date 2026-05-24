import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

/// Provider pentru gestionarea conexiunilor BLE
class BleProvider extends ChangeNotifier {
  final FlutterBluePlus _flutterBlue = FlutterBluePlus();

  // Dispozitive BLE găsite
  List<ScanResult> _scanResults = [];

  // Dispozitiv conectat
  BluetoothDevice? _connectedDevice;

  // Caracteristica pentru scriere/citire
  BluetoothCharacteristic? _writeCharacteristic;
  BluetoothCharacteristic? _notifyCharacteristic;

  // Stări
  bool _isScanning = false;
  bool _isConnected = false;
  String _statusMessage = 'Deconectat';

  // Mesaje primite
  List<String> _receivedMessages = [];

  // ─── GETTERS ────────────────────────────────────────────────────────────

  List<ScanResult> get scanResults => _scanResults;
  bool get isScanning => _isScanning;
  bool get isConnected => _isConnected;
  String get statusMessage => _statusMessage;
  BluetoothDevice? get connectedDevice => _connectedDevice;
  List<String> get receivedMessages => _receivedMessages;

  // ─── METODE PUBLICE ─────────────────────────────────────────────────────

  /// Scanează dispozitive BLE
  Future<void> startScan() async {
    try {
      _isScanning = true;
      _scanResults.clear();
      notifyListeners();

      // Pornește scanarea
      await _flutterBlue.startScan(
        timeout: const Duration(seconds: 10),
      );

      // Ascultă rezultatele scanării
      _flutterBlue.scanResults.listen((results) {
        _scanResults = results;
        notifyListeners();
      });

      // Oprește scanarea după 10 secunde
      await Future.delayed(const Duration(seconds: 10));
      await _flutterBlue.stopScan();
      _isScanning = false;
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare la scanare: $e';
      _isScanning = false;
      notifyListeners();
    }
  }

  /// Oprește scanarea
  Future<void> stopScan() async {
    try {
      await _flutterBlue.stopScan();
      _isScanning = false;
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare la oprire scanare: $e';
      notifyListeners();
    }
  }

  /// Se conectează la un dispozitiv BLE
  Future<bool> connectToDevice(BluetoothDevice device) async {
    try {
      _statusMessage = 'Se conectează...';
      notifyListeners();

      await device.connect(timeout: const Duration(seconds: 10));
      _connectedDevice = device;
      _isConnected = true;
      _statusMessage = 'Conectat la ${device.name}';

      // Descoperă serviciile și caracteristicile
      await _discoverServices(device);

      notifyListeners();
      return true;
    } catch (e) {
      _statusMessage = 'Eroare conexiune: $e';
      _isConnected = false;
      _connectedDevice = null;
      notifyListeners();
      return false;
    }
  }

  /// Se deconectează de la dispozitiv
  Future<void> disconnectDevice() async {
    try {
      if (_connectedDevice != null) {
        await _connectedDevice!.disconnect();
      }
      _connectedDevice = null;
      _isConnected = false;
      _writeCharacteristic = null;
      _notifyCharacteristic = null;
      _statusMessage = 'Deconectat';
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare deconexiune: $e';
      notifyListeners();
    }
  }

  /// Trimite o comandă text prin BLE
  Future<bool> sendCommand(String command) async {
    try {
      if (_writeCharacteristic == null) {
        _statusMessage = 'Caracteristica de scriere nu este disponibilă';
        notifyListeners();
        return false;
      }

      // Convertește stringul în bytes
      List<int> bytes = command.codeUnits;

      // Trimite datele
      await _writeCharacteristic!.write(bytes, withoutResponse: false);

      _statusMessage = 'Comandă trimisă: $command';
      notifyListeners();
      return true;
    } catch (e) {
      _statusMessage = 'Eroare trimitere: $e';
      notifyListeners();
      return false;
    }
  }

  // ─── METODE PRIVATE ─────────────────────────────────────────────────────

  /// Descoperă serviciile și caracteristicile dispozitivului
  Future<void> _discoverServices(BluetoothDevice device) async {
    try {
      List<BluetoothService> services = await device.discoverServices();

      for (BluetoothService service in services) {
        // Caută serviciul FFE0 (HM-10 standard)
        if (service.uuid.toString().toUpperCase().contains('FFE0')) {
          for (BluetoothCharacteristic characteristic
              in service.characteristics) {
            // Caută caracteristica FFE1
            if (characteristic.uuid.toString().toUpperCase().contains('FFE1')) {
              _writeCharacteristic = characteristic;
              _notifyCharacteristic = characteristic;

              // Activează notificările
              if (characteristic.properties.notify) {
                await characteristic.setNotifyValue(true);
                characteristic.onValueReceived.listen((value) {
                  _handleReceivedData(value);
                });
              }
            }
          }
        }
      }

      if (_writeCharacteristic == null) {
        _statusMessage = 'Caracteristica FFE1 nu a fost găsită';
      } else {
        _statusMessage = 'Servicii descoperite cu succes';
      }
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare descoperire servicii: $e';
      notifyListeners();
    }
  }

  /// Procesează datele primite de la HM-10
  void _handleReceivedData(List<int> value) {
    try {
      String message = String.fromCharCodes(value);
      _receivedMessages.add(message);
      _statusMessage = 'Mesaj primit: $message';
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare procesare date: $e';
      notifyListeners();
    }
  }

  @override
  void dispose() {
    disconnectDevice();
    super.dispose();
  }
}
