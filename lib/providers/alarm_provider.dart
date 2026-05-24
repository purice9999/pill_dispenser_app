import 'package:flutter/foundation.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import '../models/alarm.dart';

class AlarmProvider extends ChangeNotifier {
  Database? _database;
  List<Alarm> _alarms = [];
  bool _isLoading = false;

  List<Alarm> get alarms => _alarms;
  bool get isLoading => _isLoading;

  // Zile ale săptămânii
  static const List<String> days = [
    'Luni',
    'Marti',
    Miercuri',
    'Joi',
    'Vineri',
    'Sambata',
    'Duminica'
  ];

  // Momente ale zilei
  static const List<String> moments = ['Dimineata', 'Pranz', 'Seara'];

  /// Inițializează baza de date
  Future<void> initDatabase() async {
    try {
      _isLoading = true;
      notifyListeners();

      final databasePath = await getDatabasesPath();
      final path = join(databasePath, 'pill_dispenser.db');

      _database = await openDatabase(
        path,
        version: 1,
        onCreate: (db, version) async {
          await db.execute('''
            CREATE TABLE alarms (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              day TEXT NOT NULL,
              moment TEXT NOT NULL,
              time TEXT NOT NULL,
              active INTEGER NOT NULL DEFAULT 1,
              created_at TEXT NOT NULL
            )
          ''');
        },
      );

      await loadAlarms();
      _isLoading = false;
      notifyListeners();
    } catch (e) {
      print('Eroare inițializare bază de date: $e');
      _isLoading = false;
      notifyListeners();
    }
  }

  /// Încarcă toate alarmele din baza de date
  Future<void> loadAlarms() async {
    try {
      if (_database == null) return;

      final maps = await _database!.query('alarms', orderBy: 'day, moment');
      _alarms = maps.map((map) => Alarm.fromMap(map)).toList();
      notifyListeners();
    } catch (e) {
      print('Eroare încărcare alarmă: $e');
    }
  }

  /// Adaugă o nouă alarmă
  Future<void> addAlarm(String day, String moment, String time) async {
    try {
      if (_database == null) return;

      final alarm = Alarm(
        day: day,
        moment: moment,
        time: time,
        active: true,
        createdAt: DateTime.now(),
      );

      final id = await _database!.insert('alarms', alarm.toMap());
      _alarms.add(alarm.copyWith(id: id));
      notifyListeners();
    } catch (e) {
      print('Eroare adăugare alarmă: $e');
    }
  }

  /// Actualizează o alarmă existentă
  Future<void> updateAlarm(int id, String day, String moment, String time, bool active) async {
    try {
      if (_database == null) return;

      final alarm = Alarm(
        id: id,
        day: day,
        moment: moment,
        time: time,
        active: active,
        createdAt: DateTime.now(),
      );

      await _database!.update(
        'alarms',
        alarm.toMap(),
        where: 'id = ?',
        whereArgs: [id],
      );

      final index = _alarms.indexWhere((a) => a.id == id);
      if (index != -1) {
        _alarms[index] = alarm;
      }
      notifyListeners();
    } catch (e) {
      print('Eroare actualizare alarmă: $e');
    }
  }

  /// Șterge o alarmă
  Future<void> deleteAlarm(int id) async {
    try {
      if (_database == null) return;

      await _database!.delete(
        'alarms',
        where: 'id = ?',
        whereArgs: [id],
      );

      _alarms.removeWhere((a) => a.id == id);
      notifyListeners();
    } catch (e) {
      print('Eroare ștergere alarmă: $e');
    }
  }

  /// Activează/Dezactivează o alarmă
  Future<void> toggleAlarm(int id, bool active) async {
    try {
      if (_database == null) return;

      await _database!.update(
        'alarms',
        {'active': active ? 1 : 0},
        where: 'id = ?',
        whereArgs: [id],
      );

      final index = _alarms.indexWhere((a) => a.id == id);
      if (index != -1) {
        _alarms[index] = _alarms[index].copyWith(active: active);
      }
      notifyListeners();
    } catch (e) {
      print('Eroare toggle alarmă: $e');
    }
  }

  /// Șterge toate alarmele
  Future<void> deleteAllAlarms() async {
    try {
      if (_database == null) return;

      await _database!.delete('alarms');
      _alarms.clear();
      notifyListeners();
    } catch (e) {
      print('Eroare ștergere toate alarmele: $e');
    }
  }

  /// Obține alarmele pentru o anumită zi
  List<Alarm> getAlarmsForDay(String day) {
    return _alarms.where((alarm) => alarm.day == day).toList();
  }

  /// Obține alarmele active
  List<Alarm> getActiveAlarms() {
    return _alarms.where((alarm) => alarm.active).toList();
  }

  /// Verifică dacă există o alarmă pentru o anumită zi și moment
  bool hasAlarm(String day, String moment) {
    return _alarms.any((alarm) => alarm.day == day && alarm.moment == moment);
  }

  /// Obține o alarmă specifică
  Alarm? getAlarm(String day, String moment) {
    try {
      return _alarms.firstWhere((alarm) => alarm.day == day && alarm.moment == moment);
    } catch (e) {
      return null;
    }
  }

  @override
  void dispose() {
    _database?.close();
    super.dispose();
  }
}
