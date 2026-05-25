import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/alarm_provider.dart';
import '../models/alarm.dart';

class ManageAlarmsScreen extends StatefulWidget {
  const ManageAlarmsScreen({super.key});

  @override
  State<ManageAlarmsScreen> createState() => _ManageAlarmsScreenState();
}

class _ManageAlarmsScreenState extends State<ManageAlarmsScreen> {
  @override
  void initState() {
    super.initState();
    final alarmProvider = context.read<AlarmProvider>();
    Future.microtask(() => alarmProvider.loadAlarms());
  }

  Future<void> _editAlarm(BuildContext context, Alarm alarm) async {
    int selectedDayIndex =
        AlarmProvider.days.indexOf(alarm.day).clamp(0, AlarmProvider.days.length - 1);
    int selectedMomentIndex =
        AlarmProvider.moments.indexOf(alarm.moment).clamp(0, AlarmProvider.moments.length - 1);
    TimeOfDay selectedTime = _parseTime(alarm.time);

    await showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setState) => AlertDialog(
          title: const Text('Editare Alarmă'),
          content: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                DropdownButton<int>(
                  value: selectedDayIndex,
                  isExpanded: true,
                  items: List.generate(
                    AlarmProvider.daysDisplay.length,
                    (i) => DropdownMenuItem(
                      value: i,
                      child: Text(AlarmProvider.daysDisplay[i]),
                    ),
                  ),
                  onChanged: (value) {
                    setState(() => selectedDayIndex = value!);
                  },
                ),
                const SizedBox(height: 16),

                DropdownButton<int>(
                  value: selectedMomentIndex,
                  isExpanded: true,
                  items: List.generate(
                    AlarmProvider.momentsDisplay.length,
                    (i) => DropdownMenuItem(
                      value: i,
                      child: Text(AlarmProvider.momentsDisplay[i]),
                    ),
                  ),
                  onChanged: (value) {
                    setState(() => selectedMomentIndex = value!);
                  },
                ),
                const SizedBox(height: 16),

                GestureDetector(
                  onTap: () async {
                    final TimeOfDay? picked = await showTimePicker(
                      context: context,
                      initialTime: selectedTime,
                    );
                    if (picked != null) {
                      setState(() => selectedTime = picked);
                    }
                  },
                  child: Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      border: Border.all(color: Colors.blue),
                      borderRadius: BorderRadius.circular(4),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(
                          '${selectedTime.hour.toString().padLeft(2, '0')}:${selectedTime.minute.toString().padLeft(2, '0')}',
                          style: const TextStyle(
                              fontSize: 16, fontWeight: FontWeight.bold),
                        ),
                        const Icon(Icons.access_time),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Anulare'),
            ),
            ElevatedButton(
              onPressed: () {
                final time =
                    '${selectedTime.hour.toString().padLeft(2, '0')}:${selectedTime.minute.toString().padLeft(2, '0')}';
                context.read<AlarmProvider>().updateAlarm(
                      alarm.id!,
                      AlarmProvider.days[selectedDayIndex],
                      AlarmProvider.moments[selectedMomentIndex],
                      time,
                      alarm.active,
                    );
                Navigator.pop(context);
              },
              child: const Text('Salvare'),
            ),
          ],
        ),
      ),
    );
  }

  TimeOfDay _parseTime(String time) {
    final parts = time.split(':');
    return TimeOfDay(
      hour: int.parse(parts[0]),
      minute: int.parse(parts[1]),
    );
  }

  void _deleteAlarm(BuildContext context, int id) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Ștergere Alarmă'),
        content: const Text('Ești sigur că vrei să ștergi această alarmă?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Anulare'),
          ),
          ElevatedButton(
            onPressed: () {
              context.read<AlarmProvider>().deleteAlarm(id);
              Navigator.pop(context);
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Șterge'),
          ),
        ],
      ),
    );
  }

  String _getDisplayDay(String day) {
    final index = AlarmProvider.days.indexOf(day);
    return index >= 0 ? AlarmProvider.daysDisplay[index] : day;
  }

  String _getDisplayMoment(String moment) {
    final index = AlarmProvider.moments.indexOf(moment);
    return index >= 0 ? AlarmProvider.momentsDisplay[index] : moment;
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<AlarmProvider>(
      builder: (context, alarmProvider, _) {
        if (alarmProvider.isLoading) {
          return const Center(child: CircularProgressIndicator());
        }

        if (alarmProvider.alarms.isEmpty) {
          return Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(Icons.alarm_off, size: 64, color: Colors.grey[400]),
                const SizedBox(height: 16),
                Text(
                  'Nu ai nicio alarmă setată',
                  style: TextStyle(fontSize: 16, color: Colors.grey[600]),
                ),
              ],
            ),
          );
        }

        return ListView.builder(
          itemCount: alarmProvider.alarms.length,
          itemBuilder: (context, index) {
            final alarm = alarmProvider.alarms[index];
            return Card(
              margin: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
              child: ListTile(
                leading: Checkbox(
                  value: alarm.active,
                  onChanged: (value) {
                    alarmProvider.toggleAlarm(alarm.id!, value ?? false);
                  },
                ),
                title: Text(
                  '${_getDisplayDay(alarm.day)} - ${_getDisplayMoment(alarm.moment)} - ${alarm.time}',
                  style: TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                    color: alarm.active ? Colors.black : Colors.grey,
                    decoration: alarm.active
                        ? TextDecoration.none
                        : TextDecoration.lineThrough,
                  ),
                ),
                subtitle: Text(
                  'Creat: ${alarm.createdAt.day}/${alarm.createdAt.month}/${alarm.createdAt.year}',
                  style: TextStyle(color: Colors.grey[600]),
                ),
                trailing: PopupMenuButton(
                  itemBuilder: (context) => [
                    PopupMenuItem(
                      child: const Row(
                        children: [
                          Icon(Icons.edit, color: Colors.blue),
                          SizedBox(width: 8),
                          Text('Editare'),
                        ],
                      ),
                      onTap: () => _editAlarm(context, alarm),
                    ),
                    PopupMenuItem(
                      child: const Row(
                        children: [
                          Icon(Icons.delete, color: Colors.red),
                          SizedBox(width: 8),
                          Text('Ștergere'),
                        ],
                      ),
                      onTap: () => _deleteAlarm(context, alarm.id!),
                    ),
                  ],
                ),
              ),
            );
          },
        );
      },
    );
  }
}
