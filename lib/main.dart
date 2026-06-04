import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:firebase_core/firebase_core.dart';
import 'providers/ble_provider.dart';
import 'providers/history_provider.dart';
import 'providers/alarm_provider.dart';
import 'screens/home_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  bool firebaseOk = false;
  try {
    await Firebase.initializeApp();
    firebaseOk = true;
  } catch (e) {
    debugPrint('Firebase nu a putut fi initializat: $e');
  }

  runApp(PillDispenserApp(firebaseEnabled: firebaseOk));
}

class PillDispenserApp extends StatelessWidget {
  final bool firebaseEnabled;
  const PillDispenserApp({super.key, required this.firebaseEnabled});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => BleProvider()),
        ChangeNotifierProvider(
          create: (_) => HistoryProvider(cloudEnabled: firebaseEnabled),
        ),
        ChangeNotifierProvider(
          create: (_) => AlarmProvider()..initDatabase(),
        ),
      ],
      child: MaterialApp(
        title: 'Pill Dispenser',
        theme: ThemeData(
          primarySwatch: Colors.blue,
          useMaterial3: true,
          colorScheme: ColorScheme.fromSeed(
            seedColor: Colors.blue,
            brightness: Brightness.light,
          ),
        ),
        home: const _AppBridge(child: HomeScreen()),
        debugShowCheckedModeBanner: false,
      ),
    );
  }
}

/// Conectează BleProvider cu HistoryProvider pentru a loga evenimentele de pastile.
class _AppBridge extends StatefulWidget {
  final Widget child;
  const _AppBridge({required this.child});

  @override
  State<_AppBridge> createState() => _AppBridgeState();
}

class _AppBridgeState extends State<_AppBridge> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final ble = context.read<BleProvider>();
      final history = context.read<HistoryProvider>();
      ble.onPillEvent = (message) => history.addEntry(message);
    });
  }

  @override
  Widget build(BuildContext context) => widget.child;
}
