import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ble_provider.dart';
import '../providers/alarm_provider.dart';
import 'set_alarm_screen.dart';
import 'history_screen.dart';
import 'alarms_screen.dart';
import 'manage_alarms_screen.dart';

/// Ecranul principal cu navigare între SetAlarm și History
class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;

  final List<Widget> _screens = [
    const SetAlarmScreen(),
    const HistoryScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dozator de Pastile'),
        elevation: 0,
        centerTitle: true,
      ),
      body: _screens[_selectedIndex],
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _selectedIndex,
        onTap: (index) {
          setState(() {
            _selectedIndex = index;
          });
        },
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.alarm),
            label: 'Setare Alarmă',
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
