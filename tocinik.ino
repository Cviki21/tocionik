// Definicija pinova
#define MICRO_SWITCH_PIN  14   // GPIO14, D5 na NodeMCU - Mikro prekidač za detekciju čaše
#define PUMP_PIN          12   // GPIO12, D6 na NodeMCU - MOSFET za pumpu
#define BUZZER_PIN        13   // GPIO13, D7 na NodeMCU - Piezo zvučnik
#define POT_PIN           A0   // Potenciometar

// Varijable
int potValue = 0;
unsigned long pourDuration = 0; // Trajanje točenja u milisekundama
bool glassDetected = false;
bool previousGlassDetected = false; // Varijabla za praćenje prethodnog stanja
bool pouring = false;
bool serialCommand = false; // Varijabla za praćenje serijske naredbe

void setup() {
  // Inicijalizacija serijske komunikacije
  Serial.begin(115200);
  Serial.println("Inicijalizacija sustava...");

  // Inicijalizacija pinova
  pinMode(MICRO_SWITCH_PIN, INPUT_PULLUP); // Koristimo unutarnji pull-up otpornik
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);

  Serial.println("Sustav spreman.");
}

void loop() {
  // Čitanje vrijednosti potenciometra
  potValue = analogRead(POT_PIN);
  // Mapiranje vrijednosti na vrijeme točenja (npr. 1 do 10 sekundi)
  pourDuration = map(potValue, 0, 1023, 1000, 10000);

  // Provjera prisutnosti čaše (mikro prekidač pritisnut)
  glassDetected = digitalRead(MICRO_SWITCH_PIN) == LOW;

  // Detekcija promjene stanja mikro prekidača (rising edge)
  bool glassJustPlaced = glassDetected && !previousGlassDetected;

  // Provjera serijske komunikacije
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Uklanja eventualne razmake i nove redove

    if (command.equalsIgnoreCase("START")) {
      Serial.println("Primljena naredba: START");
      serialCommand = true;
    } else if (command.equalsIgnoreCase("STOP")) {
      Serial.println("Primljena naredba: STOP");
      serialCommand = false;
      // Ako je pumpa u radu, zaustavi je
      if (pouring) {
        digitalWrite(PUMP_PIN, LOW);
        pouring = false;
        Serial.println("Pumpa zaustavljena preko serijske naredbe.");
      }
    } else {
      Serial.println("Nepoznata naredba: " + command);
    }
  }

  // Ako je čaša upravo postavljena ili je primljena serijska naredba START, i točenje nije već u tijeku
  if ((glassJustPlaced || serialCommand) && !pouring) {
    pouring = true;

    // Zvuk na početku točenja
    tone(BUZZER_PIN, 1000, 200); // 1000 Hz, 200 ms
    delay(200);

    // Pauza od 500 ms
    delay(500);

    // Pokretanje pumpe
    digitalWrite(PUMP_PIN, HIGH);
    unsigned long startTime = millis();

    Serial.println("Pumpa pokrenuta.");
    Serial.print("Trajanje točenja: ");
    Serial.print(pourDuration / 1000.0);
    Serial.println(" sekundi.");

    // Točenje dok ne istekne vrijeme ili dok se čaša ne ukloni (ako je pokrenuto preko mikro prekidača)
    while (millis() - startTime < pourDuration) {
      // Provjera je li čaša još uvijek prisutna (samo ako je pokrenuto preko mikro prekidača)
      if (!serialCommand && digitalRead(MICRO_SWITCH_PIN) == HIGH) { // Mikro prekidač otpušten
        Serial.println("Čaša uklonjena tijekom točenja.");
        break; // Prekid točenja
      }

      // Provjera je li primljena naredba STOP preko serijske veze
      if (serialCommand && Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.equalsIgnoreCase("STOP")) {
          Serial.println("Primljena naredba: STOP");
          serialCommand = false;
          break;
        }
      }

      // Kratka pauza
      delay(10);
    }

    // Zaustavljanje pumpe
    digitalWrite(PUMP_PIN, LOW);

    // Zvuk na kraju točenja
    tone(BUZZER_PIN, 1500, 300); // 1500 Hz, 300 ms
    delay(300);

    Serial.println("Pumpa zaustavljena.");
    pouring = false;
    serialCommand = false; // Resetiranje serijske naredbe
  }

  // Ako se čaša ukloni tijekom točenja (samo ako je pokrenuto preko mikro prekidača)
  if (!glassDetected && pouring && !serialCommand) {
    // Zaustavljanje pumpe
    digitalWrite(PUMP_PIN, LOW);

    // Zvuk upozorenja
    tone(BUZZER_PIN, 2000, 500); // 2000 Hz, 500 ms
    delay(500);

    Serial.println("Pumpa zaustavljena jer je čaša uklonjena.");
    pouring = false;
  }

  // Ažuriranje prethodnog stanja mikro prekidača
  previousGlassDetected = glassDetected;

  // Kratko čekanje prije ponovnog očitavanja
  delay(50);
}
