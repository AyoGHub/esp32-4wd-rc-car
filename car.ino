#include <bluepad32.h>

// =================================================================
// --- L293D SIDE 1: ALL 3 DRIVE MOTORS (Wired in Parallel) --------
// =================================================================
const int driveEnable1 = 25; // Connect to L293D Pin 1 (Enable 1,2)
const int driveInput1  = 26; // Connect to L293D Pin 2 (Input 1)
const int driveInput2  = 27; // Connect to L293D Pin 7 (Input 2)

// =================================================================
// --- L293D SIDE 2: 4th DC MOTOR FOR INDEPENDENT STEERING ---------
// =================================================================
const int driveEnable2 = 14; // Connect to L293D Pin 9 (Enable 3,4)
const int driveInput3  = 12; // Connect to L293D Pin 10 (Input 3)
const int driveInput4  = 13; // Connect to L293D Pin 15 (Input 4)

const int steerSpeed   = 140; // Tuned ~55% steering power constraint
const int blueLED      = 2;  
const int relayPin     = 32; 

bool globalForward = false;
bool globalBackward = false;
int requestedSteering = 0;   // -1 = Left, 0 = Stopped, 1 = Right

// =================================================================
// --- NON-BLOCKING RAINBOW RGB LED CONFIGURATION -----------------
// =================================================================
const int redPin   = 18;
const int greenPin = 19;
const int bluePin  = 21;

const int redChannel   = 0;
const int greenChannel = 1;
const int blueChannel  = 2;

bool ledRainbowActive = false; 
bool lastTriggerState = false;  
int currentHue = 0;            
unsigned long lastLedUpdate = 0;
const unsigned long ledInterval = 120; // Slower update frame window (60ms)

// =================================================================
// --- BUZZER & NON-BLOCKING HORN/MELODY MASTER SYSTEM ------------
// =================================================================
const int buzzerPin = 22; // Connect positive leg of passive buzzer here
const int buzzerChannel = 4; // Isolated PWM channel for sound generation

bool lastYButtonState = false;

// Note frequency definitions (Hz)
#define NOTE_Bf4 466  // B Flat 4 (Mutes song)
#define NOTE_E3  165
#define NOTE_G3  196
#define NOTE_A4  220
#define NOTE_B4  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A5  440
#define NOTE_AS5 466
#define NOTE_B5  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_A6  880
#define NOTE_AS6 932
#define NOTE_B6  988
// Melody Management Variables
bool melodyPlaying = false;
int currentSongSelection = 0; 
int currentNoteIndex = 0;
unsigned long noteStartTime = 0;
unsigned long currentNoteDuration = 0;

// Maximum structure limits for note handling rows
const int MAX_NOTES = 41; // Expanded to 41 to fit complex tracks
int songMelodies[3][MAX_NOTES];
int songDurations[3][MAX_NOTES];
int songLengths[3];

// --- GAMMA CORRECTION TABLE ---
const uint8_t gammaTable[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3,   3,   3,
    3,   4,   4,   4,   4,   5,   5,   5,   6,   6,   6,   7,   7,   7,   8,   8,
    9,   9,   9,  10,  10,  11,  11,  12,  12,  13,  13,  14,  14,  15,  15,  16,
   16,  17,  17,  18,  18,  19,  20,  20,  21,  21,  22,  23,  23,  24,  25,  25,
   26,  27,  28,  28,  29,  30,  31,  32,  32,  33,  34,  35,  36,  37,  38,  39,
   40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  54,  55,  56,
   57,  59,  60,  61,  63,  64,  65,  67,  68,  70,  71,  73,  74,  76,  77,  79,
   81,  82,  84,  86,  87,  89,  91,  93,  94,  96,  98, 100, 102, 104, 106, 108,
  110, 112, 114, 116, 118, 121, 123, 125, 127, 130, 132, 134, 137, 139, 142, 144,
  147, 149, 152, 154, 157, 160, 162, 165, 168, 171, 174, 176, 179, 182, 185, 188,
  191, 194, 197, 200, 203, 207, 210, 213, 216, 220, 223, 226, 230, 233, 237, 240,
  244, 247, 251, 255
};

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Joy-Con connected\n");
      myControllers[i] = ctl;
      break;
    }
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      break;
    }
  }
  stopAllMotors();
}

void stopAllMotors() {
  digitalWrite(blueLED, LOW);
  digitalWrite(relayPin, LOW);
  
  digitalWrite(driveInput1, LOW);  digitalWrite(driveInput2, LOW);
  analogWrite(driveEnable1, 0);
  
  digitalWrite(driveInput3, LOW);  digitalWrite(driveInput4, LOW);
  analogWrite(driveEnable2, 0);
  
  ledcWriteTone(buzzerChannel, 0);
}

void setRainbowColor(int hue) {
  float r, g, b;
  float h = hue / 60.0;
  float x = (1.0 - fabs(fmod(h, 2.0) - 1.0));
  
  if (h >= 0 && h < 1)      { r = 1.0; g = x;   b = 0.0; }
  else if (h >= 1 && h < 2) { r = x;   g = 1.0; b = 0.0; }
  else if (h >= 2 && h < 3) { r = 0.0; g = 1.0; b = x;   }
  else if (h >= 3 && h < 4) { r = 0.0; g = x;   b = 1.0; }
  else if (h >= 4 && h < 5) { r = x;   g = 0.0; b = 1.0; }
  else                      { r = 1.0; g = 0.0; b = x;   }

  // Convert floats to 8-bit integers (0-255)
  int rVal = (int)(r * 255);
  int gVal = (int)(g * 255);
  int bVal = (int)(b * 255);

  // CRITICAL NOTE: If your RGB LED is "Common Anode", you must invert these values:
  // rVal = 255 - rVal; gVal = 255 - gVal; bVal = 255 - bVal;

  ledcWrite(redChannel, rVal);
  ledcWrite(greenChannel, gVal);
  ledcWrite(blueChannel, bVal);
}


void turnLedOff() {
  ledcWrite(redChannel,   0);
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel,  0);
}

void startSong(int songNum) {
  melodyPlaying = true;
  currentSongSelection = songNum;
  currentNoteIndex = 0;
  currentNoteDuration = songDurations[songNum][0];
  noteStartTime = 0; 
}

void setupSongData() {
  // --- SONG 0: El Jarabe Tapatío (Mexican Hat Dance Hook) ---
  songLengths[0] = 40;
  int m0[] = {NOTE_G5, NOTE_FS5, NOTE_G5, NOTE_E5, NOTE_DS5, NOTE_E5, NOTE_C5, NOTE_B5, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A5, NOTE_B5, NOTE_C5,  NOTE_D5, NOTE_E5, NOTE_F5, NOTE_D5, NOTE_F5, NOTE_E5, NOTE_F5, NOTE_D5, NOTE_CS5, NOTE_D5, NOTE_B5, NOTE_AS5, NOTE_B5, NOTE_G4, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_A6, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,    0};
  int d0[] = {150,     150,       150,      150,      150,     150,     150,      150,     150,     450,     150,     150,    150,      150,      150,     150,     150,      150,     150,     450,   150,     150,     150,      150,      150,     150,     150,      150,     150,     450,    150,    150,     150,    150,      150,     150,      150,    150,      300,      300};
  for(int i=0; i<40; i++) { songMelodies[0][i] = m0[i]; songDurations[0][i] = d0[i]; }

  // --- SONG 1: La Cucaracha (Chorus Hook) ---
  songLengths[1] = 18;
  int m1[] = {NOTE_C4, NOTE_C4, NOTE_C4, NOTE_F4, NOTE_A5, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_F4, NOTE_A5, 0,       NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,NOTE_D4, NOTE_C4};
  int d1[] = {125,     125,     125,     350,     250,     125,     125,     125,     350,     250,     125,     200,     200,     200,      200,     200,    200,     200};
  for(int i=0; i<18; i++) { songMelodies[1][i] = m1[i]; songDurations[1][i] = d1[i]; }

  // --- SONG 2: Charge! Baseball Stadium Song ---
  songLengths[2] = 7;
  int m2[] = {NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_E5, NOTE_G5, 0};
  int d2[] = {150,     150,     150,     300,     150,     600,     400};
  for(int i=0; i<7; i++) { songMelodies[2][i] = m2[i]; songDurations[2][i] = d2[i]; }

}

void setup() {
  Serial.begin(115200);
  setupSongData();

  // Initialize Motor Pins
  pinMode(blueLED, OUTPUT);
  pinMode(driveEnable1, OUTPUT);
  pinMode(driveInput1, OUTPUT);
  pinMode(driveInput2, OUTPUT);
  pinMode(driveEnable2, OUTPUT);
  pinMode(driveInput3, OUTPUT);
  pinMode(driveInput4, OUTPUT);
  pinMode(relayPin, OUTPUT);
  
  stopAllMotors();
  

  ledcSetup(redChannel, 5000, 8);
  ledcSetup(greenChannel, 5000, 8);
  ledcSetup(blueChannel, 5000, 8);
  
  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);
  turnLedOff();

  // Buzzer Setup Configuration (Version 2.x Syntax)

  
  

  turnLedOff();

  // Buzzer Setup Configuration
 // Do the same update for your buzzer setup line:
  ledcSetup(buzzerChannel, 2000, 12);
  ledcAttachPin(buzzerPin, buzzerChannel);
  ledcWriteTone(buzzerChannel, 0); 
  
  BP32.setup(&onConnectedController, &onDisconnectedController);
}

void loop() {
  bool hasLatestData = BP32.update();
  bool regularHornActive = false;
  bool yButtonCurrentlyPressed = false;
  
  if (hasLatestData) {
    globalForward = false;
    globalBackward = false;
    requestedSteering = 0; 
    bool triggerCurrentlyPressed = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      ControllerPtr ctl = myControllers[i];
      if (ctl && ctl->isConnected()) {
        
        uint32_t buttons = ctl->buttons();
        
        // Acceleration Mapping
        if (buttons & BUTTON_SHOULDER_L) globalForward = true;
        if (buttons & BUTTON_SHOULDER_R) globalBackward = true;

        // ZL/ZR Toggle Detection
        if ((buttons & BUTTON_TRIGGER_L) || (buttons & BUTTON_TRIGGER_R)) {
          triggerCurrentlyPressed = true;
        }

        // Steering Mapping
        int32_t joyX = ctl->axisX(); 
        if (joyX < -200) requestedSteering = -1; 
        else if (joyX > 200) requestedSteering = 1;  

        // --- MUTE & SONG CONTROL DETECTIONS ---
        if ((buttons & BUTTON_THUMB_L) || (buttons & BUTTON_THUMB_R)) {
          regularHornActive = true; 
        }
        if (buttons & BUTTON_Y) {
          yButtonCurrentlyPressed = true;
        }
      }
    }

    // LED State machine button latch
    if (triggerCurrentlyPressed && !lastTriggerState) {
      ledRainbowActive = !ledRainbowActive; 
    }
    lastTriggerState = triggerCurrentlyPressed; 

    
    

    // Y Button Song Router Latch (Advances song index only when initially pressed down)
    if (yButtonCurrentlyPressed && !lastYButtonState) {
      static int nextSongIndex = 0;
      startSong(nextSongIndex);
      
      nextSongIndex++;
      if (nextSongIndex > 3) nextSongIndex = 0; // Loop queue index back to track 0
    }
    lastYButtonState = yButtonCurrentlyPressed;

    // --- EXECUTE DRIVE WHEELS (L293D SIDE 1) ---
    if (globalForward) {
      digitalWrite(blueLED, HIGH);
      digitalWrite(relayPin, HIGH);
      analogWrite(driveEnable1, 255);
      digitalWrite(driveInput1, HIGH);   digitalWrite(driveInput2, LOW);
    } 
    else if (globalBackward) {
      digitalWrite(blueLED, HIGH);
      digitalWrite(relayPin, LOW);
      analogWrite(driveEnable1, 255);
      digitalWrite(driveInput1, LOW);    digitalWrite(driveInput2, HIGH);
    } 
    else {
      digitalWrite(blueLED, LOW);
      digitalWrite(relayPin, LOW);
      digitalWrite(driveInput1, LOW);   digitalWrite(driveInput2, LOW);
      analogWrite(driveEnable1, 0);
    }
  }
  
  // --- EXECUTE STEERING WHEEL (L293D SIDE 2) ---
  if (requestedSteering == -1) {
    analogWrite(driveEnable2, steerSpeed);
    digitalWrite(driveInput3, HIGH);   digitalWrite(driveInput4, LOW);
  } 
  else if (requestedSteering == 1) {
    analogWrite(driveEnable2, steerSpeed);
    digitalWrite(driveInput3, LOW);    digitalWrite(driveInput4, HIGH);
  } 
  else {
    digitalWrite(driveInput3, LOW);   digitalWrite(driveInput4, LOW);
    analogWrite(driveEnable2, 0);
  }

  // --- NON-BLOCKING RAINBOW STATE MACHINE ENGINE ---
  if (ledRainbowActive) {
    if (millis() - lastLedUpdate >= ledInterval) {
      currentHue++;
      if (currentHue >= 360) currentHue = 0;
      setRainbowColor(currentHue);
      lastLedUpdate = millis();
    }
  } else {
    turnLedOff();
  }

  // =================================================================
  // --- MUTE & AUDIO SOUND ENGINE ROUTER -------------
  // =================================================================
  if (regularHornActive) {
    melodyPlaying = false; // Override any background song if you explicitly hit the mute button
    ledcWriteTone(buzzerChannel, NOTE_Bf4); // Mutes sound
  } 
  else if (melodyPlaying) {
    if (millis() - noteStartTime >= currentNoteDuration) {
      
      // FIXED: Corrected index matching for 2D song arrays
      if (currentNoteIndex < songLengths[currentSongSelection]) {
        int note = songMelodies[currentSongSelection][currentNoteIndex];
        currentNoteDuration = songDurations[currentSongSelection][currentNoteIndex];
        
        if (note == 0) {
          ledcWriteTone(buzzerChannel, 0); // Silent rest note marker
        } else {
          ledcWriteTone(buzzerChannel, note); // Play pitch frequency
        }
        
        noteStartTime = millis();
        currentNoteIndex++;
      } else {
        ledcWriteTone(buzzerChannel, 0);
        melodyPlaying = false;
      }
    }
  } 
  else {
    ledcWriteTone(buzzerChannel, 0);
  }

  delay(1); 
}

