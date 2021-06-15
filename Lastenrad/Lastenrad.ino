//Rechtliche Maximalwerte fuer ein Pedelec gemaess StVZO:
#define MAX_PEDELEC_LEISTUNG_WATT 250 //Watt
#define MAX_PEDELEC_GESCHWINDIGKEIT_KMH 25 //km/h
#define MAX_SCHIEBEHILFE_GESCHWINDIGKEIT_KMH 6 //km/h

#define PIN_LEISTUNG 10
#define PIN_GENERATORWIDERSTAND 11
#define PIN_RUECKWAERTS 4
#define PIN_HALL_A 7
#define PIN_HALL_B 2
#define PIN_GEN_HALL_A 3
#define PIN_GEN_HALL_B 12
#define PIN_REKUPERATION 9

#define APIN_GENERATORLEISTUNG 0
#define APIN_UNTERSTUETZUNG 1
#define APIN_TRETWIDERSTAND 2
#define APIN_BREMSHEBEL 3

//Pins fuer spaetere Erweiterungen, die bereits am Stecker verbunden sind
//#define PIN_AUX_1 13
//#define PIN_AUX_2 5

#define GESCHWINDIGKEIT_ERMITTELN_INTERVAL 5 //mal pro Sekunde
#define MOTORSTEUERUNG_INTERVAL 20 //mal pro Sekunde
#define MILLISEKUNDEN_PRO_SEKUNDE 1000
#define GESCHWINDIGKEIT_UMRECHNUNGSFAKTOR 278 //mm/s entsprechen 1 km/h
#define MAX_PEDELEC_GESCHWINDIGKEIT MAX_PEDELEC_GESCHWINDIGKEIT_KMH * GESCHWINDIGKEIT_UMRECHNUNGSFAKTOR
#define MAX_SCHIEBEHILFE_GESCHWINDIGKEIT MAX_SCHIEBEHILFE_GESCHWINDIGKEIT_KMH * GESCHWINDIGKEIT_UMRECHNUNGSFAKTOR

#define UNTERSTUETZUNG_KENNLINIE_TEILER 7000 //generatorleistung^2 / TEILER + OFFSET
#define UNTERSTUETZUNG_KENNLINIE_OFFSET -2 //generatorleistung^2 / TEILER + OFFSET

#define MOTORPOLE 46
#define RADUMFANG 1756 //mm
#define NENNSPANNUNG 36 //Volt
#define NENNSTROM 30 //Ampere - Entspricht 100% am Pin Leistung
#define VOLLE_LEISTUNG 255 // 100% Leistungssignal
#define MAX_GESCHWINDIGKEIT_MAX_UNTERSTUETZUNG_LEISTUNG (VOLLE_LEISTUNG * MAX_PEDELEC_LEISTUNG_WATT) / (NENNSTROM * NENNSPANNUNG) // Leistungssignal bei 25 km/h und Volllast

//Elektrische Leistungsbegrezung:
//Durch die elektrische Leistungsbegrenzung wird gewaehrleistet, dass die erlaubte Unterstuetzungsleistung niemals ueberschritten wird. Da der Wirkungsgrad <= 100% ist, ist das physikalisch nicht moeglich.
//Leistung = Drehmoment * Drehzahl
//Leistung = Spannung * Strom
//Spannung und Strom sind bekannt. Drehzahl ist bekannt. Das Drehmoment ist linear zum Strom.
//Das Drehmoment ist nicht bekannt, aber der genaue Wert ist fuer die Leistungsbegrenzung auch nicht so wichtig, weil sicher ist dass die abgegebene mechanische Leistung niemals die elektrische Leistung uebersteigt.
//Bei 36 V liegt der Strom bei 250 W / 36 V = ~7 A.
//Bei der Fahrgeschwindigkeit von 25 km/h und voller elektrischer Leistung 250 W nehme ich die Drehzahl 1 (einheitenlos) und das Drehmoment 1 (einheitenlos) an.
//Diese beiden Variablen muessen sich so verhalten, dass ihr Produkt niemals groesser als 1 wird, dann ist sichergestellt, dass die mechanische Leistung auch 250 Watt nicht uebersteigt.
//Das bedeutet, bei einer Drehzahl von 0.5 (Geschwindigkeit 12.5 km/h) kann das Drehmoment 2 betragen, bei Drehzahl 0.25 kann das Drehmoment 4 sein und so weiter.
//Rechnung: leistung = (MAX_GESCHWINDIGKEIT_MAX_UNTERSTUETZUNG_LEISTUNG * MAX_PEDELEC_GESCHWINDIGKEIT) / geschwindigkeit

#define GENERATORSTROM_MAX 25 //Ampere - Wenn das Strommess-IC ein 100% Signal gibt
#define GENERATORSTROM_MAX_SIGNAL 1023 // 100% Eingangssignal - Bedeutet maximaler positiver Stromfluss am Strommess-IC
#define GENERATOR_UMRECHNUNGSFAKTOR GENERATORSTROM_MAX * VOLLE_LEISTUNG
#define GENERATOR_UMRECHNUNGSTEILER (GENERATORSTROM_MAX_SIGNAL / 2) * NENNSTROM
//Rechnung: Ausgabeleistung = (Eingabeleistung * GENERATOR_UMRECHNUNGSFAKTOR) / GENERATOR_UMRECHNUNGSTEILER

uint16_t geschwindigkeit = 0; // in Millimeter pro Sekunde! 1000 mm/s = 1 m/s (278 mm/s = 1 km/h)
volatile bool rueckwaertsIst = false;
bool rueckwaertsSoll = false;
uint16_t unterstuetzung = 0;
uint16_t tretwiderstand = 0;
uint16_t bremshebel = 0;
uint8_t leistung = 0;
uint16_t generatorleistung = 0;
uint8_t ausgabeleistungNachGenerator = 0;
uint8_t rekuperation = 0;

volatile uint8_t motorInkrementZaehler = 0;
uint8_t motorInkrementLetzterZaehlerstand = 0;
uint32_t zeit = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_LEISTUNG, OUTPUT);
  pinMode(PIN_GENERATORWIDERSTAND, OUTPUT);
  pinMode(PIN_RUECKWAERTS, OUTPUT);
  pinMode(PIN_REKUPERATION, OUTPUT);

  pinMode(PIN_HALL_A, INPUT);
  pinMode(PIN_HALL_B, INPUT);

  pinMode(PIN_GEN_HALL_A, INPUT);
  pinMode(PIN_GEN_HALL_B, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_HALL_A), motorHallISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_GEN_HALL_A), generatorISR, CHANGE);

}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() != zeit) {
    zeit = millis();
    zeitfunktionen();
  }
  
}

void zeitfunktionen() {
  if (zeit % (MILLISEKUNDEN_PRO_SEKUNDE / GESCHWINDIGKEIT_ERMITTELN_INTERVAL) == 0) {
    noInterrupts();
    int8_t schritte = motorInkrementZaehler - motorInkrementLetzterZaehlerstand;
    motorInkrementLetzterZaehlerstand = motorInkrementZaehler;
    interrupts();
    geschwindigkeit = (schritte * RADUMFANG) / MOTORPOLE;

    unterstuetzung = analogRead(APIN_UNTERSTUETZUNG);
    tretwiderstand = analogRead(APIN_TRETWIDERSTAND);
    analogWrite(PIN_GENERATORWIDERSTAND, zehnBitInAchtBitUmwandeln(tretwiderstand));
  }
  if (zeit % (MILLISEKUNDEN_PRO_SEKUNDE / MOTORSTEUERUNG_INTERVAL) == 0) {
    generatorleistungMessen();
    if (rueckwaertsIst == rueckwaertsSoll) {
      if (geschwindigkeit > MAX_PEDELEC_GESCHWINDIGKEIT) {
        leistung = ausgabeleistungNachGenerator;
      }
      else {
        leistung = ausgabeleistungNachGenerator + unterstuetzungsleistungBerechnen();
      }
    }
    else {
      rekuperation = ausgabeleistungNachGenerator; // TODO: Passende Rekuperationsstärke anhand der Generatorleistung berechnen
    }

    bremshebel = analogRead(APIN_BREMSHEBEL);
    uint8_t manuelleRekuperation = zehnBitInAchtBitUmwandeln(bremshebel);
    if (manuelleRekuperation > rekuperation) {
      rekuperation = manuelleRekuperation;
    }

    analogWrite(PIN_LEISTUNG, leistung);
  }
}

void motorHallISR() {
  bool vorwaerts = digitalRead(PIN_HALL_A) == digitalRead(PIN_HALL_B);
  if (vorwaerts) {
    motorInkrementZaehler++;
  }
  else {
    motorInkrementZaehler--;
  }
  rueckwaertsIst = !vorwaerts;
}

void generatorISR() {
  //TODO: Richtung erkennen, optional Drehzahl ermitteln
}

uint8_t zehnBitInAchtBitUmwandeln(uint16_t zehnBitWert) {
  return (uint8_t)(zehnBitWert >> 2);
}

void analogeingabenAbfragen() {
  generatorleistung = analogRead(APIN_GENERATORLEISTUNG);
  unterstuetzung = analogRead(APIN_UNTERSTUETZUNG);
  tretwiderstand = analogRead(APIN_TRETWIDERSTAND);
  bremshebel = analogRead(APIN_BREMSHEBEL);
}

void generatorleistungMessen() {
  generatorleistung = analogRead(APIN_GENERATORLEISTUNG);
  if (generatorleistung < 512) {
    ausgabeleistungNachGenerator = 0;
  }
  else {
    ausgabeleistungNachGenerator = ((generatorleistung - 512) * GENERATOR_UMRECHNUNGSFAKTOR) / GENERATOR_UMRECHNUNGSTEILER;
  }
}

uint8_t unterstuetzungsleistungBerechnen() {
  return constrain(
    ((generatorleistung * generatorleistung) / UNTERSTUETZUNG_KENNLINIE_TEILER + UNTERSTUETZUNG_KENNLINIE_OFFSET), 
    0, (MAX_GESCHWINDIGKEIT_MAX_UNTERSTUETZUNG_LEISTUNG * MAX_PEDELEC_GESCHWINDIGKEIT) / geschwindigkeit);
}
