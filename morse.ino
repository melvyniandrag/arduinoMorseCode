#include <LiquidCrystal.h>

enum class PhotoresistorState
{
  ON,
  OFF
};

constexpr uint8_t RS = 12;
constexpr uint8_t ENABLE = 11;
constexpr uint8_t D4 = 5;
constexpr uint8_t D5 = 4;
constexpr uint8_t D6 = 3;
constexpr uint8_t D7 = 2;

LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);
constexpr uint8_t WORD_SPACE = 7;
constexpr uint8_t CHAR_SPACE = 3;
constexpr uint32_t MORSE_TIME_MS = 1000;
constexpr uint32_t NUM_POLLS = 5;
constexpr uint32_t POLL_INTERVAL_MS =
    MORSE_TIME_MS / NUM_POLLS;
uint8_t currentSample = 0;

constexpr int INPUT_PIN = A0;
constexpr int PHOTORESISTOR_CUTOFF = 512;
int reliableSample = -1;
int numSynchronizationOffs = 0;
int samplesToIgnore = -1;//NUM_POLLS;

constexpr char MORSE_TREE[32] = {
  '\0',                         // 0 unused
  '\0',                         // 1 root
  'E', 'T',                     // 2–3
  'I', 'A', 'N', 'M',           // 4–7
  'S', 'U', 'R', 'W',           // 8–11
  'D', 'K', 'G', 'O',           // 12–15
  'H', 'V', 'F', '\0',          // 16–19
  'L', '\0', 'P', 'J',          // 20–23
  'B', 'X', 'C', 'Y',           // 24–27
  'Z', 'Q', '\0', '\0'          // 28–31
};

bool currentSignal;
uint16_t runLength = 0;
uint8_t morseNode = 1;
bool characterEmitted = true;
bool wordSpaceEmitted = true;

PhotoresistorState lastState = PhotoresistorState::OFF;

uint32_t nextPollTime;

void addDot()
{
      Serial.println("dot");

  morseNode *= 2;
}

void addDash()
{
      Serial.println("dash");

  morseNode = morseNode * 2 + 1;
}




void setup()
{
    Serial.begin(9600);
    pinMode(INPUT_PIN, INPUT);
    nextPollTime = millis();
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("ALFIE MORSE");
}

void printMsg(String msg)
{
  lcd.setCursor(0, 1);
  lcd.print("                "); // 16 spaces
  lcd.setCursor(0, 1);
  lcd.print(msg);
}

void printMsg(char msg)
{
  lcd.setCursor(0, 1);
  lcd.print("                "); // 16 spaces
  lcd.setCursor(0, 1);
  lcd.print(msg);
}

void resetSynchronizationState()
{
  samplesToIgnore = -1;
  numSynchronizationOffs = 0;
  reliableSample = -1;
  lastState = PhotoresistorState::OFF;
  runLength = 0;
  characterEmitted = true;
  wordSpaceEmitted = true;
  morseNode = 1;
}

void synchronize(PhotoresistorState& state, int currentSample)
{
    // try to synchronize
    // synchronize on word end
    // want to get 7 offs in a row
    if(numSynchronizationOffs == 0 && samplesToIgnore < 0)
    {
      samplesToIgnore = 2;
    }
    if(samplesToIgnore == 0)
    {
      if(state == PhotoresistorState::OFF)
      {
        numSynchronizationOffs += 1;
        samplesToIgnore = NUM_POLLS;
        //Serial.print("Num offs: ");
        //Serial.print(numSynchronizationOffs);
        //Serial.print("/");
        //Serial.println(WORD_SPACE);
        if(numSynchronizationOffs == WORD_SPACE)
        {
          //Serial.print("Synchronized on sample: ");
          //Serial.println(currentSample);
          printMsg("Synced");
          reliableSample = currentSample;
        }
      }
      else
      {
        Serial.println("Synchronization failed! Starting over.");
        resetSynchronizationState();
      }
    }
    else
    {
      samplesToIgnore -= 1;
    }
}

char getCharacter()
{
  char ret = '?';
  if (morseNode < sizeof(MORSE_TREE) && MORSE_TREE[morseNode] != '\0') {
    ret =  MORSE_TREE[morseNode];
  }
  morseNode = 1;
  return ret;
}



void processCompletedRun(PhotoresistorState& state, uint16_t length)
{
  if (state == PhotoresistorState::ON)
  {
    // Adjust these classifications to tolerate timing errors.
    // e.g. could force length == 1 or length == 3, and if neither condition is met, log error and reset synchronization
    if (length == 1) {
      addDot();
    } 
    else if(length == 3)
    {
      addDash();
    }
    else
    {
      Serial.println("Error! run length was: ");
      Serial.println(length);
      resetSynchronizationState();
    }
  }
  else 
  {
    characterEmitted = false;
    wordSpaceEmitted = false;
  }
}

void parseMorse(PhotoresistorState& state)
{
  if (state == lastState)
  {
    ++runLength;
  }
  else
  {
    Serial.println("State changed!");

    // The signal changed, so the previous run is complete.
    processCompletedRun(lastState, runLength);

    lastState = state;
    runLength = 1;
  }

  if(state == PhotoresistorState::OFF)
  {
    //Serial.print("photoresistor OFF! runlength: ");
    //Serial.println(runLength);

        if (!characterEmitted &&
            runLength >= CHAR_SPACE) {
            printMsg(getCharacter());
            characterEmitted = true;
        }
        if (!wordSpaceEmitted &&
            runLength >= WORD_SPACE) {
            printMsg("[NEW WORD]");
            wordSpaceEmitted = true;
        }

  }
}

void processPoll(PhotoresistorState& state, int currentSample)
{
  if(reliableSample < 0 || reliableSample >= NUM_POLLS)
  {
    printMsg("Synchronizing");
    synchronize(state, currentSample);
  }
  else
  {
    if(currentSample == reliableSample)
    {
      // Only handle the sample if it's the reliable sample.
      parseMorse(state);
    }
    else
    {
      // Ignore this sample.
    }
  }
}

void loop()
{
    uint32_t now = millis();
    if ((int32_t)(now - nextPollTime) >= 0)
    {
        currentSample = ((currentSample + 1) % NUM_POLLS);
        int value = analogRead(INPUT_PIN);
        PhotoresistorState photoresistorState = PhotoresistorState::OFF;
        if(value > PHOTORESISTOR_CUTOFF)
        {
          photoresistorState = PhotoresistorState::ON;
        }
        else
        {
          photoresistorState = PhotoresistorState::OFF;
        }
        processPoll(photoresistorState, currentSample);
        nextPollTime += POLL_INTERVAL_MS;
    }
}