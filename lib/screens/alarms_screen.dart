import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/alarm_provider.dart';
import '../providers/ble_provider.dart';
import '../models/alarm.dart';

class AlarmsScreen extends StatefulWidget {
  const AlarmsScreen({Key? key}) : super(key: key);

  @override
  State<AlarmsScreen> createState() => _AlarmsScreenState();
}

class _AlarmsScreenState extends State<AlarmsScreen> {
  final Map<String, Map<String, TimeOfDay>> _selectedTimes = {};
  final Map<String, Map<String, bool>> _selectedAlarms = {};

  @override
  void initState() {
    super.initState();
    _initializeSelections();
  }

  void _initializeSelections() {
    for (String day in AlarmProvider.days) {
      _selectedTimes[day] = {};
      _selectedAlarms[day] = {};
      for (String moment in AlarmProvider.moments) {
        _selectedTimes[day]![moment] = const TimeOfDay(hour: 8, minute: 0);
        _selectedAlarms[day]![moment] = false;
      }
    }
  }

  Future<void> _selectTime(BuildContext context, String day, String moment) async {
    final TimeOfDay? picked = await showTimePicker(
      context: context,
      initialTime: _selectedTimes[day]![moment]!,
    );
    if (picked != null) {
      setState(() {
        _selectedTimes[day]![moment] = picked;
      });
    }
  }

  String _formatTime(TimeOfDay time) {
    return '${time.hour.toString().padLeft(2, '0')}:${time.minute.toString().padLeft(2, '0')}';
  }

  Future<void> _sendAlarmsToDevice(BuildContext context) async {
    final alarmProvider = context.read<AlarmProvider>();
    final bleProvider = context.read<BleProvider>();

    if (!bleProvider.isConnected) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Nu ești conectat la dispozitiv!')),
      );
      return;
    }

    int alarmCount = 0;
    for (String day in AlarmProvider.days) {
      for (String moment in AlarmProvider.moments) {
        if (_selectedAlarms[day]![moment]!) {
          final time = _formatTime(_selectedTimes[day]![moment]!);
          await alarmProvider.addAlarm(day, moment, time);
          await bleProvider.sendCommand('$day $moment $time');
          alarmCount++;
        }
      }
    }

    if (alarmCount > 0) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('$alarmCount alarmă(e) trimisă(e)!')),
      );
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Selectează cel puțin o alarmă!')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Setare Alarmă Multiplă'),
        backgroundColor: Colors.blue,
      ),
      body: Consumer<AlarmProvider>(
        builder: (context, alarmProvider, _) {
          return SingleChildScrollView(
            child: Padding(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                children: [
                  // Afișare status BLE
                  Consumer<BleProvider>(
                    builder: (context, bleProvider, _) {
                      return Container(
                        padding: const EdgeInsets.all(12),
                        decoration: BoxDecoration(
                          color: bleProvider.isConnected ? Colors.green[100] : Colors.red[100],
                          borderRadius: BorderRadius.circular(8),
                          border: Border.all(
                            color: bleProvider.isConnected ? Colors.green : Colors.red,
                          ),
                        ),
                        child: Row(
                          children: [
                            Icon(
                              bleProvider.isConnected ? Icons.check_circle : Icons.error,
                              color: bleProvider.isConnected ? Colors.green : Colors.red,
                            ),
                            const SizedBox(width: 12),
                            Expanded(
                              child: Text(
                                bleProvider.statusMessage,
                                style: TextStyle(
                                  color: bleProvider.isConnected ? Colors.green[900] : Colors.red[900],
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
                  const SizedBox(height: 20),

                  // Alarmă pentru fiecare zi
                  ..._buildDayCards(),

                  const SizedBox(height: 20),

                  // Buton trimite
                  ElevatedButton.icon(
                    onPressed: () => _sendAlarmsToDevice(context),
                    icon: const Icon(Icons.send),
                    label: const Text('Trimite Alarmele'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.blue,
                      padding: const EdgeInsets.symmetric(
                        horizontal: 32,
                        vertical: 16,
                      ),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  List<Widget> _buildDayCards() {
    return AlarmProvider.days.map((day) {
      return Card(
        margin: const EdgeInsets.only(bottom: 16),
        child: Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                day,
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.blue,
                ),
              ),
              const SizedBox(height: 12),
              ..._buildMomentRows(day),
            ],
          ),
        ),
      );
    }).toList();
  }

  List<Widget> _buildMomentRows(String day) {
    return AlarmProvider.moments.map((moment) {
      return Padding(
        padding: const EdgeInsets.only(bottom: 12),
        child: Row(
          children: [
            Checkbox(
              value: _selectedAlarms[day]![moment],
              onChanged: (value) {
                setState(() {
                  _selectedAlarms[day]![moment] = value ?? false;
                });
              },
            ),
            Expanded(
              child: Text(moment),
            ),
            GestureDetector(
              onTap: () => _selectTime(context, day, moment),
              child: Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 12,
                  vertical: 8,
                ),
                decoration: BoxDecoration(
                  border: Border.all(color: Colors.blue),
                  borderRadius: BorderRadius.circular(4),
                ),
                child: Text(
                  _formatTime(_selectedTimes[day]![moment]!),
                  style: const TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                    color: Colors.blue,
                  ),
                ),
              ),
            ),
          ],
        ),
      );
    }).toList();
  }
}
