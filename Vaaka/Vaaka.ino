#include <LiquidCrystal.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
#define DOUT 8
#define CLK 9
// WATCH DOG TIMER LISÄTÄÄN VIIKKO 10 //sivu 59 ATMEGA48A
// AJASTIN RESET // sivu 125 ATMEGA48A
// INTERRUPT // sivu 23 ATMEGA48A
const int scaleTare = 10;
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
HX711_ADC LoadCell(DOUT,CLK);

        //VARIABLES 


float average = 0;
int counter = 0;
unsigned long t = 0;
const int calVal_eepromAdress = 0;
float newCalibrationValue = 0;

//***********************************




void setup() {
bool _check = false;
// Alustetaan I/O pinnit ja jätetään muistutus mitä i/o pinnit vastaa arduinossa


// D7 - D0 = PD7 - PD0

       //76543210
PORTD = B00111100;
      //76543210
DDRD  = B00111100;
       //76543210


// D13 - D8 = PD5 -> PD0      

PORTB = B011111;
       //543210
DDRB  = B011111;
       //543210


// A7 - A0 = PC7-> PC0 

PORTC = B00000000;
       //76543210

DDRC  = B00000000;
       //76543210
  Serial.begin(9600);
  Serial.println("Starting...");
  LoadCell.begin();
  lcd.begin(20,4);
  pinMode(10, INPUT_PULLUP);
  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  // can set to false if no need for tare
  //Taring the scale for 0 longer duration of time improves accuracy
  if(LoadCell.getTareTimeoutFlag()){
    lcd.setCursor(0,0);
    lcd.print("Timeout wirePIN");
    }
  else{
    //1.0 used for initial value for calibration
    lcd.print("Setup Complete");
    delay(500);
    lcd.clear();
    }
    while(!LoadCell.update());
    Serial.print("Calibration value: ");
    Serial.println(LoadCell.getCalFactor());
    Serial.print("HX711 measured conversion time ms: ");
    Serial.println(LoadCell.getConversionTime());
    Serial.print("HX711 measured sampling rate HZ: ");
    Serial.println(LoadCell.getSPS());
    Serial.print("HX711 measured settlingtime ms: ");
    Serial.println(LoadCell.getSettlingTime());
    Serial.println("Do you want to configure the scale y/n");
    Serial.print("EEPROM calibration value is: ");
    Serial.println(EEPROM.get(0,newCalibrationValue));
    lcd.setCursor(0,0);
    lcd.print(newCalibrationValue);
    lcd.setCursor(0,2);
    LoadCell.start(stabilizingTime, _tare);

    lcd.print("Y/N for Calibration");
    while(_check == false){
      if(Serial.available () > 0){
        char inByte = Serial.read();
        if(inByte == 'n'){
          #if defined(ESP8266) || defined(ESP32)
          EEPROM.begin(512);
          #endif
          EEPROM.get(calVal_eepromAdress, newCalibrationValue);
          LoadCell.setCalFactor(newCalibrationValue);
          Serial.println(newCalibrationValue);
          lcd.clear();
          _check = true;

        }
        else if(inByte == 'y'){
        calibrate();
        _check = true;
        }
      }    
    }
}

void loop() {  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 200; 
  if (LoadCell.update()) newDataReady = true;
  
  if(newDataReady){
    if(millis() > t + serialPrintInterval){
      float i = LoadCell.getData();
      lcd.setCursor(0,2);
      lcd.print("Weight:");
      lcd.setCursor(8,2);
      lcd.print(i);
      Serial.print("Load Cell output value: ");
      Serial.println(i);
      average += i;
      newDataReady = 0;
      t = millis();
      }
  }
  if(Serial.available () > 0){
    char inByte = Serial.read();
    if(inByte == 't' || scaleTare == HIGH) LoadCell.tareNoDelay(); // tare
    else if(inByte == 'r') calibrate(); // calibrate
    else if(inByte == 'c') changeSavedCalFactor(); //edit calibration value manually
    }

  if(LoadCell.getTareStatus()== true){
    Serial.println("Tare complete");
  }
}

void calibrate(){

  Serial.println("***");
  Serial.println("Start the calibration");
  Serial.println("Place the load cell on a level stable surface");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset");  

  boolean _resume = false;
  while(_resume == false){
    LoadCell.update();
    if(Serial.available() > 0){
      if(Serial.available() > 0){
        char inByte = Serial.read();
        if(inByte = 't') LoadCell.tareNoDelay();
      }
    }
    if(LoadCell.getTareStatus() == true){
      Serial.println("Tare Complete");
      _resume = true;
    }
  }

  Serial.println("Now place a known mass on the loadcell");
  Serial.println("Now send the weight of this mass (i.e 100.0) from serial monitor ");

  float knownMass = 0;
  _resume = false;
  while(_resume == false){
    if(Serial.available() > 0 ){
      knownMass = Serial.parseFloat();
      if(knownMass != 0 ){
        Serial.print("Known mass is: ");
        Serial.println(knownMass);   
        _resume = true;     
      }
    }        
  }

  LoadCell.refreshDataSet();
  Serial.println(knownMass);
  Serial.println(LoadCell.getNewCalibration(knownMass));
  float newCalibrationValue = LoadCell.getNewCalibration(knownMass);

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch");
  Serial.print("Save this value to EEROM address ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;

  while(_resume == false){
    if(Serial.available() > 0){
      char inByte = Serial.read();
      if(inByte == 'y'){
        #if defined(ESP8266) || defined(ESP32)
          EEPROM.begin(512);
        #endif
          EEPROM.put(calVal_eepromAdress, newCalibrationValue);
        #if defined(ESP8266) || defined(ESP32)
          EEPROM.commit();
        #endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value: ");
        Serial.print(newCalibrationValue);
        Serial.print(" Saved to EEPROM address: ");
        Serial.print(calVal_eepromAdress);
        _resume = true;
      }
      else if(inByte == 'n'){
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
}

void changeSavedCalFactor(){
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
    Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
      #if defined(ESP8266)|| defined(ESP32)
              EEPROM.begin(512);
      #endif
              EEPROM.put(calVal_eepromAdress, newCalibrationValue);
      #if defined(ESP8266)|| defined(ESP32)
              EEPROM.commit();
      #endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}


