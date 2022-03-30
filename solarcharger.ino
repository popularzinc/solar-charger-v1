#include <Wire.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>       

LiquidCrystal_I2C lcd(0x27,20,4);   
 
Adafruit_INA219 ina219;

int mosfet = 9;


uint8_t Battery[8]  = {0x0E, 0x1B, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F};
uint8_t Panel[8]  = {0x1F, 0x15, 0x1F, 0x15, 0x1F, 0x15, 0x1F, 0x00};
uint8_t Pwm[8]  = {0x1D, 0x15, 0x15, 0x15, 0x15,0x15, 0x15, 0x17};
uint8_t Flash[8]  = {0x01, 0x02, 0x04, 0x1F, 0x1F, 0x02, 0x04, 0x08};


#define BATT A3 
#define SOL A2 
#define LOAD A1

bool charge = false;

int target = 7.2;
int pwm = 0;
int sol_min = 9;

float const_current = 700.0;
float current_cut = 135.0;
float const_volt = 7.2;
float trickle = 125.0;

bool bulk = false;
bool absorption = false;
bool Float = false;

String STATE = "X";

float watts = 0.0;
float shuntvoltage = 0;
float busvoltage = 0;
float power_mW = 0;


float current_mA = 0;
float loadvoltage = 0;
float batt_volt = 0;
float sol_volt = 0;
float load_volt = 0;

void setup(void) 
{


  
  Serial.begin(115200);
  while (!Serial) {
      delay(1);
  }
  lcd.init();                    

  // Print a message to the LCD.

  lcd.backlight();                

  lcd.setCursor(0,0);   

  lcd.createChar(0, Battery);
  lcd.createChar(1, Panel);
  lcd.createChar(2, Pwm);
  lcd.createChar(3, Flash);

  
  uint32_t currentFrequency;
    
  Serial.println("Hello!");
  
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  Serial.println("Measuring voltage and current with INA219 ...");
}

void loop(void) 
{

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  power_mW = ina219.getPower_mW();

  current_mA = 0;//ina219.getCurrent_mA();
  loadvoltage = 0;//busvoltage + (shuntvoltage / 1000);
  batt_volt = 0;//(analogRead(BATT) * (5.0 / 1024.0)) * 2;
  sol_volt = 0;//(analogRead(SOL) * (5.0 / 1024.0)) * 3.9;

  for (int i = 0; i <= 100; i++) {
    current_mA += ina219.getCurrent_mA();
  }
  current_mA = (current_mA/100)*-1;

  for (int i = 0; i <= 100; i++) {
    batt_volt += (analogRead(BATT) * (5.0 / 1024.0)) * 2;
  }
  batt_volt = batt_volt/100;

  for (int i = 0; i <= 100; i++) {
    sol_volt += (analogRead(SOL) * (5.0 / 1024.0)) * 4;
  }
  sol_volt = sol_volt/100;

  for (int i = 0; i <= 100; i++) {
    load_volt += (analogRead(LOAD) * (5.0 / 1024.0)) * 4;
  }
  load_volt = load_volt/100;

  absorption = false;
  bulk = false;
  Float = false;
  charge = false;
    
  if(sol_volt>sol_min){
    charge = true;
    if(batt_volt >= target){
      absorption = true;
      if(current_mA <= current_cut){
        absorption = false;
        Float = true;
      }
    }else{
      bulk = true;
    }
  }else{
    charge = false;
    pwm = 0;
    STATE = "X";
  }

  if(charge){
    if(bulk){
      STATE = "BULK";
      if(current_mA > const_current){
        pwm--; 
      }else{
        pwm++;
      }
    }else if(absorption){
      STATE = "ABSORPTION";
      if(load_volt > const_volt){
        pwm--;
      }else{
        pwm++;
      }
    }else if(Float){
      STATE = "FLOAT";
      if(current_mA > trickle){
        pwm--;
      }else{
        pwm++;
      }
    }
    
    if(pwm>255){
      pwm = 255;
    }else if(pwm < 0){
      pwm = 0;
    }
    analogWrite(mosfet,pwm);
  }

  watts = load_volt*(current_mA*10);

  lcd.setCursor(11,0); 
  lcd.print(watts,2); lcd.print(" W"); 

  lcd.setCursor(11,1); 
  lcd.print(load_volt,2); lcd.print(" V"); 

  lcd.setCursor(11,3); 
  lcd.print(STATE);
  
  lcd.setCursor(0,0); 
  //lcd.print("LV :  "); lcd.print(load_volt); lcd.print(" V"); // output voltage
  lcd.write(1); lcd.print(" "); lcd.print(sol_volt); lcd.print(" V "); 
  lcd.setCursor(0,1); 
  lcd.write(0); lcd.print(" "); lcd.print(batt_volt); lcd.print(" V"); // output current
  lcd.setCursor(0,2); 
  lcd.write(3); lcd.print(" "); lcd.print(current_mA,2); lcd.print(" mA"); // output current
  lcd.setCursor(0,3); 
  lcd.write(2); lcd.print(" "); lcd.print(map(pwm,0,255,0,100)); lcd.print("%");

  
  Serial.print("Load Voltage    :  "); Serial.print(load_volt); Serial.print(" V"); // output voltage
  Serial.print("Current         :  "); Serial.print(current_mA); Serial.print(" "); // output current
  Serial.print("Battery Voltage :  "); Serial.print(batt_volt); Serial.print(" V"); // output current
  Serial.print("Solar Voltage   :  "); Serial.print(sol_volt); Serial.print(" V "); Serial.print(pwm); // output current
  Serial.println("");

  //delay(2);
}
