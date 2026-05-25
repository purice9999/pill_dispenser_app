import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ble_provider.dart';
import '../providers/history_provider.dart';
import '../providers/alarm_provider.dart';
import 'ble_connection_screen.dart';

/// Ecranul pentru setarea alarmei
class SetAlarmScreen extends StatefulWidget {
  const SetAlarmScreen({super.key});

  @override
  State<SetAlarmScreen> createState() => _SetAlarmScreenState();
}

class _SetAlarmScreenState extends State<SetAlarmScreen> {
  // Index pentru selecție (mapăm la display și la comanda ASCII)
  int _selectedDayIndex = 0;
  int _selectedMomentIndex = 0;
  TimeOfDay _selectedTime = TimeOfDay.now();

  String _confirmationMessage = '';
  bool _showConfirmation = false;

  @override
  Widget build(BuildContext context) {
    return Consumer<BleProvider>(
      builder: (context, bleProvider, _) {
        return SingleChildScrollView(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              // ─── STATUS CONEXIUNE ───────────────────────────────────────
              _buildConnectionStatus(bleProvider),
              const SizedBox(height: 24),

              // ─── SELECTOR ZI ─────────────────────────────────────────────
              _buildDaySelector(),
              const SizedBox(height: 20),

              // ─── SELECTOR MOMENT ────────────────────────────────────────
              _buildMomentSelector(),
              const SizedBox(height: 20),

              // ─── SELECTOR ORA ───────────────────────────────────────────
              _buildTimeSelector(context),
              const SizedBox(height: 24),

              // ─── BUTON CONECTARE BLE ────────────────────────────────────
              ElevatedButton.icon(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (context) => const BleConnectionScreen(),
                    ),
                  );
                },
                icon: const Icon(Icons.bluetooth),
                label: const Text('Conectare BLE'),
                style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.symmetric(vertical: 12),
                  backgroundColor: Colors.blue,
                ),
              ),
              const SizedBox(height: 16),

              // ─── BUTON TRIMITE ──────────────────────────────────────────
              ElevatedButton.icon(
                onPressed: bleProvider.isConnected
                    ? () => _sendAlarm(context, bleProvider)
                    : null,
                icon: const Icon(Icons.send),
                label: const Text('Trimite la Dispozitiv'),
                style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.symmetric(vertical: 12),
                  backgroundColor: Colors.green,
                  disabledBackgroundColor: Colors.grey,
                ),
              ),
              const SizedBox(height: 20),

              // ─── MESAJ CONFIRMARE ───────────────────────────────────────
              if (_showConfirmation)
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.green.shade100,
                    border: Border.all(color: Colors.green),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    _confirmationMessage,
                    style: const TextStyle(
                      color: Colors.green,
                      fontWeight: FontWeight.bold,
                    ),
                    textAlign: TextAlign.center,
                  ),
                ),

              // ─── MESAJ STATUS ───────────────────────────────────────────
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.blue.shade50,
                  border: Border.all(color: Colors.blue),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Text(
                  bleProvider.statusMessage,
                  style: const TextStyle(
                    color: Colors.blue,
                    fontSize: 12,
                  ),
                  textAlign: TextAlign.center,
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  // ─── WIDGET-URI HELPER ───────────────────────────────────────────────────

  Widget _buildConnectionStatus(BleProvider bleProvider) {
    return Card(
      color: bleProvider.isConnected ? Colors.green.shade50 : Colors.red.shade50,
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Row(
          children: [
            Icon(
              bleProvider.isConnected ? Icons.check_circle : Icons.cancel,
              color: bleProvider.isConnected ? Colors.green : Colors.red,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    bleProvider.isConnected ? 'Conectat' : 'Deconectat',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      color: bleProvider.isConnected ? Colors.green : Colors.red,
                    ),
                  ),
                  if (bleProvider.connectedDevice != null)
                    Text(
                      bleProvider.connectedDevice!.platformName,
                      style: const TextStyle(fontSize: 12),
                    ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDaySelector() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'Selectează Ziua',
          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
        ),
        const SizedBox(height: 8),
        Container(
          decoration: BoxDecoration(
            border: Border.all(color: Colors.blue),
            borderRadius: BorderRadius.circular(8),
          ),
          child: DropdownButton<int>(
            value: _selectedDayIndex,
            isExpanded: true,
            underline: const SizedBox(),
            padding: const EdgeInsets.symmetric(horizontal: 12),
            onChanged: (int? newIndex) {
              setState(() {
                _selectedDayIndex = newIndex ?? 0;
              });
            },
            items: List.generate(AlarmProvider.daysDisplay.length, (i) {
              return DropdownMenuItem<int>(
                value: i,
                child: Text(AlarmProvider.daysDisplay[i]),
              );
            }),
          ),
        ),
      ],
    );
  }

  Widget _buildMomentSelector() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'Selectează Momentul',
          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
        ),
        const SizedBox(height: 8),
        Container(
          decoration: BoxDecoration(
            border: Border.all(color: Colors.blue),
            borderRadius: BorderRadius.circular(8),
          ),
          child: DropdownButton<int>(
            value: _selectedMomentIndex,
            isExpanded: true,
            underline: const SizedBox(),
            padding: const EdgeInsets.symmetric(horizontal: 12),
            onChanged: (int? newIndex) {
              setState(() {
                _selectedMomentIndex = newIndex ?? 0;
              });
            },
            items: List.generate(AlarmProvider.momentsDisplay.length, (i) {
              return DropdownMenuItem<int>(
                value: i,
                child: Text(AlarmProvider.momentsDisplay[i]),
              );
            }),
          ),
        ),
      ],
    );
  }

  Widget _buildTimeSelector(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const Text(
          'Selectează Ora',
          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 14),
        ),
        const SizedBox(height: 8),
        GestureDetector(
          onTap: () async {
            final TimeOfDay? picked = await showTimePicker(
              context: context,
              initialTime: _selectedTime,
            );
            if (picked != null) {
              setState(() {
                _selectedTime = picked;
              });
            }
          },
          child: Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              border: Border.all(color: Colors.blue),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  _selectedTime.format(context),
                  style: const TextStyle(fontSize: 16),
                ),
                const Icon(Icons.access_time, color: Colors.blue),
              ],
            ),
          ),
        ),
      ],
    );
  }

  // ─── LOGICA TRIMITERE ────────────────────────────────────────────────────

  void _sendAlarm(BuildContext context, BleProvider bleProvider) async {
    // Construiește comanda ASCII: "ZI MOMENT ORA" (fără diacritice, pentru PIC)
    final hour = _selectedTime.hour.toString().padLeft(2, '0');
    final minute = _selectedTime.minute.toString().padLeft(2, '0');
    final day = AlarmProvider.days[_selectedDayIndex];
    final moment = AlarmProvider.moments[_selectedMomentIndex];
    final command = '$day $moment $hour:$minute';

    // Cache providers before async gap
    final historyProvider = Provider.of<HistoryProvider>(context, listen: false);

    // Trimite comanda prin BLE
    final success = await bleProvider.sendCommand(command);

    if (!mounted) return;

    if (success) {
      await historyProvider.addEntry('Alarmă trimisă: $command');

      // Afișează confirmare
      setState(() {
        _confirmationMessage = '✓ Alarmă trimisă!\n$command';
        _showConfirmation = true;
      });

      // Ascunde mesajul după 3 secunde
      Future.delayed(const Duration(seconds: 3), () {
        if (mounted) {
          setState(() {
            _showConfirmation = false;
          });
        }
      });
    }
  }
}
