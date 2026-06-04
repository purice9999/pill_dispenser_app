import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/history_entry.dart';
import '../providers/history_provider.dart';

class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});

  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  @override
  Widget build(BuildContext context) {
    return Consumer<HistoryProvider>(
      builder: (context, hp, _) {
        if (!hp.isInitialized) {
          return const Center(child: CircularProgressIndicator());
        }

        return Column(
          children: [
            // ─── COD PACIENT + CLOUD STATUS ──────────────────────────────
            if (hp.cloudEnabled)
              Padding(
                padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
                child: Card(
                  color: Colors.blue.shade50,
                  child: Padding(
                    padding: const EdgeInsets.all(10),
                    child: Row(
                      children: [
                        const Icon(Icons.cloud_done, color: Colors.blue, size: 20),
                        const SizedBox(width: 8),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              const Text(
                                'Sincronizat cu cloud',
                                style: TextStyle(
                                  fontWeight: FontWeight.bold,
                                  color: Colors.blue,
                                  fontSize: 12,
                                ),
                              ),
                              Text(
                                'Cod pacient: ${hp.patientId}',
                                style: TextStyle(
                                  fontSize: 11,
                                  color: Colors.blue.shade700,
                                ),
                              ),
                            ],
                          ),
                        ),
                        IconButton(
                          icon: const Icon(Icons.edit, size: 18, color: Colors.blue),
                          tooltip: 'Schimbă cod pacient',
                          onPressed: () => _showChangePatientId(context, hp),
                        ),
                      ],
                    ),
                  ),
                ),
              ),

            // ─── BUTOANE CONTROL ────────────────────────────────────────
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 8, 16, 0),
              child: Row(
                children: [
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: hp.history.isEmpty
                          ? null
                          : () => _showClearConfirmation(context, hp),
                      icon: const Icon(Icons.delete, size: 18),
                      label: const Text('Resetare'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.red,
                        disabledBackgroundColor: Colors.grey,
                      ),
                    ),
                  ),
                ],
              ),
            ),

            // ─── LISTA ISTORIC ───────────────────────────────────────────
            Expanded(
              child: hp.history.isEmpty
                  ? Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(Icons.history, size: 64, color: Colors.grey.shade300),
                          const SizedBox(height: 16),
                          Text(
                            'Niciun eveniment înregistrat',
                            style: TextStyle(color: Colors.grey.shade600, fontSize: 16),
                          ),
                          if (hp.cloudEnabled) ...[
                            const SizedBox(height: 8),
                            Text(
                              'Evenimentele apar automat\ncând pacientul ia/nu ia pastila',
                              textAlign: TextAlign.center,
                              style: TextStyle(color: Colors.grey.shade500, fontSize: 13),
                            ),
                          ],
                        ],
                      ),
                    )
                  : ListView.builder(
                      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                      itemCount: hp.history.length,
                      itemBuilder: (context, index) =>
                          _buildHistoryCard(context, hp.history[index], hp),
                    ),
            ),
          ],
        );
      },
    );
  }

  Widget _buildHistoryCard(
    BuildContext context,
    HistoryEntry entry,
    HistoryProvider hp,
  ) {
    final taken = !entry.message.toLowerCase().contains('nu a fost');
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 6),
      color: taken ? Colors.green.shade50 : Colors.red.shade50,
      child: ListTile(
        leading: Icon(
          taken ? Icons.check_circle : Icons.cancel,
          color: taken ? Colors.green : Colors.red,
        ),
        title: Text(
          entry.message,
          style: TextStyle(
            fontWeight: FontWeight.w600,
            color: taken ? Colors.green.shade800 : Colors.red.shade800,
          ),
        ),
        subtitle: Text(entry.formattedDateTime, style: const TextStyle(fontSize: 12)),
        trailing: IconButton(
          icon: const Icon(Icons.delete, color: Colors.red),
          onPressed: () => _showDeleteConfirmation(context, entry, hp),
        ),
      ),
    );
  }

  void _showDeleteConfirmation(
    BuildContext context,
    HistoryEntry entry,
    HistoryProvider hp,
  ) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Ștergere Intrare'),
        content: const Text('Ești sigur că vrei să ștergi această intrare?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Anulare'),
          ),
          TextButton(
            onPressed: () {
              hp.deleteEntry(entry.id);
              Navigator.pop(ctx);
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Intrare ștearsă')),
              );
            },
            child: const Text('Ștergere', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );
  }

  void _showClearConfirmation(BuildContext context, HistoryProvider hp) {
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Resetare Istoric'),
        content: const Text('Ștergi doar istoricul local. Datele din cloud rămân.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Anulare'),
          ),
          TextButton(
            onPressed: () {
              hp.clearHistory();
              Navigator.pop(ctx);
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Istoric șters')),
              );
            },
            child: const Text('Ștergere', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );
  }

  /// Dialog pentru schimbarea codului pacientului (pentru membri familie)
  void _showChangePatientId(BuildContext context, HistoryProvider hp) {
    final controller = TextEditingController(text: hp.patientId);
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Cod Pacient'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Introdu codul pacientului pentru a vedea istoricul acestuia.',
              style: TextStyle(fontSize: 13),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: controller,
              decoration: const InputDecoration(
                labelText: 'Cod pacient',
                border: OutlineInputBorder(),
                hintText: 'ex: 1234567890',
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Anulare'),
          ),
          ElevatedButton(
            onPressed: () async {
              Navigator.pop(ctx);
              await hp.setPatientId(controller.text);
              if (context.mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('Cod actualizat, se sincronizează...')),
                );
              }
            },
            child: const Text('Salvare'),
          ),
        ],
      ),
    );
  }
}
