import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../providers/ble_provider.dart';

class BleConnectionScreen extends StatefulWidget {
  const BleConnectionScreen({super.key});

  @override
  State<BleConnectionScreen> createState() => _BleConnectionScreenState();
}

class _BleConnectionScreenState extends State<BleConnectionScreen> {
  static const String _hm10Name = 'HMSoft';

  @override
  void initState() {
    super.initState();
    Future.delayed(const Duration(milliseconds: 500), () async {
      if (!mounted) return;
      final ble = Provider.of<BleProvider>(context, listen: false);
      // Incearca conectare directa la HMSoft cunoscut; fallback pe scan
      final ok = await ble.connectToKnownDevice();
      if (!ok && mounted) ble.startScan();
    });
  }

  @override
  void dispose() {
    Provider.of<BleProvider>(context, listen: false).stopScan();
    super.dispose();
  }

  bool _isHM10(ScanResult r) => r.device.platformName == _hm10Name;

  /// Sorteaza lista: HMSoft primul, restul dupa semnal
  List<ScanResult> _sorted(List<ScanResult> results) {
    final list = List<ScanResult>.from(results);
    list.sort((a, b) {
      final aIsHM = _isHM10(a) ? 0 : 1;
      final bIsHM = _isHM10(b) ? 0 : 1;
      if (aIsHM != bIsHM) return aIsHM - bIsHM;
      return b.rssi.compareTo(a.rssi); // semnal mai puternic primul
    });
    return list;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Conectare BLE'),
        centerTitle: true,
      ),
      body: Consumer<BleProvider>(
        builder: (context, bleProvider, _) {
          final sorted = _sorted(bleProvider.scanResults);

          return Column(
            children: [
              // ─── STATUS SCANARE ────────────────────────────────────
              Padding(
                padding: const EdgeInsets.all(16.0),
                child: Card(
                  color: bleProvider.isScanning
                      ? Colors.blue.shade50
                      : Colors.grey.shade50,
                  child: Padding(
                    padding: const EdgeInsets.all(12),
                    child: Row(
                      children: [
                        if (bleProvider.isScanning)
                          const SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        else
                          Icon(Icons.bluetooth_searching,
                              color: Colors.grey.shade600),
                        const SizedBox(width: 12),
                        Expanded(
                          child: Text(
                            bleProvider.isScanning
                                ? 'Se scanează dispozitive...'
                                : 'Scanare completă',
                            style: TextStyle(
                              color: bleProvider.isScanning
                                  ? Colors.blue
                                  : Colors.grey.shade600,
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),

              // ─── BUTON RESCANNARE ──────────────────────────────────
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: ElevatedButton.icon(
                  onPressed:
                      bleProvider.isScanning ? null : bleProvider.startScan,
                  icon: const Icon(Icons.refresh),
                  label: const Text('Rescannare'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.blue,
                    disabledBackgroundColor: Colors.grey,
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // ─── LISTA DISPOZITIVE ─────────────────────────────────
              Expanded(
                child: sorted.isEmpty
                    ? Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(Icons.bluetooth_disabled,
                                size: 64, color: Colors.grey.shade300),
                            const SizedBox(height: 16),
                            Text(
                              'Niciun dispozitiv găsit',
                              style: TextStyle(
                                  color: Colors.grey.shade600, fontSize: 16),
                            ),
                            const SizedBox(height: 8),
                            Text(
                              'Asigură-te că HM-10 este pornit și în rază',
                              style: TextStyle(
                                  color: Colors.grey.shade500, fontSize: 12),
                            ),
                          ],
                        ),
                      )
                    : ListView.builder(
                        padding: const EdgeInsets.symmetric(horizontal: 16),
                        itemCount: sorted.length,
                        itemBuilder: (context, index) => _buildDeviceCard(
                          context,
                          sorted[index],
                          bleProvider,
                        ),
                      ),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildDeviceCard(
    BuildContext context,
    ScanResult result,
    BleProvider bleProvider,
  ) {
    final device = result.device;
    final name = device.platformName.isEmpty ? 'Necunoscut' : device.platformName;
    final isHM10 = _isHM10(result);
    final isConnected = bleProvider.connectedDevice?.remoteId == device.remoteId;

    return Card(
      margin: const EdgeInsets.symmetric(vertical: 6),
      color: isConnected
          ? Colors.green.shade50
          : isHM10
              ? Colors.blue.shade50
              : null,
      shape: isHM10
          ? RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(12),
              side: BorderSide(color: Colors.blue.shade400, width: 1.5),
            )
          : null,
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: isConnected
              ? Colors.green.shade100
              : isHM10
                  ? Colors.blue.shade100
                  : Colors.grey.shade100,
          child: Icon(
            isConnected ? Icons.check : Icons.bluetooth,
            color: isConnected
                ? Colors.green
                : isHM10
                    ? Colors.blue
                    : Colors.grey,
          ),
        ),
        title: Row(
          children: [
            Text(
              name,
              style: TextStyle(
                fontWeight:
                    (isHM10 || isConnected) ? FontWeight.bold : FontWeight.normal,
              ),
            ),
            if (isHM10 && !isConnected) ...[
              const SizedBox(width: 8),
              Container(
                padding:
                    const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                decoration: BoxDecoration(
                  color: Colors.blue,
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Text(
                  'HM-10',
                  style: TextStyle(color: Colors.white, fontSize: 10),
                ),
              ),
            ],
          ],
        ),
        subtitle: Text(
          '${device.remoteId}  •  ${result.rssi} dBm',
          style: const TextStyle(fontSize: 11),
        ),
        trailing: isConnected
            ? const Icon(Icons.check_circle, color: Colors.green)
            : ElevatedButton(
                onPressed: () => _connectToDevice(device, bleProvider),
                style: ElevatedButton.styleFrom(
                  backgroundColor: isHM10 ? Colors.blue : null,
                ),
                child: const Text('Conectare'),
              ),
      ),
    );
  }

  void _connectToDevice(
    BluetoothDevice device,
    BleProvider bleProvider,
  ) async {
    await bleProvider.stopScan();

    if (!mounted) return;

    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => AlertDialog(
        content: Row(
          children: [
            const CircularProgressIndicator(),
            const SizedBox(width: 16),
            Expanded(
              child: Text('Se conectează la ${device.platformName}...'),
            ),
          ],
        ),
      ),
    );

    final success = await bleProvider.connectToDevice(device);

    if (!mounted) return;

    Navigator.pop(context);

    if (success) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Conectat la ${device.platformName}'),
          backgroundColor: Colors.green,
        ),
      );
      Future.delayed(const Duration(seconds: 1), () {
        if (mounted) Navigator.pop(context);
      });
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Eroare: ${bleProvider.statusMessage}'),
          backgroundColor: Colors.red,
        ),
      );
      bleProvider.startScan();
    }
  }
}
