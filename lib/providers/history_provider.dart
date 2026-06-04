import 'package:flutter/foundation.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import '../models/history_entry.dart';

class HistoryProvider extends ChangeNotifier {
  Database? _database;
  List<HistoryEntry> _history = [];
  bool _isInitialized = false;
  final bool _cloudEnabled;

  List<HistoryEntry> get history => _history;
  bool get isInitialized => _isInitialized;

  HistoryProvider({bool cloudEnabled = false}) : _cloudEnabled = cloudEnabled {
    _initializeDatabase();
  }

  Future<void> _initializeDatabase() async {
    try {
      final databasePath = await getDatabasesPath();
      final path = join(databasePath, 'history.db');

      _database = await openDatabase(
        path,
        version: 1,
        onCreate: (db, version) async {
          await db.execute('''
            CREATE TABLE IF NOT EXISTS history (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              message TEXT NOT NULL,
              timestamp TEXT NOT NULL,
              created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
          ''');
        },
      );

      _isInitialized = true;
      await _loadHistory();
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

      // Salveaza in cloud (in fundal, fara sa blocheze UI)
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

  // Salveaza evenimentul in Firestore — colectia "history", vizibila pentru familie
  Future<void> _saveToCloud(String message, DateTime timestamp) async {
    try {
      await FirebaseFirestore.instance.collection('history').add({
        'message': message,
        'timestamp': Timestamp.fromDate(timestamp),
        'type': message.contains('nu a fost') ? 'neluata' : 'luata',
        'device': 'PillDispenser',
      });
    } catch (e) {
      debugPrint('Cloud save esuat: $e');
    }
  }

  @override
  void dispose() {
    _database?.close();
    super.dispose();
  }
}
