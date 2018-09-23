#include <OneWire.h> // DS18B20 Temp probe
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h> //LCD w/ i2C board
#include <elapsedMillis.h>

#define BACKLIGHT_PIN 13
// Temp probes
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 12


// Variables
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // LiquidCrystal_I2C lcd(0x3f);  // Set the LCD I2C address


/// Temp Probe initialization /// 
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Assign address manually. The addresses below will beed to be changed
// to valid device addresses on your bus. Device address can be retrieved
// by using either oneWire.search(deviceAddress) or individually via
// sensors.getAddress(deviceAddress, index)
DeviceAddress thermometer1 = { 0x28, 0xFF, 0xFD, 0xD7, 0x80, 0x14, 0x02, 0x8C };
DeviceAddress thermometer2 = { 0x28, 0xFF, 0xFF, 0xDE, 0x24, 0x17, 0x03, 0xFF };

int targetTemp1 = 45;
int targetTemp2 = 65;

// Temp Sensor
int TempRequestDelayInMillis = 750;
unsigned long  lastTempRequest = millis();
float temperature1 = 0.0;
float temperature2 = 0.0;

//// Buttons
//const int upButtonPin = 3;
//const int downButtonPin = 5;
//const int setButtonPin = 4;
//int upButtonState = 0;
//int downButtonState = 0;
//int setButtonState = 0;
//int lastSetButtonState = 0;

// Buttons
// still bounces a bit
const int upButtonPin = 3;
const int downButtonPin = 5;
const int setButtonPin = 4;
const int shortButtonDelay = 100; // delay when changing target temp quickly
const int longButtonDelay = 600; // delay when changing target temp slowly
const int noButtonDelay = 100; // not set to 0 to prevent bouncing
const int countBeforeFastTempChange = 3; // when holding down a button, wait this many seconds before changing the target temp quickly

int setButtonState = 0;
int lastSetButtonState = 0;
int upButtonState = 0;
int downButtonState = 0;
int tempIncreaseCounter = 0;
int tempDecreaseCounter = 0;
unsigned long tempChangeDelay = 0; //unsigned long because it is going to hold milliseconds
unsigned long tempIncreaseTime = 0;
unsigned long tempDecreaseTime = 0;
unsigned long lastTempChange = 0;
char lastButton = 'x'; // last button was up or down. When going from one to the other, temp will change slowly



// 0 = homeMode
// 1 = setTemp
int displayMode = 0;

void setup() {
  Serial.begin(9600);
    
  // Buttons
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  pinMode(setButtonPin, INPUT);

  // LCD
  // Switch on the backlight
  pinMode ( BACKLIGHT_PIN, OUTPUT );
  digitalWrite ( BACKLIGHT_PIN, HIGH );
  lcd.begin(16,2); // initialize the lcd 

  // Temp Sensor
  sensors.begin();
  sensors.setResolution(thermometer1, TEMPERATURE_PRECISION);
  sensors.setResolution(thermometer2, TEMPERATURE_PRECISION);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  pinMode(13, OUTPUT); 
}

/////////////////////
///// FUNCTIONS /////
/////////////////////

//=========//
// getTemp //
//=========//
float getTemp(DeviceAddress deviceAddress){
    float tempF = sensors.getTempF(deviceAddress);
    sensors.requestTemperatures(); 
    lastTempRequest = millis(); 
    return tempF;
} // END getTemp


//==================//
// CurrentTempToLCD //
//==================//
void CurrentTempToLCD() {
  lcd.home ();
  lcd.print ("Temp 1: ");
  lcd.print(temperature1,1);
  lcd.setCursor ( 0, 1 );
  lcd.print ("Temp 2: ");
  lcd.print(temperature2,1);
}


//===============//
// Check Buttons //
//===============//

int setTargetTemp(int targetTemp) {
  upButtonState = digitalRead(upButtonPin);
  downButtonState = digitalRead(downButtonPin);
  
  if (upButtonState == HIGH) {
    tempDecreaseCounter = 0;
    // set button delay
    // look at debounce script for this. Set variable to high / low button state. 
    if (lastButton != 'u') {               // single button press - slight delay to prevent bouncing. Change temperature fast with multuple button presses.
        tempChangeDelay = noButtonDelay;
    } else if (tempIncreaseCounter > countBeforeFastTempChange) { // button held long enough to trigger fast temp change
        tempChangeDelay = shortButtonDelay;
    } else {tempChangeDelay = longButtonDelay;}

    // do the work
    if ((millis() - tempIncreaseTime) > tempChangeDelay) {
        targetTemp++;
        lastButton = 'u';
        tempIncreaseCounter++; // set to 0 in 'button down' and on no button press
        tempIncreaseTime = millis();
     //   lcd.clear();
    }
    lastTempChange = millis();
  } else if (downButtonState == HIGH) {
    tempIncreaseCounter = 0;
    // set button delay
    // look at debounce script for this. Set variable to high / low button state. 
    if (lastButton != 'd') {               // single button press - slight delay to prevent bouncing. Change temperature fast with multuple button presses.
        tempChangeDelay = noButtonDelay;
    } else if (tempDecreaseCounter > countBeforeFastTempChange) { // button held long enough to trigger fast temp change
        tempChangeDelay = shortButtonDelay;
    } else {tempChangeDelay = longButtonDelay;}

    // do the work
    if ((millis() - tempDecreaseTime) > tempChangeDelay) { 
      targetTemp--;
      lastButton = 'd';
      tempDecreaseCounter++; // set to 0 in 'button down' and on no button press
      tempDecreaseTime = millis();
     // lcd.clear();
    }
    lastTempChange = millis();
  } else {
      tempIncreaseCounter = 0;
      tempDecreaseCounter = 0;
      lastButton = 'x';
  }
  return targetTemp;
}

void displayTargetTempCheck(int targetTemp1, int targetTemp2) {
  downButtonState = digitalRead(downButtonPin);
  upButtonState = digitalRead(upButtonPin);
  while (downButtonState == 1 or upButtonState == 1) {
    downButtonState = digitalRead(downButtonPin);
    upButtonState = digitalRead(upButtonPin);
    lcd.home ();                   // go home
    lcd.print ("Target 1: ");
    lcd.print(targetTemp1,1);
    lcd.setCursor ( 0, 1 );        // go to the next line
    lcd.print ("Target 2: ");
    lcd.print(targetTemp2,1);
    lcd.setCursor ( 15, 0 );
    lcd.print (" ");
  }
}

//void targetTempToLCD(int targetTemp1, int targetTemp2) {
//    downButtonState = digitalRead(downButtonPin);
//    upButtonState = digitalRead(upButtonPin);
//    lcd.home ();                   // go home
//    lcd.print ("Target 1: ");
//    lcd.print(targetTemp1,1);
//    lcd.setCursor ( 0, 1 );        // go to the next line
//    lcd.print ("Target 2: ");
//    lcd.print(targetTemp2,1);
//    lcd.setCursor ( 15, 0 );
//    lcd.print (" ");
//}

void homeMode() {
  displayTargetTempCheck(targetTemp1,targetTemp2); // check for up/down button press from home screen
  CurrentTempToLCD();
}

void setMode() {
  int timeOutDelay = 30000;
  unsigned long lastButtonPressTime = millis();
  int tempToChange = 0; //which sensor to change target temp for
  char changeIndicatorTherm1 = ' ';
  char changeIndicatorTherm2 = ' ';
  while (displayMode == 1){
    if ((millis() - lastButtonPressTime) > timeOutDelay){displayMode = 0;}
    setButtonState = digitalRead(setButtonPin);
    if (setButtonState != lastSetButtonState) {
      lastButtonPressTime = millis();
      if (setButtonState == 1) {tempToChange++;}
      lastSetButtonState = setButtonState;
    }
    if (tempToChange == 0){
      changeIndicatorTherm1 = '*';
      changeIndicatorTherm2 = ' ';
      int tempTemp = targetTemp1;
      targetTemp1 = setTargetTemp(targetTemp1);
      if (tempTemp != targetTemp1){lcd.setCursor (10,0);lcd.print ("   ");} //clear last temp from LCD
    } else if (tempToChange == 1){
      changeIndicatorTherm1 = ' ';
      changeIndicatorTherm2 = '*';
      int tempTemp = targetTemp2;
      targetTemp2 = setTargetTemp(targetTemp2);
      if (tempTemp != targetTemp2){lcd.setCursor (10,1);lcd.print ("   ");} //clear last temp from LCD
    } else {displayMode = 0;}
    lcd.home ();
    lcd.print ("Target 1: ");
    lcd.print (targetTemp1);
    lcd.setCursor ( 15, 0 );
    lcd.print (changeIndicatorTherm1);
    lcd.setCursor ( 0, 1 );        // go to the next line
    lcd.print ("Target 2: ");
    lcd.print (targetTemp2);
    lcd.setCursor ( 15, 1 );
    lcd.print (changeIndicatorTherm2);
  }
  lcd.clear();
}

///////////////
// MAIN LOOP //
///////////////

void loop() {
  // check for display mode change
  setButtonState = digitalRead(setButtonPin);
  if (setButtonState != lastSetButtonState) {
    if (setButtonState == 1) {displayMode++;}
    lastSetButtonState = setButtonState;
  }
  if (displayMode == 0){
      homeMode();
    } else if (displayMode == 1){
      setMode();
  }
  if (millis() - lastTempRequest >= TempRequestDelayInMillis) {// wait for temp to be ready to collect
    temperature1 = getTemp(thermometer1);
    temperature2 = getTemp(thermometer2);
  }
}
