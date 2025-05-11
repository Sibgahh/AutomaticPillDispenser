#define BLYNK_TEMPLATE_ID "TMPL6qgg0KNkW"
#define BLYNK_TEMPLATE_NAME "IOT"
#define BLYNK_AUTH_TOKEN "fA88zwaeDa-BkxIHuhTk60RqjzQ_sxUK"
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Stepper.h>
#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// Ultrasonic sensor pins
#define TRIGGER_PIN 4
#define ECHO_PIN 2

char ssid[] = "Yesyesyes";
char pass[] = "AWEWEEWOO";

// Define stepper motor connections and steps per revolution
#define STEPS_PER_REVOLUTION 1024
#define IN1_PIN  5
#define IN2_PIN  18
#define IN3_PIN  19
#define IN4_PIN  17

// Create an instance of the stepper motor
Stepper stepper(STEPS_PER_REVOLUTION, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

// Create an instance of the RTC
RTC_DS3231 rtc;

// Create an instance of the LCD (address 0x27, 16 characters, 2 lines)
LiquidCrystal_I2C lcd(0x27, 16, 2);

BlynkTimer timer;

int moveCount = 0; // Counter for stepper movements
int moveLimit = 1; // Default limit for stepper movements
DateTime lastMoveDate; // Store the last move date

// Define variables for the time windows
int morningStart = 7;
int morningEnd = 10;
int eveningStart = 16;
int eveningEnd = 22;

void setup()
{
  // Start the serial communication
  Serial.begin(115200);

  // Initialize the Blynk connection
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialize the stepper motor
  stepper.setSpeed(5); // Set the speed to 5 RPM

  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    // Set the RTC to the current date & time if it lost power
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pill dispenser");
  lcd.setCursor(0, 1);
  lcd.print("Sibgah, Andrew");
  delay(2000);

  // Initialize the ultrasonic sensor
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup a function to be called every second
  timer.setInterval(1000L, checkDistance);
  timer.setInterval(1000L, displayTime);
}

void loop()
{
  Blynk.run();
  timer.run();
}

BLYNK_WRITE(V0) // Button on V0 for moving the stepper
{
  int pinValue = param.asInt();
  if (pinValue == 1) { // If button is pressed
    if (canMoveStepper()) {
      moveStepper();
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Belum waktunya");
      lcd.setCursor(0, 1);
      lcd.print("minum obat");
      delay(1000);
    }
  }
}

BLYNK_WRITE(V1) // Button on V1 for resetting the move count
{
  int pinValue = param.asInt();
  if (pinValue == 1) { // If button is pressed
    resetMoveCount();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Move Count");
    lcd.setCursor(0, 1);
    lcd.print("Reset");
    delay(2000); // Display reset message for 2 seconds
    displayTime();
  }
}

BLYNK_WRITE(V6) // Slider on V6 for setting morning start time
{
  morningStart = param.asInt(); // Set morningStart to slider value
}

BLYNK_WRITE(V7) // Slider on V7 for setting morning end time
{
  morningEnd = param.asInt(); // Set morningEnd to slider value
}

BLYNK_WRITE(V8) // Slider on V8 for setting evening start time
{
  eveningStart = param.asInt(); // Set eveningStart to slider value
}

BLYNK_WRITE(V9) // Slider on V9 for setting evening end time
{
  eveningEnd = param.asInt(); // Set eveningEnd to slider value
}

BLYNK_WRITE(V4) // Slider on V4 for setting the move limit
{
  moveLimit = param.asInt(); // Set moveLimit to slider value
}

BLYNK_WRITE(V5) // Button on V5 for resetting the ESP32
{
  int pinValue = param.asInt();
  if (pinValue == 1) { // If button is pressed
    ESP.restart(); // Restart the ESP32
  }
}

bool canMoveStepper() {
  DateTime now = rtc.now();

  // Reset move count at midnight
  if (now.day() != lastMoveDate.day()) {
    moveCount = 0;
  }

  // Check if current time is within allowed intervals and move count is less than moveLimit
  bool morningWindow = now.hour() >= morningStart && now.hour() < morningEnd;
  bool eveningWindow = now.hour() >= eveningStart && now.hour() < eveningEnd;

  if ((morningWindow || eveningWindow) && moveCount < moveLimit) {
    return true;
  }
  return false;
}

void moveStepper() {
  // Display "Processing" message on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Processing");
  
  // Move the stepper motor
  int steps = -1024;
  stepper.step(steps); // Move 1024 steps (full revolution for 28BYJ-48)

  // Increment move count and update last move date
  moveCount++;
  lastMoveDate = rtc.now();

  // Display success message and then return to the main display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sukses!");
  delay(2000);
  displayTime();
}

void resetMoveCount() {
  moveCount = 0;
  lastMoveDate = rtc.now(); // Update last move date to now
}

void displayTime() {
  DateTime now = rtc.now();

  // Display the current time on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  if (canMoveStepper()) {
    lcd.print("Obat sudah siap");
    Blynk.logEvent("obat", "Obat sudah siap nih!, Ambil obatnya yuk");
  } else {
    lcd.print("Time: ");
    lcd.setCursor(0, 1);
    lcd.print(now.hour());
    lcd.print(':');
    if (now.minute() < 10) {
      lcd.print('0');
    }
    lcd.print(now.minute());
    lcd.print(':');
    if (now.second() < 10) {
      lcd.print('0');
    }
    lcd.print(now.second());
  }
}

void checkDistance() {
  // Trigger the sensor by setting the trigger pin high for 10 microseconds
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Read the echo pin and calculate the distance
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  // Check if the distance is below 15 cm and can move the stepper
  if (distance < 7 && canMoveStepper()) {
    moveStepper();
  }
}
