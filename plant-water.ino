// Tape pulls
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
  
  //Serial.print("Values: [");

  for (int i = 0; i < length; i++) {
    //Serial.print(data[i]);
    //Serial.print(", ");
  }

  //Serial.println("]");
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
int screen_flag = 0;
int percent = 50;

Queue tempData;
Queue humidData;
Queue lightData;

char buffer[20];

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
const unsigned long periodTempHumidInput = 5000; // Every 60 mins 3600000
const unsigned long periodSoilInput = 10000; // 2 min 120000
const unsigned long periodLightInput = 5000; // 60 min 3600000
const unsigned long periodPumpController = 1000; // 0.1 sec 
const unsigned long periodDisplayController = 100; // 0.1 sec

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
enum DC_States {DC_init, DC_Temp, DC_ToTemp, DC_Threshold, DC_ToThreshold, DC_Light, DC_ToLight, DC_Humidity, DC_ToHumidity}; // Display Controller
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
  
  
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        ////Serial.print("SM: ");
        ////Serial.println(i);
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
      /*
      Serial.print(analogRead(A2));
      Serial.print(" ");
      Serial.println(analogRead(A3));
      */
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
      if (screen_flag == 0) {
        line1 = "Line 1 temp";
        line2 = "Line 2 temp";
      }
      else if (screen_flag == 1) {
        dtostrf(f, 6, 2, buffer);
        line1 = "Temp: ";
        line1 += buffer;
        line1 += " F";

        dtostrf(tempData.getAverage(), 6, 2, buffer);
        line2 = "Avg: ";
        line2 += buffer;
        line2 += " F";
      }
      else if (screen_flag == 2) {
        dtostrf(h, 6, 2, buffer);
        line1 = "Humidity:";
        line1 += buffer;
        line1 += "%";

        dtostrf(humidData.getAverage(), 6, 2, buffer);
        line2 = "Avg: ";
        line2 += buffer;
        line2 += "%";
      }
      else if (screen_flag == 3) {
        line1 = "Avg # Hrs Light";
        line2 = "/Day:";
        dtostrf(lightData.avgDaylightHours(), 6, 2, buffer);
        line2 += buffer;
        line2 += " Hrs";
      }
      else if (screen_flag == 4) {
        line1 = "Moist: ";
        line1.concat(moist_val);
        line2 = "Threshold: ";
        line2.concat(threshold);
        //line2 += "%";
      }
      LCDWriteLines(line1, line2);
    break;
  }
  return state;
}

// Task 3 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_DisplayController(int state) {
  ////Serial.print("State: ");
  ////Serial.println(state);
  switch (state) { // State Transitions
    case DC_init:
      state = DC_Temp;
      break;

    //Temp states
    case DC_Temp:
      if(JS_Pos == Left){
        state = DC_ToThreshold;
      }
      else if(JS_Pos == Right){
        state = DC_ToHumidity;
      }
      else {
        state = DC_Temp;
      }
      break;
    case DC_ToTemp:
      if(JS_Pos != Neutral){
        state = DC_ToTemp;
      }
      else {
        state = DC_Temp;
      }
      break;
      
    //Threshold states
    case DC_Threshold:
      if(JS_Pos == Left){
        state = DC_ToLight;
      }
      else if(JS_Pos == Right){
        state = DC_ToTemp;
      }
      else {
        state = DC_Threshold;
      }
      break;
    case DC_ToThreshold:
      if(JS_Pos != Neutral){
        state = DC_ToThreshold;
      }
      else {
        state = DC_Threshold;
      }
      break;

    //Light states
    case DC_Light:
      if(JS_Pos == Left){
        state = DC_ToHumidity;
      }
      else if(JS_Pos == Right){
        state = DC_ToThreshold;
      }
      else {
        state = DC_Light;
      }
      break;
    case DC_ToLight:
      if(JS_Pos != Neutral){
        state = DC_ToLight;
      }
      else {
        state = DC_Light;
      }
      break;

    //Humidity
    case DC_Humidity:
      if(JS_Pos == Left){
        state = DC_ToTemp;
      }
      else if(JS_Pos == Right){
        state = DC_ToLight;
      }
      else {
        state = DC_Humidity;
      }
      break;
    case DC_ToHumidity:
      if(JS_Pos != Neutral){
        state = DC_ToHumidity;
      }
      else {
        state = DC_Humidity;
      }
      break;
  }

  switch (state) { // State Actions
    case DC_init:
      break;

    //Temp states
    case DC_Temp:
      screen_flag = 1;
      break;
    case DC_ToTemp:
      screen_flag = 1;
      break;

    //Threshold states
    case DC_Threshold:
      screen_flag = 4;
      //Serial.println(JS_Pos);
      if (JS_Pos == Up) {
        if (percent < 100) {
          percent += 1;
          threshold = map(percent, 0, 100, 0, 1023);
        }
      }
      else if (JS_Pos == Down) {
        if (percent > 0) {
          percent -= 1;
          threshold = map(percent, 0, 100, 0, 1023);
        }
      }
      break;
    case DC_ToThreshold:
      screen_flag = 4;
      break;

    //Light states
    case DC_Light:
      screen_flag = 3;
      break;
    case DC_ToLight:
      screen_flag = 3;
      break;

    //Humidity
    case DC_Humidity:
      screen_flag = 2;
      break;
    case DC_ToHumidity:
      screen_flag = 2;
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

      //Serial.print("Temp: ");
      //Serial.println(fahrenheit);
      //Serial.print("Temp ");
      tempData.output();
      
      float humidity = dht.readHumidity();
      h = humidity;
      humidData.push(humidity);

      //Serial.print("Humidity: ");
      //Serial.println(humidity); 
      //Serial.print("Humidity ");     
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
      threshold = map(percent, 0, 100, 0, 1023);
      Serial.print(moist_val);
      Serial.print(" ");
      Serial.println(threshold);
      if (moist_val < threshold) {
        pump_flag = 1;
        Serial.println("flag up");
      }
      //Serial.print("Moisture: ");
      //Serial.println(moist_val);
    break;
  }
  
  return state;
}

// Task 6 ////////////////////////////////////////////////////////////////////////////////////////////////////////
int TickFct_LightInput(int state) {
  switch (state) { // State Transitions
    case LI_init:
    state = LI_Sample;
    ////Serial.println("test");
    break;
    case LI_Sample:
    break;
  }

   switch (state) { // State Actions
    case LI_Sample:
      int light = analogRead(A0);
      lightData.push(light);

      //Serial.print("Light: ");
      //Serial.println(light);
      //Serial.print("Light ");
      lightData.output();
      //Serial.print("Average Daylight: ");
      //Serial.println(lightData.avgDaylightHours());
    
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
        Serial.println("flag recieved");
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
      digitalWrite(pumpPin, HIGH);
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
  //Serial.println("");
  digitalWrite(pumpPin, LOW);
}

void loop() 
{  
  f = dht.readTemperature(true); // Only works properly in loop for some reason
}