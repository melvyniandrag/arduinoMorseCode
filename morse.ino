#include <LiquidCrystal.h>

constexpr uint8_t RS = 12;
constexpr uint8_t ENABLE = 11;
constexpr uint8_t D4 = 5;
constexpr uint8_t D5 = 4;
constexpr uint8_t D6 = 3;
constexpr uint8_t D7 = 2;

LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);
constexpr uint8_t WORD_SPACE = 7;
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

enum class PhotoresistorState
{
  ON,
  OFF
};


uint32_t nextPollTime;

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
        Serial.print("Num offs: ");
        Serial.print(numSynchronizationOffs);
        Serial.print("/");
        Serial.println(WORD_SPACE);
        if(numSynchronizationOffs == WORD_SPACE)
        {
          reliableSample = currentSample;
        }
      }
      else
      {
        Serial.println("Synchronization failed! Starting over.");
        samplesToIgnore = -1;
        numSynchronizationOffs = 0;
      }
    }
    else
    {
      samplesToIgnore -= 1;
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
        printMsg("Ready");
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