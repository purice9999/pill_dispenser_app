/// Model pentru o alarmă
class Alarm {
  final int? id;
  final String day;           // Luni, Marti, Miercuri, Joi, Vineri, Sambata, Duminica
  final String moment;        // Dimineata, Pranz, Seara
  final String time;          // HH:MM format
  final bool active;          // true = activ, false = inactiv
  final DateTime createdAt;

  Alarm({
    this.id,
    required this.day,
    required this.moment,
    required this.time,
    required this.active,
    required this.createdAt,
  });

  /// Convertește obiectul în Map pentru baza de date
  Map<String, dynamic> toMap() {
    return {
      'id': id,
      'day': day,
      'moment': moment,
      'time': time,
      'active': active ? 1 : 0,
      'created_at': createdAt.toIso8601String(),
    };
  }

  /// Creează un obiect din Map (din baza de date)
  factory Alarm.fromMap(Map<String, dynamic> map) {
    return Alarm(
      id: map['id'] as int?,
      day: map['day'] as String,
      moment: map['moment'] as String,
      time: map['time'] as String,
      active: (map['active'] as int) == 1,
      createdAt: DateTime.parse(map['created_at'] as String),
    );
  }

  /// Creează o copie cu câmpuri modificate
  Alarm copyWith({
    int? id,
    String? day,
    String? moment,
    String? time,
    bool? active,
    DateTime? createdAt,
  }) {
    return Alarm(
      id: id ?? this.id,
      day: day ?? this.day,
      moment: moment ?? this.moment,
      time: time ?? this.time,
      active: active ?? this.active,
      createdAt: createdAt ?? this.createdAt,
    );
  }

  /// Formatează alarmă pentru afișare
  String get displayText => '$day - $moment - $time';

  /// Formatează pentru trimitere la PIC
  String get commandText => '$day $moment $time';

  @override
  String toString() => 'Alarm(id: $id, day: $day, moment: $moment, time: $time, active: $active)';

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is Alarm &&
          runtimeType == other.runtimeType &&
          id == other.id &&
          day == other.day &&
          moment == other.moment &&
          time == other.time;

  @override
  int get hashCode => id.hashCode ^ day.hashCode ^ moment.hashCode ^ time.hashCode;
}
