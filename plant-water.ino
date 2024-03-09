// Tape pulls
// https://www.arduino.cc/reference/en/libraries/queue/
// https://learn.sparkfun.com/tutorials/soil-moisture-sensor-hookup-guide

#include "DHT.h"
#include "Timer.h"
#include "Adafruit_LiquidCrystal.h"
#include "Queue.h"

#define DHTPIN 2
#define DHTTYPE DHT11

Queue::Queue() {
  length = 0;
  index = 0;
}

void Queue::push(float value) {
  if (length < 72) {
    data[index] = value;
    index++;
    length++;
  }
  else {
    data[index] = value;
    index++;
  }

  if (index > 71) {
    index = 0;
  } 
}

float Queue::getAverage() {

  float avg = 0;

  for (int i = 0; i < length; i++) {
    avg += data[i];
  }

  avg = avg / float(length);

  return avg;
}

void Queue::output() {
  
  Serial.print("Values: [");

  for (int i = 0; i < length; i++) {
    Serial.print(data[i]);
    Serial.print(", ");
  }

  Serial.println("]");
}

DHT dht(DHTPIN, DHTTYPE);

// Variables
int moist_val = 0;
int soilPin = A1;
int soilPower = 5;
int pumpPin = 3;
float f = 0;

Queue tempData;
Queue humidData;

String line1 = "Line 1 temp";
String line2 = "Line 2 temp";

// LCD variables
const int rs = 12, en = 11, d4 = 9, d5 = 8, d6 = 7, d7 = 6;
Adafruit_LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

typedef struct task {
   int state;                  // Tasks current state
   unsigned long period;       // Task period
   unsigned long elapsedTime;  // Time elapsed since last task tick
   int (*TickFct)(int);        // Task tick function
} task;

const unsigned char tasksNum = 3;
task tasks[tasksNum];

// Task Periods
const unsigned long periodLCDOutput = 100;
const unsigned long periodJoystickInput = 100;
const unsigned long periodTempHumidInput = 3600000; // Every 60 mins 
//const unsigned long periodSoundOutput = 100;
//const unsigned long periodController = 100;
//const unsigned long periodCursor = 100;

// GCD 
const unsigned long tasksPeriodGCD = 100;

// Task Function Definitions
int TickFct_LCDOutput(int state);
int TickFct_JoystickInput(int state);
//int TickFct_SoundOutput(int state);
//int TickFct_Controller(int state);

// Task Enumeration Definitions
enum LO_States {LO_init, LO_Update}; //LCD Output
enum JI_States {JI_init, JI_Sample}; // Joystick Input
enum TH_States {TH_init, TH_Sample}; // Temperature Humidity
//enum SO_States {SO_init, SO_SoundOn, SO_SoundOff};
//enum C_States {C_init, C_Off, C_Song1_Trans, C_Song1, C_Song2, C_Song3, C_Song2_Trans, C_Song3_Trans};
enum JS_Positions {Up, Down, Left, Right, Neutral} JS_Pos = Neutral;
//enum CP_States {CP_init, CP_TL, CP_TR, CP_BL, CP_BR};
//enum Cursor_Positions {TopLeft, TopRight, BotLeft, BotRight} C_Pos = BotRight;
//int buttonState = 1;

void LCDWriteLines(String line1, String line2) {
  lcd.clear();          
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

void TimerISR() {
  /*
  if (JS_Pos == Up) {
    Serial.println("Up");
  }
  else if (JS_Pos == Down) {
    Serial.println("Down");
  }
  else if (JS_Pos == Left) {
    Serial.println("Left");
  }
  else if (JS_Pos == Right) {
    Serial.println("Right");
  }
  else if (JS_Pos == Neutral) {
    Serial.println("Neutral");
  }
  */
  
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}

// Task 1 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_JoystickInput(int state) {
  switch (state) { // State Transitions
    case JI_init:
    state = JI_Sample;
    break;
    case JI_Sample:
    break;
  }

   switch (state) { // State Actions
    case JI_Sample:
      if ((analogRead(A3) > 700) && (analogRead(A3) > analogRead(A2)) && ((1023 - analogRead(A3)) < analogRead(A2))) {
        JS_Pos = Left;
      }
      else if ((analogRead(A2) > 700) && (analogRead(A2) > analogRead(A3)) && ((1023 - analogRead(A2)) < analogRead(A3))) {
        JS_Pos = Down;
      }
      else if ((analogRead(A3) < 300) && (analogRead(A3) < analogRead(A2)) && (analogRead(A3) < (1023 - analogRead(A2)))) {
        JS_Pos = Right;  
      }
      else if ((analogRead(A2) < 300) && (analogRead(A2) < analogRead(A3)) && (analogRead(A2) < (1023 - analogRead(A3)))) {
        JS_Pos = Up;
        
      }
      else {
        JS_Pos = Neutral;
      }
    break;
  }
  
  return state;
}

// Task 2 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_LCDOutput(int state) {
  switch (state) { // State Transitions
    case LO_init:
      state = LO_Update;
    break;

    case LO_Update:
      state = LO_Update;
    break;
  }

  switch (state) { // State Actions
    case LO_Update:
      LCDWriteLines(line1, line2);
    break;
  }
  return state;
}
// Task 3 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_TempHumidInput(int state) {
  switch (state) { // State Transitions
    case TH_init:
    state = TH_Sample;
    break;
    case TH_Sample:
    break;
  }

   switch (state) { // State Actions
    case TH_Sample:
      float fahrenheit = f;
      tempData.push(fahrenheit);

      Serial.print("Temp: ");
      Serial.println(fahrenheit);
      Serial.print("Temp ");
      tempData.output();
      
      float humidty = dht.readHumidity();
      humidData.push(humidty);

      Serial.print("Humidity: ");
      Serial.println(humidty); 
      Serial.print("Humidity ");     
      humidData.output();
    break;
  }
  
  return state;
}

// Task 4 ////////////////////////////////////////////////////////////////////////////////////////////////////////

// Task 5 ////////////////////////////////////////////////////////////////////////////////////////////////////////

void InitializeTasks() {
  unsigned char i=0;
  tasks[i].state = LO_init;
  tasks[i].period = periodLCDOutput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_LCDOutput;
  ++i;
  tasks[i].state = JI_init;
  tasks[i].period = periodJoystickInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_JoystickInput;
  ++i;
  tasks[i].state = TH_init;
  tasks[i].period = periodTempHumidInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_TempHumidInput;
  ++i;
  /*
  tasks[i].state = C_init;
  tasks[i].period = periodController;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Controller;
  ++i;
  tasks[i].state = CP_init;
  tasks[i].period = periodCursor;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_Cursor;
  */
}

void setup() 
{
  InitializeTasks();
  pinMode(4, INPUT_PULLUP); //Joystick push 

  TimerSet(tasksPeriodGCD);
  TimerOn();

  Serial.begin(9600);

  dht.begin();
  lcd.begin(16, 2);

  pinMode(pumpPin, OUTPUT);
  pinMode(soilPower, OUTPUT); //Set D7 as an OUTPUT
  digitalWrite(soilPower, LOW); //Set to LOW so no power is flowing through the sensor
  Serial.println("");
  //digitalWrite(pumpPin, HIGH);

  
}

void loop() 
{  
  f = dht.readTemperature(true);
  /*
        Serial.print(F("Temperature: "));
        Serial.print(f);
        Serial.println(F("°F"));
  
  float h = dht.readHumidity();
  //Celsius
  float t = dht.readTemperature();
  //Fahrenheit
  f = dht.readTemperature(true);

  Serial.print(F("Light Level: "));
  Serial.println(analogRead(A0));
  Serial.print("Soil Moisture = ");    
  Serial.println(readSoil());
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.println(F("%"));
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.println(F("°F"));
  Serial.println("");

  LCDWriteLines(line1, line2);

  Serial.print(analogRead(A2));
  Serial.print(' ');
  Serial.println(analogRead(A3));

  delay(1000); 
  
  //digitalWrite(pumpPin, HIGH);
  
  */
}
//This is a function used to get the soil moisture content
int readSoil()
{
  digitalWrite(soilPower, HIGH); //turn D7 "On"
  delay(10); //wait 10 milliseconds 
  moist_val = analogRead(soilPin); //Read the SIG value form sensor 
  digitalWrite(soilPower, LOW); //turn D7 "Off"
  return moist_val; //send current moisture value
}
