/*
   -----------------------------------------------------------------------------

                                       /\ \                 
                        __   _ __   ___\ \ \____  __  __    
                      /'_ `\/\`'__\/ __`\ \ '__`\/\ \/\ \   
                     /\ \L\ \ \ \//\ \L\ \ \ \L\ \ \ \_\ \  
                     \ \____ \ \_\\ \____/\ \_,__/\/`____ \ 
                      \/___L\ \/_/ \/___/  \/___/  `/___/> \
                        /\____/                       /\___/
                        \_/__/                        \/__/ 

         an Arduino 101 + grove-based Personal Information Display! (^-^)b      

   -----------------------------------------------------------------------------

*/

// for RTC
#include <CurieTime.h>

// for Curie Bluetooth Low Energy (BLE)
#include <CurieBLE.h>

// for Curie inertial measurement unit (BMI160)
#include "CurieIMU.h"

// for Grove LCD RGB Backlight
#include <Wire.h>
#include "rgb_lcd.h"

// for log
#include <math.h>


// pin setup
// NOTE: A0 reserved for randomSeed()
const int thermistor_pin = A1;
const int button_pin = 3;
const int piezo_pin = 4;
const int led_pin = 5;      // must support PWM


BLEPeripheral blePeripheral;
BLEService lcdService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEUnsignedCharCharacteristic redCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEUnsignedCharCharacteristic greenCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEUnsignedCharCharacteristic blueCharacteristic("19B10013-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLECharacteristic messageCharacteristic("19B10014-E8F2-537E-4F6C-D104768A1214", BLEWrite, 20); // TODO: const vs. hard-coded
BLEUnsignedIntCharacteristic timeCharacteristic("19B10015-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

rgb_lcd lcd;

static byte color_r = 0;
static byte color_g = 0;
static byte color_b = 0;


void setup() {

  // randomize based on unconnected pin noise
  randomSeed(analogRead(0));


  // DON'T set RTC - either it's already set because power stayed on
  // between sketches, or we'll default to 0 and set via BLE later

  // TODO: remove
  // set RTC to midnight on 20170101 :tada:
  //setTime(00, 00, 00, 1, 1, 2017);
  // epoch time (default appears to be 0 if not set)
  //setTime(1490315941);
  

  // initialize Curie BLE, services and characteristics
  blePeripheral.setLocalName("HelloCurie");
  // set advertised service UUID
  blePeripheral.setAdvertisedServiceUuid(lcdService.uuid());

  blePeripheral.addAttribute(lcdService);
  blePeripheral.addAttribute(redCharacteristic);
  blePeripheral.addAttribute(greenCharacteristic);
  blePeripheral.addAttribute(blueCharacteristic);
  blePeripheral.addAttribute(messageCharacteristic);
  blePeripheral.addAttribute(timeCharacteristic);

  // (setting of RGB characteristic values moved to UpdateBacklightColor() )


  unsigned char buffer[20] = "";
  messageCharacteristic.setValue(buffer, 20); // TODO: actual value

  blePeripheral.begin();


  // initialize Curie IMU
  CurieIMU.begin();

  // set accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);


  // set up button
  pinMode(button_pin, INPUT);

  // set up piezo and chirp at startup
  pinMode(piezo_pin, OUTPUT);
  digitalWrite(piezo_pin, HIGH);
  delay(10);
  digitalWrite(piezo_pin, LOW);

  // set up and turn on LED (max brightness)
  pinMode(led_pin, OUTPUT);
  analogWrite(led_pin, 255);


  // set up LCD rows/cols
  lcd.begin(16, 2);

  SetRandomBacklightColor();

  lcd.print((const char *) buffer);
}

void loop() {

  /*
        _                   _   
       (_)_ __  _ __  _   _| |_ 
       | | '_ \| '_ \| | | | __|
       | | | | | |_) | |_| | |_ 
       |_|_| |_| .__/ \__,_|\__|
               |_|      
  */
  
  // poll for BLE events
  blePeripheral.poll();

  // set cursor to beginning of second (numbered from 0) row
  lcd.setCursor(0, 1);

  // randomize backlight and switch off LED while pressed
  // TODO: debounce!
  if (digitalRead(button_pin)) {
    SetRandomBacklightColor();
    analogWrite(led_pin, 0);
  } else {
    analogWrite(led_pin, 255);
  }

  // TODO: make BLE/button exclusive/prioritize one or the other?
  if (redCharacteristic.written()) {
    // TODO: error checking?
    color_r = redCharacteristic.value();
    UpdateBacklightColor();
  }
  if (greenCharacteristic.written()) {
    // TODO: error checking?
    color_g = greenCharacteristic.value();
    UpdateBacklightColor();
  }
  if (blueCharacteristic.written()) {
    // TODO: error checking?
    color_b = blueCharacteristic.value();
    UpdateBacklightColor();
  }

  if (messageCharacteristic.written()) {
    // TODO: error checking?
    size_t message_length = messageCharacteristic.valueLength();
    // TODO: 17 -> const
    // note that string literal *includes* \0 termination
    unsigned char message[17] = "                "; // cheap padding for now
    char message_formatted[17];

    // it's possible to receive messages longer than the LCD width
    if (message_length > 16) {
      // truncate for now
      // TODO: handle longer messages via scrolling later...
      message_length = 16;
    };

    // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
    memcpy(message, messageCharacteristic.value(), message_length);

    lcd.setCursor(0, 0);
    lcd.print((const char *) message);
    //lcd.print((const char *) message_formatted);
  }

  // update RTC if new epoch time received
  if (timeCharacteristic.written()) {
    // TODO: error checking?
    setTime(timeCharacteristic.value());
  }
  

  /*
                    _               _   
         ___  _   _| |_ _ __  _   _| |_ 
        / _ \| | | | __| '_ \| | | | __|
       | (_) | |_| | |_| |_) | |_| | |_ 
        \___/ \__,_|\__| .__/ \__,_|\__|
                       |_|       
  */
  
  // TODO: add cycling through states via button
  // TODO: add cycling through states over time
  
//  ShowDebug();
  ShowHome();
//  ShowWeather();
//  ShowTweets();
//  ShowNotifications();
  

  // update RTC characteristic
  timeCharacteristic.setValue(now());
  // TODO: is this expensive? if so, write-only w/b fine...

  delay(100);
}


void ShowHome() {

  // green background
  
  color_r = 31;
  color_g = 255;
  color_b = 31;

  UpdateBacklightColor();

  // TODO: refactor

  // quick RTC demo
  char hh_mm[6] = "HH:MM";
  // toggle ":" every second (blink)
  lcd.setCursor(11, 0);

  // override w/ "blinking VCR" if not set
  // we'll define "not set" as < midnight on January 1st, 2000
  // (since it'll take a while to get there from 0!)
  if (now() < 946684800) {
    if (second() % 2) {
      sprintf(hh_mm, "%02d:%02d", 12, 0);
    } else {
      sprintf(hh_mm, "  :  ");
    }    
  } else {
    // TODO: time zones, DST, etc.
    if (second() % 2) {
      sprintf(hh_mm, "%02d:%02d", hour(), minute());
    } else {
      sprintf(hh_mm, "%02d %02d", hour(), minute());
    }
  }
  
  lcd.print(hh_mm);

  // date
  char yyyymmdd[11] = "YYYY.MM.DD";
  lcd.setCursor(0, 0);
  if (now() > 946684800) {
    sprintf(yyyymmdd, "%04d.%02d.%02d", year(), month(), day());
  }
  lcd.print(yyyymmdd);
  

  lcd.setCursor(0, 1);
  lcd.print("[ groby ]");
  
  lcd.setCursor(10, 1);
  if (second() % 2) {
    lcd.print("(^-^)b");
  }
  else {
    lcd.print("(-_-)o");
  }

}

void ShowDebug() {

  float ax, ay, az;   //scaled accelerometer values

  // read accelerometer measurements from device, scaled to the configured range
  CurieIMU.readAccelerometerScaled(ax, ay, az);
  lcd.print(ax);

  lcd.setCursor(6, 1);
  lcd.print(ay);

  lcd.setCursor(12, 1);
  lcd.print(az);


  // get temp via IMU due to issues with Grove sensor
  // see also https://github.com/01org/corelibs-arduino101/blob/master/libraries/CurieIMU/src/BMI160.cpp#L2302
  float temperature = CurieIMU.readTemperature();
  float temperature_c = (temperature / 512.0) + 23;
  float temperature_f = temperature_c * 9 / 5 + 32;

  // TODO: restore
  //lcd.setCursor(11, 0);
  //lcd.print(temperature_f);

  // quick RTC demo
  char hh_mm[6] = "HH:MM";
  // toggle ":" every second (blink)
  lcd.setCursor(11, 0);

  // override w/ "blinking VCR" if not set
  // we'll define "not set" as < midnight on January 1st, 2000
  // (since it'll take a while to get there from 0!)
  if (now() < 946684800) {
    if (second() % 2) {
      sprintf(hh_mm, "%02d:%02d", 12, 0);
    } else {
      sprintf(hh_mm, "  :  ");
    }    
  } else {
    // TODO: time zones, DST, etc.
    if (second() % 2) {
      sprintf(hh_mm, "%02d:%02d", hour(), minute());
    } else {
      sprintf(hh_mm, "%02d %02d", hour(), minute());
    }
  }
  
  lcd.print(hh_mm);

  // TODO: remove
  // epoch time
  lcd.setCursor(0, 0);
  lcd.print(now());
}

void UpdateBacklightColor() {

  // update BLE read values
  redCharacteristic.setValue(color_r);
  greenCharacteristic.setValue(color_g);
  blueCharacteristic.setValue(color_b);

  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
}

void SetRandomBacklightColor() {

  color_r = random(256);
  color_g = random(256);
  color_b = random(256);

  UpdateBacklightColor();
}

