import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../providers/ble_provider.dart';

/// Ecranul pentru conectarea la dispozitivul BLE HM-10
class BleConnectionScreen extends StatefulWidget {
  const BleConnectionScreen({Key? key}) : super(key: key);

  @override
  State<BleConnectionScreen> createState() => _BleConnectionScreenState();
}

class _BleConnectionScreenState extends State<BleConnectionScreen> {
  @override
  void initState() {
    super.initState();
    // Pornește scanarea la deschiderea ecranului
    Future.delayed(const Duration(milliseconds: 500), () {
      if (mounted) {
        Provider.of<BleProvider>(context, listen: false).startScan();
      }
    });
  }

  @override
  void dispose() {
    // Oprește scanarea la închiderea ecranului
    Provider.of<BleProvider>(context, listen: false).stopScan();
    super.dispose();
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
          return Column(
            children: [
              // ─── STATUS SCANARE ─────────────────────────────────────────
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
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                            ),
                          )
                        else
                          Icon(
                            Icons.bluetooth_searching,
                            color: Colors.grey.shade600,
                          ),
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

              // ─── BUTON RESCANNARE ───────────────────────────────────────
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: ElevatedButton.icon(
                  onPressed: bleProvider.isScanning
                      ? null
                      : () {
                          bleProvider.startScan();
                        },
                  icon: const Icon(Icons.refresh),
                  label: const Text('Rescannare'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.blue,
                    disabledBackgroundColor: Colors.grey,
                  ),
                ),
              ),
              const SizedBox(height: 16),

              // ─── LISTA DISPOZITIVE ──────────────────────────────────────
              Expanded(
                child: bleProvider.scanResults.isEmpty
                    ? Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(
                              Icons.bluetooth_disabled,
                              size: 64,
                              color: Colors.grey.shade300,
                            ),
                            const SizedBox(height: 16),
                            Text(
                              'Niciun dispozitiv găsit',
                              style: TextStyle(
                                color: Colors.grey.shade600,
                                fontSize: 16,
                              ),
                            ),
                            const SizedBox(height: 8),
                            Text(
                              'Asigură-te că HM-10 este pornit și în rază',
                              style: TextStyle(
                                color: Colors.grey.shade500,
                                fontSize: 12,
                              ),
                            ),
                          ],
                        ),
                      )
                    : ListView.builder(
                        padding: const EdgeInsets.symmetric(horizontal: 16),
                        itemCount: bleProvider.scanResults.length,
                        itemBuilder: (context, index) {
                          final result = bleProvider.scanResults[index];
                          return _buildDeviceCard(
                            context,
                            result,
                            bleProvider,
                          );
                        },
                      ),
              ),
            ],
          );
        },
      ),
    );
  }

  /// Widget pentru o intrare din lista de dispozitive
  Widget _buildDeviceCard(
    BuildContext context,
    ScanResult result,
    BleProvider bleProvider,
  ) {
    final device = result.device;
    final isConnected = bleProvider.connectedDevice?.id == device.id;

    return Card(
      margin: const EdgeInsets.symmetric(vertical: 8),
      color: isConnected ? Colors.green.shade50 : null,
      child: ListTile(
        leading: Icon(
          Icons.bluetooth,
          color: isConnected ? Colors.green : Colors.blue,
        ),
        title: Text(
          device.name.isEmpty ? 'Dispozitiv necunoscut' : device.name,
          style: TextStyle(
            fontWeight: isConnected ? FontWeight.bold : FontWeight.normal,
          ),
        ),
        subtitle: Text(
          device.id.toString(),
          style: const TextStyle(fontSize: 11),
        ),
        trailing: isConnected
            ? const Icon(Icons.check_circle, color: Colors.green)
            : ElevatedButton(
                onPressed: () => _connectToDevice(context, device, bleProvider),
                child: const Text('Conectare'),
              ),
      ),
    );
  }

  /// Conectează la dispozitivul selectat
  void _connectToDevice(
    BuildContext context,
    BluetoothDevice device,
    BleProvider bleProvider,
  ) async {
    // Oprește scanarea
    await bleProvider.stopScan();

    // Afișează loading
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        content: Row(
          children: [
            const CircularProgressIndicator(),
            const SizedBox(width: 16),
            Expanded(
              child: Text('Se conectează la ${device.name}...'),
            ),
          ],
        ),
      ),
    );

    // Încearcă conexiunea
    final success = await bleProvider.connectToDevice(device);

    if (mounted) {
      Navigator.pop(context); // Închide loading dialog

      if (success) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Conectat la ${device.name}'),
            backgroundColor: Colors.green,
          ),
        );
        Future.delayed(const Duration(seconds: 1), () {
          if (mounted) {
            Navigator.pop(context); // Revine la SetAlarmScreen
          }
        });
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Eroare conectare: ${bleProvider.statusMessage}'),
            backgroundColor: Colors.red,
          ),
        );
        // Reia scanarea
        bleProvider.startScan();
      }
    }
  }
}
