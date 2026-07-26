#include <LiquidCrystal.h>

constexpr uint8_t RS = 12;
constexpr uint8_t ENABLE = 11;
constexpr uint8_t D4 = 5;
constexpr uint8_t D5 = 4;
constexpr uint8_t D6 = 3;
constexpr uint8_t D7 = 2;

LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);

constexpr uint32_t MORSE_TIME_MS = 1000;
constexpr uint32_t NUM_POLLS = 5;
constexpr uint32_t POLL_INTERVAL_MS =
    MORSE_TIME_MS / NUM_POLLS;
constexpr int INPUT_PIN = A0;
constexpr int PHOTORESISTOR_CUTOFF = 512;

enum class PhotoresistorState
{
  ON,
  OFF
};

PhotoresistorState photoresistorState = PhotoresistorState::OFF;

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

void printPhotoresState(PhotoresistorState& state)
{
  lcd.setCursor(0, 1);
  lcd.print("                "); // 16 spaces
  lcd.setCursor(0, 1);

  if(state == PhotoresistorState::ON)
  {
    lcd.print("On");
  }
  else
  {
    lcd.print("Off");
  }

}

void processPoll()
{
  int value = analogRead(INPUT_PIN);
  
  if(value > PHOTORESISTOR_CUTOFF)
  {
    photoresistorState = PhotoresistorState::ON;
  }
  else
  {
    photoresistorState = PhotoresistorState::OFF;
  }
  printPhotoresState(photoresistorState);

}

void loop()
{
    uint32_t now = millis();

    if ((int32_t)(now - nextPollTime) >= 0)
    {
        // Serial.print("polled at ");
        // Serial.println(now);
        // Serial.println(" ");
        nextPollTime += POLL_INTERVAL_MS;
        processPoll();
    }
}