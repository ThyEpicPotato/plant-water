// Tape pulls
// https://www.arduino.cc/reference/en/libraries/queue/
// https://learn.sparkfun.com/tutorials/soil-moisture-sensor-hookup-guide
// 892 Max moisture
// ### Min moisture

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

float Queue::avgDaylightHours() {
  float avg = 0;

  for (int i = 0; i < length; i++) {
    if (data[i] >= 60) {
      avg += 1;
    }
  }

  avg = avg / float(length);

  avg *= 24;

  return avg;
}

DHT dht(DHTPIN, DHTTYPE);

// Variables
int moist_val = 0;
int soilPin = A1;
int soilPower = 5;
int pumpPin = 3;
float f = 0; // Latest temp reading
float h = 0; // Latest Humidity Reading
int threshold = 892;
int pump_flag = 0;

Queue tempData;
Queue humidData;
Queue lightData;

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

const unsigned char tasksNum = 7;
task tasks[tasksNum];

// Task Periods
const unsigned long periodLCDOutput = 100; // 0.1 sec
const unsigned long periodJoystickInput = 100; // 0.1 sec
const unsigned long periodTempHumidInput = 3600000; // Every 60 mins 
const unsigned long periodSoilInput = 900000; // 15 min
const unsigned long periodLightInput = 3600000; // 60 min
const unsigned long periodPumpController = 1000; // 0.1 sec 
const unsigned long periodDisplayController = 100; // 0.1s

// GCD 
const unsigned long tasksPeriodGCD = 100;

// Task Function Definitions
int TickFct_LCDOutput(int state);
int TickFct_JoystickInput(int state);
int TickFct_TempHumidInput(int state);
int TickFct_SoilInput(int state);
int TickFct_LightInput(int state);
int TickFct_PumpController(int state);
int TickFct_DisplayController(int state);

// Task Enumeration Definitions
enum LO_States {LO_init, LO_Update}; //LCD Output
enum DC_States {DC_init, 
                DC_Temp, DC_Temp_To_Threshold, DC_Temp_To_Humidity,
                DC_Threshold, DC_Threshold_To_Temp, DC_Threshold_To_Light, 
                DC_Light, DC_Light_To_Threshold, DC_Light_To_Humidity,
                DC_Humidity, DC_Humidity_To_Light, DC_Humidity_To_Temp}; // Display Controller
enum JI_States {JI_init, JI_Sample}; // Joystick Input
enum TH_States {TH_init, TH_Sample}; // Temperature Humidity
enum SI_States {SI_init, SI_Sample}; // Soil Input
enum LI_States {LI_init, LI_Sample}; // Light Input
enum PC_States {PC_init, PC_Wait, PC_On}; // Pump Controller
enum JS_Positions {Up, Down, Left, Right, Neutral} JS_Pos = Neutral;

void LCDWriteLines(String line1, String line2) {
  lcd.clear();          
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

int readSoil()
{
  digitalWrite(soilPower, HIGH);
  delay(10);
  moist_val = analogRead(soilPin); 
  digitalWrite(soilPower, LOW);
  return moist_val;
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
int TickFct_DisplayController(int state) {
  switch (state) { // State Transitions
    case DC_init:
      state = DC_Temp;
      break;

    //Temp states
    case DC_Temp:
      if(JS_Pos == Left){
        state = DC_Temp_To_Threshold;
      }
      if(JS_Pos == Right){
        state = DC_Temp_To_Humidity;
      }
      break;
    case DC_Temp_To_Threshold:
      if(JS_Pos == Neutral){
        state = DC_Threshold;
      }
      break;
    case DC_Temp_To_Humidity:
      if(JS_Pos == Neutral){
        state = DC_Humidity;
      }
      break;
      
    //Threshold states
    case DC_Threshold:
      if(JS_Pos == Left){
        state = DC_Threshold_To_Light;
      }
      if(JS_Pos == Right){
        state = DC_Threshold_To_Temp;
      }
      break;
    case DC_Threshold_To_Light:
      if(JS_Pos == Neutral){
        state = DC_Light;
      }
      break;
    case DC_Threshold_To_Temp:
      if(JS_Pos == Neutral){
        state = DC_Temp;
      }
      break;

    //Light states
    case DC_Light:
      if(JS_Pos == Left){
        state = DC_Light_To_Humidity;
      }
      if(JS_Pos == Right){
        state = DC_Light_To_Threshold;
      }
      break;
    case DC_Light_To_Humidity:
      if(JS_Pos == Neutral){
        state = DC_Humidity;
      }
      break;
    case DC_Light_To_Threshold:
      if(JS_Pos == Neutral){
        state = DC_Threshold;
      }
      break;

    //Humidity
    case DC_Humidity:
      if(JS_Pos == Left){
        state = DC_Humidity_To_Temp;
      }
      if(JS_Pos == Right){
        state = DC_Humidity_To_Light;
      }
      break;
    case DC_Humidity_To_Temp:
      if(JS_Pos == Neutral){
        state = DC_Temp;
      }
      break;
    case DC_Humidity_To_Light:
      if(JS_Pos == Neutral){
        state = DC_Light;
      }
      break;
    default:
      state = DC_init;
      break;
  }

  switch (state) { // State Actions
    case DC_init:
      break;

    //Temp states
    case DC_Temp:
      line1 = "Temp: ";
      line2 = "Avg Temp:";
      break;
    case DC_Temp_To_Threshold:
      line1 = "Temp: ";
      line2 = "Avg Temp:";
      break;
    case DC_Temp_To_Humidity:
      line1 = "Temp: ";
      line2 = "Avg Temp:";
      break;

    //Threshold states
    case DC_Threshold:
      line1 = "To Do";
      line2 = "Threshold";
      break;
    case DC_Threshold_To_Light:
      line1 = "To Do";
      line2 = "Threshold";
      break;
    case DC_Threshold_To_Temp:
      line1 = "To Do";
      line2 = "Threshold";
      break;

    //Light states
    case DC_Light:
      line1 = "Measured Light: ";
      line2 = "Avg Light: ";
      break;
    case DC_Light_To_Humidity:
      line1 = "Measured Light: ";
      line2 = "Avg Light: ";
      break;
    case DC_Light_To_Threshold:
      line1 = "Measured Light: ";
      line2 = "Avg Light: ";
      break;

    //Humidity
    case DC_Humidity:
      line1 = "Humidity: ";
      line2 = "Avg Humidity: ";
      break;
    case DC_Humidity_To_Temp:
      line1 = "Humidity: ";
      line2 = "Avg Humidity: ";
      break;
    case DC_Humidity_To_Light:
      line1 = "Humidity: ";
      line2 = "Avg Humidity: ";
      break;

    default:
      line1 = "Default case";
      line2 = "Possible bug?";
      break;
  }
  return state;
}
// Task 4 ////////////////////////////////////////////////////////////////////////////////////////////////////////
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
      
      float humidity = dht.readHumidity();
      h = humidity;
      humidData.push(humidity);

      Serial.print("Humidity: ");
      Serial.println(humidity); 
      Serial.print("Humidity ");     
      humidData.output();
    break;
  }
  
  return state;
}

// Task 5 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_SoilInput(int state) {
  switch (state) { // State Transitions
    case SI_init:
    state = SI_Sample;
    break;
    case SI_Sample:
    break;
  }

   switch (state) { // State Actions
    case SI_Sample:
      moist_val = readSoil();
      if (moist_val < threshold) {
        pump_flag = 1;
      }
      Serial.print("Moisture: ");
      Serial.println(moist_val);
    break;
  }
  
  return state;
}

// Task 6 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_LightInput(int state) {
  switch (state) { // State Transitions
    case LI_init:
    state = LI_Sample;
    //Serial.println("test");
    break;
    case LI_Sample:
    break;
  }

   switch (state) { // State Actions
    case LI_Sample:
      int light = analogRead(A0);
      lightData.push(light);

      Serial.print("Light: ");
      Serial.println(light);
      Serial.print("Light ");
      lightData.output();
      Serial.print("Average Daylight: ");
      Serial.println(lightData.avgDaylightHours());
    
    break;
  }
  
  return state;
}

int i = 0;

// Task 7 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_PumpController(int state) {
  switch (state) { // State Transitions
    case PC_init:
      state = PC_Wait;
    break;

    case PC_Wait:
      if (pump_flag == 0) {
        state = PC_Wait;
      }
      else if (pump_flag == 1) {
        state = PC_On;
      }
    break;

    case PC_On:
      if (i < 60) {
        state = PC_On;
      }
      else if (i >= 60) {
        state = PC_Wait;
        pump_flag = 0;
      }
  }

   switch (state) { // State Actions
    case PC_Wait:
      digitalWrite(pumpPin, LOW);
      Serial.println("Pump Off");
      i = 0;
    break;

    case PC_On:
      //digitalWrite(pumpPin, HIGH);
      Serial.println("Pump On");
      i++;
    break;
  }
  
  return state;
}

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
  tasks[i].state = SI_init;
  tasks[i].period = periodSoilInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_SoilInput;
  ++i;
  tasks[i].state = LI_init;
  tasks[i].period = periodLightInput;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_LightInput;
  ++i;
  tasks[i].state = PC_init;
  tasks[i].period = periodPumpController;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_PumpController;
  ++i;
  tasks[i].state = DC_init;
  tasks[i].period = periodDisplayController;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_DisplayController;
  
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
  digitalWrite(pumpPin, LOW);
}

void loop() 
{  
  f = dht.readTemperature(true); // Only works properly in loop for some reason
  //digitalWrite(pumpPin, HIGH);
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