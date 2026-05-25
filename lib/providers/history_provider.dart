import 'package:flutter/foundation.dart';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';
import '../models/history_entry.dart';

/// Provider pentru gestionarea istoricului pastilelor
class HistoryProvider extends ChangeNotifier {
  Database? _database;
  List<HistoryEntry> _history = [];
  bool _isInitialized = false;

  // ─── GETTERS ────────────────────────────────────────────────────────────

  List<HistoryEntry> get history => _history;
  bool get isInitialized => _isInitialized;

  // ─── CONSTRUCTOR ────────────────────────────────────────────────────────

  HistoryProvider() {
    _initializeDatabase();
  }

  // ─── METODE PUBLICE ─────────────────────────────────────────────────────

  /// Inițializează baza de date
  Future<void> _initializeDatabase() async {
    try {
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
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare inițializare bază de date: $e');
      _isInitialized = true; // Mark as initialized even on error
      notifyListeners();
    }
  }

  /// Încarcă istoricul din baza de date
  Future<void> _loadHistory() async {
    try {
      if (_database == null) return;

      final List<Map<String, dynamic>> maps = await _database!.query(
        'history',
        orderBy: 'created_at DESC',
      );

      _history = List.generate(
        maps.length,
        (i) => HistoryEntry.fromMap(maps[i]),
      );

      notifyListeners();
    } catch (e) {
      debugPrint('Eroare încărcare istoric: $e');
    }
  }

  /// Adaugă o intrare în istoric
  Future<void> addEntry(String message) async {
    try {
      if (_database == null) return;

      final entry = HistoryEntry(
        message: message,
        timestamp: DateTime.now(),
      );

      await _database!.insert(
        'history',
        entry.toMap(),
        conflictAlgorithm: ConflictAlgorithm.replace,
      );

      _history.insert(0, entry);
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare adăugare intrare: $e');
    }
  }

  /// Șterge întregul istoric
  Future<void> clearHistory() async {
    try {
      if (_database == null) return;

      await _database!.delete('history');
      _history.clear();
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare ștergere istoric: $e');
    }
  }

  /// Șterge o intrare specifică
  Future<void> deleteEntry(int id) async {
    try {
      if (_database == null) return;

      await _database!.delete(
        'history',
        where: 'id = ?',
        whereArgs: [id],
      );

      _history.removeWhere((entry) => entry.id == id);
      notifyListeners();
    } catch (e) {
      debugPrint('Eroare ștergere intrare: $e');
    }
  }

  @override
  void dispose() {
    _database?.close();
    super.dispose();
  }
}
