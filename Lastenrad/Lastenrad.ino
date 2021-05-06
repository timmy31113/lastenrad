#define MOTORPOLE 46
#define RADUMFANG 1756 //mm
#define MAX_LEISTUNG_WATT 500 //Watt
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
#define MAX_PEDELEC_LEISTUNG (255 * MAX_LEISTUNG_WATT) / MAX_PEDELEC_LEISTUNG_WATT

#define UNTERSTUETZUNG_KENNLINIE_TEILER 7000 //generatorleistung^2 / TEILER + OFFSET
#define UNTERSTUETZUNG_KENNLINIE_OFFSET -2 //generatorleistung^2 / TEILER + OFFSET

uint16_t geschwindigkeit = 0; // in Millimeter pro Sekunde! 1000 mm/s = 1 m/s (278 mm/s = 1 km/h)
volatile bool rueckwaertsIst = false;
bool rueckwaertsSoll = false;
uint16_t unterstuetzung = 0;
uint16_t tretwiderstand = 0;
uint16_t bremshebel = 0;
uint8_t leistung = 0;
uint16_t generatorleistung = 0;
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
    generatorleistung = analogRead(APIN_GENERATORLEISTUNG);
    if (rueckwaertsIst == rueckwaertsSoll) {
      if (geschwindigkeit > MAX_PEDELEC_GESCHWINDIGKEIT) {
        leistung = zehnBitInAchtBitUmwandeln(generatorleistung);
      }
      else {
        leistung = zehnBitInAchtBitUmwandeln(generatorleistung) + unterstuetzungsleistungBerechnen();
      }
    }
    else {
      rekuperation = zehnBitInAchtBitUmwandeln(generatorleistung);
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

uint8_t unterstuetzungsleistungBerechnen() {
  return constrain(
    ((generatorleistung * generatorleistung) / UNTERSTUETZUNG_KENNLINIE_TEILER + UNTERSTUETZUNG_KENNLINIE_OFFSET), 
    0, MAX_PEDELEC_LEISTUNG);
}
