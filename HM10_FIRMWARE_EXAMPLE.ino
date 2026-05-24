/*
 * EXEMPLU: Cod pentru microcontroller (Arduino/PIC) care primește comenzi de la HM-10
 * 
 * Fluxul:
 * 1. Aplicația Flutter trimite: "Luni Dimineata 08:30"
 * 2. HM-10 primește și transmite la microcontroller prin UART
 * 3. Microcontroller parsează comanda și activează dozatorul
 * 4. Microcontroller trimite confirmare: "Pastila luata"
 * 5. HM-10 transmite confirmare înapoi la aplicație
 * 
 * Conexiuni:
 * HM-10 TX → Arduino RX (pin 0)
 * HM-10 RX → Arduino TX (pin 1)
 * HM-10 GND → Arduino GND
 * HM-10 VCC → 3.3V (cu voltage divider pe RX!)
 */

#include <SoftwareSerial.h>

// Creează serial software pentru HM-10 (RX, TX)
SoftwareSerial bleSerial(10, 11); // RX pe pin 10, TX pe pin 11

// Pini pentru motor/solenoid
const int MOTOR_PIN = 5;
const int LED_PIN = 13;

// Buffer pentru comenzi primite
String receivedCommand = "";

void setup() {
  // Serial pentru debugging
  Serial.begin(9600);
  
  // Serial pentru HM-10 (9600 baud)
  bleSerial.begin(9600);
  
  // Configurare pini
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Sistem pornit - Asteapta comenzi BLE...");
}

void loop() {
  // Citește date de la HM-10
  if (bleSerial.available()) {
    char c = bleSerial.read();
    receivedCommand += c;
    
    // Dacă primim newline, procesează comanda
    if (c == '\n') {
      processCommand(receivedCommand);
      receivedCommand = ""; // Resetează buffer
    }
  }
}

/*
 * Procesează comanda primită
 * Format: "ZI MOMENT ORA"
 * Exemplu: "Luni Dimineata 08:30"
 */
void processCommand(String command) {
  command.trim(); // Elimină spații
  
  Serial.print("Comanda primita: ");
  Serial.println(command);
  
  // Parsează comanda
  int firstSpace = command.indexOf(' ');
  int secondSpace = command.indexOf(' ', firstSpace + 1);
  
  if (firstSpace == -1 || secondSpace == -1) {
    sendResponse("Eroare: Format invalid");
    return;
  }
  
  String day = command.substring(0, firstSpace);
  String moment = command.substring(firstSpace + 1, secondSpace);
  String time = command.substring(secondSpace + 1);
  
  Serial.print("Zi: ");
  Serial.println(day);
  Serial.print("Moment: ");
  Serial.println(moment);
  Serial.print("Ora: ");
  Serial.println(time);
  
  // Activează dozatorul
  activateDispenser();
  
  // Trimite confirmare
  String response = "Pastila luata - " + day + " " + moment + " " + time;
  sendResponse(response);
}

/*
 * Activează motorul/solenoidul pentru a elibera pastila
 */
void activateDispenser() {
  Serial.println("Activez dozatorul...");
  
  // Aprinde LED
  digitalWrite(LED_PIN, HIGH);
  
  // Activează motor (PWM)
  analogWrite(MOTOR_PIN, 255); // Putere maximă
  delay(1000); // Ține 1 secundă
  
  // Oprește motor
  analogWrite(MOTOR_PIN, 0);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Dozator oprit");
}

/*
 * Trimite răspuns înapoi la aplicație prin HM-10
 */
void sendResponse(String response) {
  Serial.print("Raspuns: ");
  Serial.println(response);
  
  // Trimite prin BLE
  bleSerial.println(response);
  
  // Trimite și pe serial pentru debugging
  Serial.println("Raspuns trimis la HM-10");
}

/*
 * ALTERNATIVE DE IMPLEMENTARE
 */

// ─── VARIANTA 1: Verificare Oră Exactă ──────────────────────────────────

/*
void processCommandWithTimeCheck(String command) {
  // Parsează comanda
  int firstSpace = command.indexOf(' ');
  int secondSpace = command.indexOf(' ', firstSpace + 1);
  
  String day = command.substring(0, firstSpace);
  String moment = command.substring(firstSpace + 1, secondSpace);
  String time = command.substring(secondSpace + 1);
  
  // Obține ora curentă (necesită RTC module)
  // int currentHour = rtc.getHour();
  // int currentMinute = rtc.getMinute();
  
  // Compară cu ora setată
  // if (currentHour == setHour && currentMinute == setMinute) {
  //   activateDispenser();
  // }
}
*/

// ─── VARIANTA 2: Salvare Alarme în EEPROM ──────────────────────────────

/*
#include <EEPROM.h>

struct Alarm {
  char day[10];
  char moment[10];
  char time[6];
};

void saveAlarm(Alarm alarm, int index) {
  int address = index * sizeof(Alarm);
  EEPROM.put(address, alarm);
}

Alarm loadAlarm(int index) {
  int address = index * sizeof(Alarm);
  Alarm alarm;
  EEPROM.get(address, alarm);
  return alarm;
}
*/

// ─── VARIANTA 3: Suport pentru Multiple Alarme ──────────────────────────

/*
#define MAX_ALARMS 10

struct Alarm {
  String day;
  String moment;
  String time;
  bool active;
};

Alarm alarms[MAX_ALARMS];
int alarmCount = 0;

void addAlarm(String command) {
  if (alarmCount >= MAX_ALARMS) {
    sendResponse("Eroare: Prea multe alarme");
    return;
  }
  
  // Parsează și salvează
  // ...
  
  alarmCount++;
  sendResponse("Alarma adaugata");
}

void checkAlarms() {
  for (int i = 0; i < alarmCount; i++) {
    if (alarms[i].active) {
      // Verifică dacă e ora
      // ...
    }
  }
}
*/

// ─── VARIANTA 4: Logging în SD Card ─────────────────────────────────────

/*
#include <SD.h>

File logFile;

void logEvent(String event) {
  logFile = SD.open("log.txt", FILE_WRITE);
  if (logFile) {
    logFile.println(event);
    logFile.close();
  }
}
*/
