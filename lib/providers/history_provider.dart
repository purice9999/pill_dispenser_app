import 'package:flutter/foundation.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/history_entry.dart';

class HistoryProvider extends ChangeNotifier {
  Database? _database;
  List<HistoryEntry> _history = [];
  bool _isInitialized = false;
  final bool _cloudEnabled;

  // ID unic al pacientului (generat la prima instalare, persistent)
  String _patientId = '';

  List<HistoryEntry> get history => _history;
  bool get isInitialized => _isInitialized;
  String get patientId => _patientId;
  bool get cloudEnabled => _cloudEnabled;

  HistoryProvider({bool cloudEnabled = false}) : _cloudEnabled = cloudEnabled {
    _initializeDatabase();
  }

  // ─── INIT ─────────────────────────────────────────────────────────────────

  Future<void> _initializeDatabase() async {
    try {
      // Incarca sau genereaza patient ID
      final prefs = await SharedPreferences.getInstance();
      _patientId = prefs.getString('patient_id') ?? '';
      if (_patientId.isEmpty) {
        _patientId = DateTime.now().millisecondsSinceEpoch.toString();
        await prefs.setString('patient_id', _patientId);
      }

      final databasePath = await getDatabasesPath();
      final path = join(databasePath, 'history.db');

      _database = await openDatabase(
        path,
        version: 1,
        onCreate: (db, version) async {
          await db.execute(
            '''
            CREATE TABLE IF NOT EXISTS history (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              message TEXT NOT NULL,
              timestamp TEXT NOT NULL,
              created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
            ''',
          );
        },
      );

      _isInitialized = true;
      await _loadHistory();

      // Sincronizeaza din cloud in fundal (fara sa blocheze UI)
      if (_cloudEnabled) _syncFromCloud();

      notifyListeners();
    } catch (e) {
      debugPrint('Eroare initializare baza de date: $e');
      _isInitialized = true;
      notifyListeners();
    }
  }

  Future<void> _loadHistory() async {
    try {
      if (_database == null) return;
      final maps = await _database!.query('history', orderBy: 'created_at DESC');
      _history = maps.map((m) => HistoryEntry.fromMap(m)).toList();
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare incarcare istoric: $e');
    }
  }

  // ─── METODE PUBLICE ────────────────────────────────────────────────────────

  Future<void> addEntry(String message) async {
    try {
      if (_database == null) return;

      // Deduplicare: ignora daca acelasi mesaj a aparut in ultimele 60s
      final now = DateTime.now();
      final isDuplicate = _history.any((e) =>
          e.message == message &&
          now.difference(e.timestamp).inSeconds.abs() < 60,
      );
      if (isDuplicate) return;

      // Salveaza local
      final entry = HistoryEntry(message: message, timestamp: now);
      final id = await _database!.insert(
        'history',
        entry.toMap(),
        conflictAlgorithm: ConflictAlgorithm.replace,
      );
      _history.insert(0, HistoryEntry(id: id, message: message, timestamp: now));
      notifyListeners();

      // Salveaza in cloud
      if (_cloudEnabled) _saveToCloud(message, now);
    } catch (e) {
      debugPrint('Eroare adaugare intrare: $e');
    }
  }

  Future<void> clearHistory() async {
    try {
      if (_database == null) return;
      await _database!.delete('history');
      _history.clear();
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare stergere istoric: $e');
    }
  }

  Future<void> deleteEntry(int? id) async {
    if (id == null) return;
    try {
      if (_database == null) return;
      await _database!.delete('history', where: 'id = ?', whereArgs: [id]);
      _history.removeWhere((e) => e.id == id);
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare stergere intrare: $e');
    }
  }

  // ─── FIREBASE ──────────────────────────────────────────────────────────────

  Future<void> _saveToCloud(String message, DateTime timestamp) async {
    try {
      await FirebaseFirestore.instance
          .collection('patients')
          .doc(_patientId)
          .collection('history')
          .add({
        'message': message,
        'timestamp': Timestamp.fromDate(timestamp),
        'type': message.contains('nu a fost') ? 'neluata' : 'luata',
      });
    } catch (e) {
      debugPrint('Cloud save esuat: $e');
    }
  }

  /// Sincronizeaza din Firestore → SQLite local.
  /// Folosit si de telefonul familiei care nu are BLE.
  Future<void> _syncFromCloud() async {
    try {
      final snapshot = await FirebaseFirestore.instance
          .collection('patients')
          .doc(_patientId)
          .collection('history')
          .orderBy('timestamp', descending: true)
          .limit(200)
          .get();

      bool changed = false;
      for (final doc in snapshot.docs) {
        final msg = doc['message'] as String? ?? '';
        final ts = (doc['timestamp'] as Timestamp?)?.toDate() ?? DateTime.now();

        final exists = _history.any((e) =>
            e.message == msg &&
            e.timestamp.difference(ts).inSeconds.abs() < 5);
        if (exists) continue;

        if (_database != null) {
          final id = await _database!.insert(
            'history',
            HistoryEntry(message: msg, timestamp: ts).toMap(),
            conflictAlgorithm: ConflictAlgorithm.ignore,
          );
          _history.add(HistoryEntry(id: id, message: msg, timestamp: ts));
          changed = true;
        }
      }

      if (changed) {
        _history.sort((a, b) => b.timestamp.compareTo(a.timestamp));
        notifyListeners();
      }
    } catch (e) {
      debugPrint('Cloud sync esuat: $e');
    }
  }

  /// Apelat din ecranul familiei pentru a schimba patient ID si sincroniza.
  Future<void> setPatientId(String newId) async {
    if (newId.trim().isEmpty) return;
    _patientId = newId.trim();
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('patient_id', _patientId);
    _history.clear();
    await _loadHistory();
    if (_cloudEnabled) await _syncFromCloud();
  }

  @override
  void dispose() {
    _database?.close();
    super.dispose();
  }
}
