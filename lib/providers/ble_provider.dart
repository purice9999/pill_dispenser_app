import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BleProvider extends ChangeNotifier {
  List<ScanResult> _scanResults = [];
  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _writeCharacteristic;
  bool _isScanning = false;
  bool _isConnected = false;
  String _statusMessage = 'Deconectat';
  final List<String> _receivedMessages = [];

  StreamSubscription<List<ScanResult>>? _scanSubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionSubscription;
  StreamSubscription<List<int>>? _characteristicSubscription;

  List<ScanResult> get scanResults => _scanResults;
  bool get isScanning => _isScanning;
  bool get isConnected => _isConnected;
  String get statusMessage => _statusMessage;
  BluetoothDevice? get connectedDevice => _connectedDevice;
  List<String> get receivedMessages => _receivedMessages;

  Future<void> startScan() async {
    if (_isScanning) return;
    try {
      _isScanning = true;
      _scanResults.clear();
      notifyListeners();

      await _scanSubscription?.cancel();
      _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
        _scanResults = results;
        notifyListeners();
      });

      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
      await FlutterBluePlus.isScanning.where((scanning) => !scanning).first;

      _isScanning = false;
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare scanare: $e';
      _isScanning = false;
      notifyListeners();
    }
  }

  Future<void> stopScan() async {
    try {
      await FlutterBluePlus.stopScan();
      await _scanSubscription?.cancel();
      _scanSubscription = null;
      _isScanning = false;
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      notifyListeners();
    }
  }

  Future<bool> connectToDevice(BluetoothDevice device) async {
    try {
      _statusMessage = 'Se conectează...';
      notifyListeners();

      await device.connect();
      _connectedDevice = device;
      _isConnected = true;
      _statusMessage = 'Conectat la ${device.platformName}';

      await _connectionSubscription?.cancel();
      _connectionSubscription = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          _isConnected = false;
          _connectedDevice = null;
          _writeCharacteristic = null;
          _statusMessage = 'Deconectat';
          notifyListeners();
        }
      });

      await _discoverServices(device);
      notifyListeners();
      return true;
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      _isConnected = false;
      _connectedDevice = null;
      notifyListeners();
      return false;
    }
  }

  Future<void> disconnectDevice() async {
    try {
      await _connectionSubscription?.cancel();
      _connectionSubscription = null;
      await _characteristicSubscription?.cancel();
      _characteristicSubscription = null;
      if (_connectedDevice != null) {
        await _connectedDevice!.disconnect();
      }
      _connectedDevice = null;
      _isConnected = false;
      _writeCharacteristic = null;
      _statusMessage = 'Deconectat';
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      notifyListeners();
    }
  }

  Future<bool> sendCommand(String command) async {
    try {
      if (_writeCharacteristic == null) {
        _statusMessage = 'Nu e conectat';
        notifyListeners();
        return false;
      }

      List<int> bytes = command.codeUnits;
      await _writeCharacteristic!.write(bytes, withoutResponse: false);
      _statusMessage = 'Trimis: $command';
      notifyListeners();
      return true;
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      notifyListeners();
      return false;
    }
  }

  Future<void> _discoverServices(BluetoothDevice device) async {
    try {
      List<BluetoothService> services = await device.discoverServices();

      for (BluetoothService service in services) {
        if (service.uuid.toString().toUpperCase().contains('FFE0')) {
          for (BluetoothCharacteristic characteristic in service.characteristics) {
            if (characteristic.uuid.toString().toUpperCase().contains('FFE1')) {
              _writeCharacteristic = characteristic;

              if (characteristic.properties.notify) {
                await characteristic.setNotifyValue(true);
                await _characteristicSubscription?.cancel();
                _characteristicSubscription =
                    characteristic.onValueReceived.listen(_handleReceivedData);
              }
            }
          }
        }
      }

      _statusMessage =
          _writeCharacteristic == null ? 'FFE1 nu gasit' : 'Conectat';
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      notifyListeners();
    }
  }

  void _handleReceivedData(List<int> value) {
    try {
      String message = String.fromCharCodes(value);
      _receivedMessages.add(message);
      _statusMessage = 'Primit: $message';
      notifyListeners();
    } catch (e) {
      _statusMessage = 'Eroare: $e';
      notifyListeners();
    }
  }

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _connectionSubscription?.cancel();
    _characteristicSubscription?.cancel();
    _connectedDevice?.disconnect();
    super.dispose();
  }
}
