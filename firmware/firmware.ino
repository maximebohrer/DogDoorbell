#include <Tone.h> // https://github.com/bhagman/Tone
#include <EEPROM.h>

#define sensorIn 4 // In and out both on port D
#define sensorOut 2
#define buzzer1Pin 7
#define buzzer2Pin 8
#define ledPin 13
#define potPin A2

// Music notes frequency
const int C[] = {33, 65, 131, 262, 523, 1047, 2093, 418, 8372, 16744};
const int Cs[] = {35, 69, 139, 277, 554, 1109, 2217, 4435, 8870, 17740};
const int D[] = {37, 73, 147, 294, 587, 1175, 2349, 4699, 9397, 18795};
const int Ds[] = {39, 78, 156, 311, 622, 1245, 2489, 4978, 9956, 19912};
const int E[] = {42, 82, 165, 330, 659, 1319, 2637, 5274, 10548, 21096};
const int F[] = {44, 87, 175, 349, 698, 1397, 2794, 5588, 11175, 22351};
const int Fs[] = {46, 93, 185, 370, 740, 1480, 2960, 5920, 11840, 23680};
const int G[] = {49, 98, 196, 392, 784, 1568, 3134, 6272, 12544, 25088};
const int Gs[] = {52, 104, 208, 415, 831, 1661, 3322, 6645, 13290, 26580};
const int A[] = {55, 110, 220, 440, 880, 1760, 3520, 7040, 14080, 28160};
const int As[] = {58, 117, 233, 466, 932, 1865, 3729, 7459, 14917, 29834};
const int B[] = {62, 123, 247, 494, 988, 1976, 3951, 7902, 15804, 31609};

float baselineToSubtract;
float filteredMeasurement;
bool detection = 0;
const long timeout = 500000;
float threshold;

// Sampling
const int samplingPeriod = 50;
unsigned long samplingTimer = 0;

// Music and LED
unsigned long musicTimer = 0;
unsigned long ledTimer = 0;
bool ledState = 0;
Tone buzzer1;
Tone buzzer2;

class Music {
  private:
  // Buzzer 1
    int* notes1;        // Frequency of the notes that have to be played. 0 if no sound
    int* noteLengths1;  // How many units of time each notes have to be played (Same length as notes1)
    int quantity1;      // Number of notes in the melody (Length of notes1 and noteLengths1 arrays)
    int index1;         // Keeps track of what note is being played in the notes1 array
    int subindex1;      // Keeps track of how many units of time of this note have already been played

    // Buzzer 2
    int* notes2;
    int* noteLengths2;
    int quantity2;
    int index2;
    int subindex2;

    int intervalle;    // Length of the unit of time in milliseconds
    unsigned long timer_musique;

    bool playing;

  public:
    Music(int notes1[], int noteLengths1[], int quantity1, int notes2[], int noteLengths2[], int quantity2, int intervalle) {
      this->notes1 = notes1;
      this->noteLengths1 = noteLengths1;
      this->quantity1 = quantity1;
      this->notes2 = notes2;
      this->noteLengths2 = noteLengths2;
      this->quantity2 = quantity2;
      this->intervalle = intervalle;

      playing = false;
      index1 = 0;
      subindex1 = 0;
      index2 = 0;
      subindex2 = 0;
    }

    void play() {
      if (millis() - timer_musique >= intervalle || !playing) {
        timer_musique = millis();

        if (notes1[index1] > 0 && (subindex1 == 0 || !playing)) { // Call play() only if it's a new note or if the music is resumed (The Tone library handles the timing by itself)
          buzzer1.play(notes1[index1], (noteLengths1[index1] - subindex1) * intervalle - intervalle / 10); // Subtract a tenth of time unit to have a clear gap between notes
        }
        if (notes2[index2] > 0 && (subindex2 == 0 || !playing)) {
          buzzer2.play(notes2[index2], (noteLengths2[index2] - subindex2) * intervalle - intervalle / 10);
        }

        subindex1++;
        if (subindex1 >= noteLengths1[index1]) {
          index1++;
          subindex1 = 0;
          if (index1 >= quantity1) {
            index1 = 0;
          }
        }
        subindex2++;
        if (subindex2 >= noteLengths2[index2]) {
          index2++;
          subindex2 = 0;
          if (index2 >= quantity2) {
            index2 = 0;
          }
        }

        playing = true;
      }
    }

    void stop() {
      playing = false;
      buzzer1.stop();
      buzzer2.stop();
    }
};

Music *currentMusic; // Pointer to the current music

// Comptine d'un autre été
const int amelie_notes1[] =     { E[2], B[2], G[2], B[2], E[2], B[2], G[2], B[2],    D[2], B[2], G[2], B[2], D[2], B[2], G[2], B[2],    D[2], B[2], Fs[2], B[2], D[2], B[2], Fs[2], B[2],    D[2], A[2], Fs[2], A[2], D[2], A[2], Fs[2], A[2]};
const int amelie_noteLengths1[] = { 2,    2,    2,    2,    2,    2,    2,    2,       2,    2,    2,    2,    2,    2,    2,    2,       2,    2,    2,     2,    2,    2,    2,     2,       2,    2,    2,     2,    2,    2,    2,     2};
const int amelie_notes2[] =     { B[3], E[4], B[4], B[3], E[4], B[4], B[3], E[4], B[4], B[3], E[4], B[4], B[3], E[4], C[4], E[4],    B[3], D[4], B[4], B[3], D[4], B[4], B[3], D[4], B[4], B[3], D[4], B[4], B[3], D[4], A[3], D[4],    Fs[3], B[3], Fs[4], Fs[3], B[3], Fs[4], Fs[3], B[3], Fs[4], Fs[3], B[3], Fs[4], Fs[3], B[3], G[3], B[3],    A[3], D[4], A[4], A[3], D[4], A[4], A[3], D[4], A[4], A[3], D[4], A[4], A[3], D[4], G[3], D[4]};
const int amelie_noteLengths2[] = { 1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,       1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,       1,     1,    1,     1,     1,    1,     1,     1,    1,     1,     1,    1,     1,     1,    1,    1,       1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1};
Music amelie(amelie_notes1, amelie_noteLengths1, 32, amelie_notes2, amelie_noteLengths2, 64, 230);

// Canon in D
const int canon_notes1[] =     { D[2], A[2], D[3], A[2], A[1], E[2], A[2], E[2], B[1], Fs[2], B[2], Fs[2], Fs[1], Cs[2], Fs[2], Cs[2], G[1], D[2], G[2], D[2], D[2], A[2], D[3], A[2], G[1], D[2], G[2], D[2], A[1], E[2], A[2], E[2] };
const int canon_noteLengths1[] = { 2,    2,    2,    2,    2,    2,    2,    2,    2,    2,     2,    2,     2,     2,     2,     2,     2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2 };
const int canon_notes2[] =     { A[4], Fs[4], G[4], A[4], Fs[4], G[4], A[4], A[3], B[3], Cs[4], D[4], E[4], Fs[4], G[4], Fs[4], D[4], E[4], Fs[4], Fs[3], G[3], A[3], B[3], A[3], G[3], A[3], D[4], Cs[4], D[4], B[3], D[4], Cs[4], B[3], A[3], G[3], A[3], G[3], Fs[3], G[3], A[3], B[3], Cs[4], D[4], B[3], D[4], Cs[4], B[3], Cs[4], D[4], E[4], A[3], B[3], Cs[4], D[4], E[4], Fs[4], G[4] };
const int canon_noteLengths2[] = { 2,    1,     1,    2,    1,     1,    1,    1,    1,    1,     1,    1,    1,     1,    2,     1,    1,    2,     1,     1,    1,    1,    1,    1,    1,    1,    1,     1,    2,    1,    1,     2,    1,    1,    1,    1,    1,     1,    1,    1,    1,     1,    2,    1,    1,     2,    1,     1,    1,    1,    1,    1,     1,    1,    1,     1};
Music canon(canon_notes1, canon_noteLengths1, 32, canon_notes2, canon_noteLengths2, 56, 200);

// Still DRE
const int dre_notes1[] =     { A[2], 0,  B[2], E[2], 0,   E[2]};
const int dre_noteLengths1[] = { 12,   24, 12,   12,   24,  12};
const int dre_notes2[] =     { A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0, C[4], E[4], A[4], 0,    B[3], E[4], A[4], 0, B[3], E[4], A[4], 0, B[3], E[4], A[4], 0,    B[3], E[4], G[4], 0, B[3], E[4], G[4], 0, B[3], E[4], G[4], 0, B[3], E[4], G[4], 0, B[3], E[4], G[4], 0,    C[4], E[4]};
const int dre_noteLengths2[] = { 3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1,    1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1,    1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1, 1,    1,    3,    1,    1,    1 };
Music dre(dre_notes1, dre_noteLengths1, 6, dre_notes2, dre_noteLengths2, 64, 53);

void setup() {
  pinMode(sensorIn, INPUT);
  pinMode(sensorOut, OUTPUT);
  pinMode(ledPin, OUTPUT);

  buzzer1.begin(buzzer1Pin);
  buzzer2.begin(buzzer2Pin);

  // Read potentiometer to set sensitivity
  threshold = map(analogRead(potPin), 0, 1023, 0, 5000);

  nextMusic();
  calibration();
  startupSound();
}

void nextMusic() {
  byte id = EEPROM.read(7); // Current music stored in address 7
  id = (id + 1) % 3; // Only 3 songs
  EEPROM.write(7, id);
  switch (id) {
    case 0:
      currentMusic = &amelie;
      break;
    case 1:
      currentMusic = &canon;
      break;
    case 2:
      currentMusic = &dre;
      break;
  }
}

void calibration() {
  baselineToSubtract = 0;
  int nb = 0;
  // Make 40 measurements and take average to find the baseline
  while (nb < 40) {
    if (millis() - samplingTimer >= samplingPeriod) {
      samplingTimer = millis();
      long measurement = takeMeasurement();
      if (measurement >= 0) {
        nb += 1;
        baselineToSubtract += measurement;
      }
    }
    // Blink led at the same time
    if (millis() - ledTimer >= 250) {
      ledTimer = millis();
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
  baselineToSubtract /= 40.0;
  filteredMeasurement = 0;
  ledState = 0;
  digitalWrite(ledPin, LOW);
}

void startupSound() {
  buzzer1.play(Ds[5], 345);
  buzzer2.play(Gs[2], 798);
  delay(370);
  buzzer1.play(Ds[4], 132);
  delay(132);
  buzzer1.play(As[4], 271);
  delay(296);
  buzzer1.play(Gs[4], 345);
  buzzer2.play(C[3], 798);
  delay(370);
  buzzer1.play(Ds[4], 132);
  delay(132);
  buzzer1.play(Ds[5], 271);
  delay(296);
  buzzer1.play(As[4], 1000);
  buzzer2.play(Ds[2], 1000);
  delay(1000);
}

void loop() {
  if (millis() - samplingTimer >= samplingPeriod) {
    samplingTimer = millis();
    long measurement = takeMeasurement();
    if (measurement >= 0) {
      filteredMeasurement = (5.0 * filteredMeasurement + measurement - baselineToSubtract) / 6.0; // Subtract baseline and apply low pass filter
      if (!detection && filteredMeasurement >= threshold) {
        // Dog is here
        detection = 1;
      }
      else if (detection && filteredMeasurement < threshold) {
        // Dog is not here
        detection = 0;
        ledState = 0;
        digitalWrite(ledPin, LOW);
        currentMusic->stop();
      }

      if (!detection) baselineToSubtract = (63.0 * baselineToSubtract + measurement) / 64.0; // Adjust baseline dynamically to avoid false positives (very slow low pass filter)
    }
  }

  if (detection) {
    currentMusic->play(); // Play music
    // Blink led
    if (millis() - ledTimer >= 150) {
      ledTimer = millis();
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}

// measurement le temps de chargement/déchargement du capteur 10 fois. -1 si timeout
long takeMeasurement() {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    long measurement = 0;

    // Here we disable interrupts to not mess up timing during critical measurement steps.
    // We directly write to PORTD to avoid the overhead of the Arduino functions (digitalWrite, pinMode, etc).
    noInterrupts();
    bitSet(PORTD, sensorOut);   // Output pin LOW just in case
    bitSet(DDRD, sensorIn);     // Set input pin as OUTPUT
    bitClear(PORTD, sensorIn);  // Input pin LOW to fully discharge the capacitive sensor
    delayMicroseconds(10);      // Wait for the sensor to discharge
    bitClear(DDRD, sensorIn);   // Input pin back as INPUT
    bitSet(PORTD, sensorOut);   // Output pin HIGH to charge the sensor through the resistor
    interrupts();

    while (!bitRead(PIND, sensorIn) && measurement < timeout) { // measurement of charge time
      measurement++;
    }

    noInterrupts();
    bitSet(DDRD, sensorIn);     // Input pin as OUTPUT
    bitSet(PORTD, sensorIn);    // Input pin HIGH to fully charge the capacitive sensor
    delayMicroseconds(10);      // Wait for the sensor to charge
    bitClear(DDRD, sensorIn);   // Input pin back as INPUT
    bitClear(PORTD, sensorIn);  // Input pin LOW to prepare next cycle and deactivate pullup resistor
    bitClear(PORTD, sensorOut); // Ouput pin LOW to discharge the sensor through the resistor
    interrupts();

    while (bitRead(PIND, sensorIn) && measurement < timeout) { // measurement of charge discharge time
      measurement++;
    }

    if (measurement >= timeout) {
      return -1;
    }
    total += measurement;
  }

  return total;
}
