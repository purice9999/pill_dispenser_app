import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/alarm_provider.dart';
import 'set_alarm_screen.dart';
import 'history_screen.dart';
import 'alarms_screen.dart';
import 'manage_alarms_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;

  final List<Widget> _screens = [
    const SetAlarmScreen(),
    const AlarmsScreen(),
    const ManageAlarmsScreen(),
    const HistoryScreen(),
  ];

  void _showDeleteAllAlarmsDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Ștergere Toate'),
        content: const Text('Ești sigur că vrei să ștergi TOATE alarmele?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Anulare'),
          ),
          ElevatedButton(
            onPressed: () {
              context.read<AlarmProvider>().deleteAllAlarms();
              Navigator.pop(context);
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            child: const Text('Șterge Toate'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dozator de Pastile'),
        elevation: 0,
        centerTitle: true,
      ),
      body: _screens[_selectedIndex],
      floatingActionButton: _selectedIndex == 2
          ? FloatingActionButton(
              backgroundColor: Colors.red,
              tooltip: 'Șterge toate alarmele',
              onPressed: () => _showDeleteAllAlarmsDialog(context),
              child: const Icon(Icons.delete_sweep),
            )
          : null,
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _selectedIndex,
        onTap: (index) {
          setState(() {
            _selectedIndex = index;
          });
        },
        type: BottomNavigationBarType.fixed,
        backgroundColor: Colors.white,
        selectedItemColor: Colors.blue,
        unselectedItemColor: Colors.grey,
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.alarm),
            label: 'Setare',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.alarm_add),
            label: 'Alarmă Multiplă',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.edit),
            label: 'Gestionare',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.history),
            label: 'Istoric',
          ),
        ],
      ),
    );
  }
}
