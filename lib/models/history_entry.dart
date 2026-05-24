/// Model pentru o intrare din istoric
class HistoryEntry {
  final int? id;
  final String message;
  final DateTime timestamp;

  HistoryEntry({
    this.id,
    required this.message,
    required this.timestamp,
  });

  /// Convertește obiectul în Map pentru baza de date
  Map<String, dynamic> toMap() {
    return {
      'id': id,
      'message': message,
      'timestamp': timestamp.toIso8601String(),
    };
  }

  /// Creează un obiect din Map (din baza de date)
  factory HistoryEntry.fromMap(Map<String, dynamic> map) {
    return HistoryEntry(
      id: map['id'] as int?,
      message: map['message'] as String,
      timestamp: DateTime.parse(map['timestamp'] as String),
    );
  }

  /// Formatează data și ora pentru afișare
  String get formattedDateTime {
    return '${timestamp.day.toString().padLeft(2, '0')}/'
        '${timestamp.month.toString().padLeft(2, '0')}/'
        '${timestamp.year} '
        '${timestamp.hour.toString().padLeft(2, '0')}:'
        '${timestamp.minute.toString().padLeft(2, '0')}';
  }
}
