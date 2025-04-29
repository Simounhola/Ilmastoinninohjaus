import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';
import 'firebase_options.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'ESP32 Ohjain',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.teal),
        useMaterial3: true,
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final DatabaseReference db = FirebaseDatabase.instance.ref();

  String lampotila = "-";
  String moodi = "-";
  String puhallus = "-";
  String aste = "-";
  bool laitePaalla = false;
  bool automaatioPaalla = true;

  @override
  void initState() {
    super.initState();

    db.child("sensorit/lampotila").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          lampotila = value.toString();
        });
      }
    });

    db.child("tila/paalla").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          laitePaalla = value == true;
        });
      }
    });

    db.child("tila/moodi").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          moodi = value.toString();
        });
      }
    });

    db.child("tila/puhallus").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          puhallus = value.toString();
        });
      }
    });

    db.child("tila/lampotila").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          aste = value.toString();
        });
      }
    });

    db.child("tila/automaattinen").onValue.listen((event) {
      final value = event.snapshot.value;
      if (value != null) {
        setState(() {
          automaatioPaalla = value == true;
        });
      }
    });
  }

  void lahetaKomento(String komento) {
    final aika = DateTime.now().toIso8601String();
    db.child("komento").set({"viesti": komento, "timestamp": aika});
    debugPrint(" Lähetettiin: $komento @ $aika");
  }

  void paivitaAutomaatio(bool arvo) {
    db.child("tila/automaattinen").set(arvo);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('AC KAUKOSÄÄDIN')),
      body: Center(
        child: SingleChildScrollView(
          child: Padding(
            padding: const EdgeInsets.all(20.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const SizedBox(height: 20),
                Text(
                  " Lämpötila: $lampotila °C ",
                  style: const TextStyle(fontSize: 25),
                ),
                Text(" Tila: $moodi", style: const TextStyle(fontSize: 20)),
                Text(
                  " Nopeus: $puhallus",
                  style: const TextStyle(fontSize: 20),
                ),
                Text(" Asteet: $aste °C", style: const TextStyle(fontSize: 20)),
                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    backgroundColor: laitePaalla ? Colors.green : Colors.red,
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () => lahetaKomento("virtakytkin"),
                  child: const Text('Virtakytkin'),
                ),

                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () => lahetaKomento("lämpötila_plus"),
                  child: const Text('Lämpötila +'),
                ),

                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () => lahetaKomento("lämpötila_miinus"),
                  child: const Text('Lämpötila -'),
                ),

                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () => lahetaKomento("puhallus"),
                  child: const Text('Nopeus +-'),
                ),

                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () => lahetaKomento("tila"),
                  child: const Text('Tilan vaihto'),
                ),

                const SizedBox(height: 25),

                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size(250, 60),
                    backgroundColor:
                        automaatioPaalla ? Colors.teal : Colors.grey,
                    textStyle: const TextStyle(fontSize: 22),
                  ),
                  onPressed: () {
                    final uusiTila = !automaatioPaalla;
                    paivitaAutomaatio(uusiTila);
                  },
                  child: Text(
                    automaatioPaalla
                        ? 'Automaatio: PÄÄLLÄ'
                        : 'Automaatio: POIS',
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
