import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/history_entry.dart';
import '../providers/history_provider.dart';

/// Ecranul pentru vizualizarea istoricului pastilelor
class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});

  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  @override
  Widget build(BuildContext context) {
    return Consumer<HistoryProvider>(
      builder: (context, historyProvider, _) {
        if (!historyProvider.isInitialized) {
          return const Center(
            child: CircularProgressIndicator(),
          );
        }

        return Column(
          children: [
            // ─── BUTON RESETARE ─────────────────────────────────────────
            Padding(
              padding: const EdgeInsets.all(16.0),
              child: ElevatedButton.icon(
                onPressed: historyProvider.history.isEmpty
                    ? null
                    : () => _showClearConfirmation(context, historyProvider),
                icon: const Icon(Icons.delete),
                label: const Text('Resetare Istoric'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.red,
                  disabledBackgroundColor: Colors.grey,
                ),
              ),
            ),

            // ─── LISTA ISTORIC ───────────────────────────────────────────
            Expanded(
              child: historyProvider.history.isEmpty
                  ? Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(
                            Icons.history,
                            size: 64,
                            color: Colors.grey.shade300,
                          ),
                          const SizedBox(height: 16),
                          Text(
                            'Niciun istoric disponibil',
                            style: TextStyle(
                              color: Colors.grey.shade600,
                              fontSize: 16,
                            ),
                          ),
                        ],
                      ),
                    )
                  : ListView.builder(
                      padding: const EdgeInsets.symmetric(horizontal: 16),
                      itemCount: historyProvider.history.length,
                      itemBuilder: (context, index) {
                        final entry = historyProvider.history[index];
                        return _buildHistoryCard(
                          context,
                          entry,
                          historyProvider,
                        );
                      },
                    ),
            ),
          ],
        );
      },
    );
  }

  /// Widget pentru o intrare din istoric
  Widget _buildHistoryCard(
    BuildContext context,
    HistoryEntry entry,
    HistoryProvider historyProvider,
  ) {
    final taken = !entry.message.toLowerCase().contains('nu a fost');
    return Card(
      margin: const EdgeInsets.symmetric(vertical: 8),
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
        subtitle: Text(
          entry.formattedDateTime,
          style: const TextStyle(fontSize: 12),
        ),
        trailing: IconButton(
          icon: const Icon(Icons.delete, color: Colors.red),
          onPressed: () {
            _showDeleteConfirmation(context, entry, historyProvider);
          },
        ),
      ),
    );
  }

  /// Dialog pentru confirmare ștergere intrare
  void _showDeleteConfirmation(
    BuildContext context,
    dynamic entry,
    HistoryProvider historyProvider,
  ) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Ștergere Intrare'),
        content: const Text('Ești sigur că vrei să ștergi această intrare?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Anulare'),
          ),
          TextButton(
            onPressed: () {
              historyProvider.deleteEntry(entry.id);
              Navigator.pop(context);
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

  /// Dialog pentru confirmare ștergere istoric complet
  void _showClearConfirmation(
    BuildContext context,
    HistoryProvider historyProvider,
  ) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Resetare Istoric'),
        content: const Text(
          'Ești sigur că vrei să ștergi întregul istoric? Această acțiune nu poate fi anulată.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Anulare'),
          ),
          TextButton(
            onPressed: () {
              historyProvider.clearHistory();
              Navigator.pop(context);
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
}
