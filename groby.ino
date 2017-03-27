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

// for Timer 1 / interrupts
#include "CurieTimerOne.h"

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
// all digital pins are usable for interrupts on 101, but 3 also works for Uno etc. so that's nice
const int button_pin = 3;
const int piezo_pin = 4;
const int led_pin = 5;      // must support PWM

const int lcd_max_length = 17; // maximum LCD message length (including \0 terminator)


const int ble_max_length = 20; // maximum characteristic size (bytes)
BLEPeripheral blePeripheral;
BLEService lcdService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEUnsignedCharCharacteristic redCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEUnsignedCharCharacteristic greenCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEUnsignedCharCharacteristic blueCharacteristic("19B10013-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLECharacteristic messageCharacteristic("19B10014-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);
BLEUnsignedIntCharacteristic timeCharacteristic("19B10015-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEIntCharacteristic tempCharacteristic("19B10016-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLECharacteristic weatherCharacteristic("19B10017-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);
BLEUnsignedIntCharacteristic tweettimeCharacteristic("19B10018-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLECharacteristic tweetuserCharacteristic("19B10019-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);
BLECharacteristic tweetmsgCharacteristic("19B10020-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);
BLECharacteristic notiheaderCharacteristic("19B10021-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);
BLECharacteristic notimessageCharacteristic("19B10022-E8F2-537E-4F6C-D104768A1214", BLEWrite, ble_max_length);



rgb_lcd lcd;

static byte color_r = 0;
static byte color_g = 0;
static byte color_b = 0;

// weather
static float outdoor_temperature_f = 0;
static unsigned char weather_forecast[lcd_max_length] = "no weather (@.@)";

// tweets
static unsigned int tweet_time = 0;
static unsigned char tweet_username[lcd_max_length] = "@nobody         ";
static unsigned char tweet_message[lcd_max_length] = "no tweets  (o_O)";

// notifications
static unsigned char notification_header[lcd_max_length] = "no notifications";
static unsigned char notification_message[lcd_max_length] = "           (T-T)";


//enum ui_states byte {
enum ui_states {
  HOME = 0,
  WEATHER = 1,
  TWEETS = 2,
  NOTIFICATIONS = 3,
  ALERT = 127,
  DEBUG = 255 // overflows back to HOME when incremented
};
// total "normal" states i.e. n/i ALERT or DEBUG
static const byte ui_state_count = 4;

// initial UI state is home
static byte current_state = HOME;
static byte last_state = HOME;

// for software button debounce in ISR
static unsigned long last_interrupt = 0;


void setup() {

  // randomize based on unconnected pin noise
  randomSeed(analogRead(0));


  // DON'T set RTC - either it's already set because power stayed on
  // between sketches, or we'll default to 0 and set via BLE later
  

  // initialize Curie BLE, services and characteristics
  blePeripheral.setLocalName("groby :)");
  // set advertised service UUID
  blePeripheral.setAdvertisedServiceUuid(lcdService.uuid());

  blePeripheral.addAttribute(lcdService);
  blePeripheral.addAttribute(redCharacteristic);
  redCharacteristic.setEventHandler(BLEWritten, red_characteristic_callback);
  blePeripheral.addAttribute(greenCharacteristic);
  greenCharacteristic.setEventHandler(BLEWritten, green_characteristic_callback);
  blePeripheral.addAttribute(blueCharacteristic);
  blueCharacteristic.setEventHandler(BLEWritten, blue_characteristic_callback);
  blePeripheral.addAttribute(messageCharacteristic);
  messageCharacteristic.setEventHandler(BLEWritten, message_characteristic_callback);
  blePeripheral.addAttribute(timeCharacteristic);
  timeCharacteristic.setEventHandler(BLEWritten, time_characteristic_callback);
  blePeripheral.addAttribute(tempCharacteristic);
  tempCharacteristic.setEventHandler(BLEWritten, temp_characteristic_callback);
  blePeripheral.addAttribute(weatherCharacteristic);
  weatherCharacteristic.setEventHandler(BLEWritten, weather_characteristic_callback);
  blePeripheral.addAttribute(tweettimeCharacteristic);
  tweettimeCharacteristic.setEventHandler(BLEWritten, tweettime_characteristic_callback);
  blePeripheral.addAttribute(tweetuserCharacteristic);
  tweetuserCharacteristic.setEventHandler(BLEWritten, tweetuser_characteristic_callback);
  blePeripheral.addAttribute(tweetmsgCharacteristic);
  tweetmsgCharacteristic.setEventHandler(BLEWritten, tweetmsg_characteristic_callback);
  blePeripheral.addAttribute(notiheaderCharacteristic);
  notiheaderCharacteristic.setEventHandler(BLEWritten, notiheader_characteristic_callback);
  blePeripheral.addAttribute(notimessageCharacteristic);
  notimessageCharacteristic.setEventHandler(BLEWritten, notimessage_characteristic_callback);


  // (setting of RGB characteristic values moved to update_backlight_color() )


  unsigned char buffer[ble_max_length] = "";
  messageCharacteristic.setValue(buffer, ble_max_length);

  weatherCharacteristic.setValue(weather_forecast, ble_max_length);
  

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
  analogWrite(led_pin, DEBUG);


  // set up LCD rows/cols
  lcd.begin(16, 2);

  set_random_backlight_color();

  lcd.print((const char *) buffer);

  // kick off ui state cycling
  CurieTimerOne.start(20 * 1000000, &ui_cycle_isr); // microseconds!

  // attach interrupt on button pin
  // TODO: should maybe not share ui_cycle_isr() and reset timer on press IRL, but this is fine for PoC!
  attachInterrupt(button_pin, ui_cycle_isr, CHANGE);
  // interrupt number = pin number on the 101
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

  // wow - this is pretty empty now that everything's done via callbacks! :D
  
  // poll for BLE events
  blePeripheral.poll();


  /*
                    _               _   
         ___  _   _| |_ _ __  _   _| |_ 
        / _ \| | | | __| '_ \| | | | __|
       | (_) | |_| | |_| |_) | |_| | |_ 
        \___/ \__,_|\__| .__/ \__,_|\__|
                       |_|       
  */

  show_state(current_state);

  // update RTC characteristic
  timeCharacteristic.setValue(now());
  // TODO: is this expensive? if so, write-only w/b fine...

  delay(100);
}



// *** BLE callbacks ***

void red_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
    // TODO: error checking?
    color_r = redCharacteristic.value();
    update_backlight_color();
}
void green_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
    // TODO: error checking?
    color_g = greenCharacteristic.value();
    update_backlight_color();
}
void blue_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
    // TODO: error checking?
    color_b = blueCharacteristic.value();
    update_backlight_color();
}

void message_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = messageCharacteristic.valueLength();
  // note that string literal *includes* \0 termination
  unsigned char message[lcd_max_length] = "                "; // cheap padding for now
  char message_formatted[lcd_max_length];

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(message, messageCharacteristic.value(), message_length);

  lcd.setCursor(0, 0);
  lcd.print((const char *) message);
  //lcd.print((const char *) message_formatted);
}

void time_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  setTime(timeCharacteristic.value());
}

void temp_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  outdoor_temperature_f = (float)tempCharacteristic.value();
}

void weather_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = weatherCharacteristic.valueLength();

  // note that string literal *includes* \0 termination
  memcpy(weather_forecast, "                ", lcd_max_length); // cheap padding for now

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(weather_forecast, weatherCharacteristic.value(), message_length);
}

void tweettime_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  tweet_time = (float)tweettimeCharacteristic.value();
}

void tweetuser_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = tweetuserCharacteristic.valueLength();

  // note that string literal *includes* \0 termination
  memcpy(tweet_username, "                ", lcd_max_length); // cheap padding for now

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(tweet_username, tweetuserCharacteristic.value(), message_length);
}

void tweetmsg_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = tweetmsgCharacteristic.valueLength();

  // note that string literal *includes* \0 termination
  memcpy(tweet_message, "                ", lcd_max_length); // cheap padding for now

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(tweet_message, tweetmsgCharacteristic.value(), message_length);
}

void notiheader_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = notiheaderCharacteristic.valueLength();

  // note that string literal *includes* \0 termination
  memcpy(notification_header, "                ", lcd_max_length); // cheap padding for now

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(notification_header, notiheaderCharacteristic.value(), message_length);
}

void notimessage_characteristic_callback(BLECentral &central, BLECharacteristic &characteristic) {
  
  // TODO: error checking?
  size_t message_length = notimessageCharacteristic.valueLength();

  // note that string literal *includes* \0 termination
  memcpy(notification_message, "                ", lcd_max_length); // cheap padding for now

  // it's possible to receive messages longer than the LCD width
  if (message_length > (lcd_max_length - 1)) {
    // truncate for now
    // TODO: handle longer messages via scrolling later...
    // NOTE: characteristic limit is 20 bytes though...
    message_length = lcd_max_length - 1;
  };

  // overwrite 16 or fewer chars (remaining stay ' ' to overwrite old on LCD)
  memcpy(notification_message, notimessageCharacteristic.value(), message_length);
}



// *** UI states and management ***

// callback function for incrementing/setting UI state 
void ui_cycle_isr() {
  
  last_state = current_state;

  // software debounce for when button triggers interrupt
  if ((millis() - last_interrupt) > 100) {
        
    current_state++;
    if (current_state > (ui_state_count - 1)) {
      current_state = 0;
    }
  
  } else if ((millis() - last_interrupt) > 50) {
    // debug mode on second very fast press (double-tap)
    current_state = 255;
    lcd.clear();
  }
  // otherwise ignore as bounce

  // clear LCD when transitioning between states
  if (current_state != last_state) {
    lcd.clear();
  }
  // not sure how long lcd.clear() takes (it's slow) but not worth counting that against interrupt time
  last_interrupt = millis();

}

// paramaterized access to UI states via ui_states value
// (for easier sequential cycling etc.)
void show_state(byte ui_state) {

  switch (ui_state) {
    case HOME:
      show_home();
      break;
    case WEATHER:
      show_weather();
      break;
    case TWEETS:
      show_tweets();
      break;
    case NOTIFICATIONS:
      show_notifications();
      break;

    default:
      show_debug();
  }
  
}


void show_home() {

  // green background
  
  color_r = 31;
  color_g = 255;
  color_b = 31;

  update_backlight_color();

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


void show_weather() {

  // orange background
  
  color_r = 255;
  color_g = 69;
  color_b = 0;

  update_backlight_color();

  // INDOOR

  // get temp via IMU due to issues with Grove sensor
  // see also https://github.com/01org/corelibs-arduino101/blob/master/libraries/CurieIMU/src/BMI160.cpp#L2302
  float indoor_temperature = CurieIMU.readTemperature();
  float indoor_temperature_c = (indoor_temperature / 512.0) + 23;
  float indoor_temperature_f = indoor_temperature_c * 9 / 5 + 32;

  char indoor_f[9] = "in:?? F ";
  lcd.setCursor(0, 0);
  sprintf(indoor_f, "in:%02.0f%cF ", indoor_temperature_f, (char)0b11011111); // °
  // see 12. Font Table in https://seeeddoc.github.io/Grove-LCD_RGB_Backlight/res/JHD1214Y_YG_1.0.pdf for supported chars
  lcd.print(indoor_f);

  // OUTDOOR
  char outdoor_f[9] = "out:?? F";
  lcd.setCursor(8, 0);
  // only display if set
  if (outdoor_temperature_f != 0) {
    sprintf(outdoor_f, "out:%02.0f%cF", outdoor_temperature_f, (char)0b11011111); // °    
  }
  lcd.print(outdoor_f);
  
  lcd.setCursor(0, 1);
  lcd.print((const char *) weather_forecast);
}


void show_tweets() {

  // Twitter blue background (#00aced)
  
  color_r = 0x00;
  color_g = 0xac;
  color_b = 0xed;

  update_backlight_color();

  lcd.setCursor(0, 0);
  lcd.print((const char *) tweet_username);
  
  // will truncate long username, and not fit if >= 16.67 hours old but that's fine for PoC
  // only display if set
  // TODO: should check RTC is set too really? (though technically < 1000 check will catch due to overflow?)
  if (tweet_time > 0) {
    lcd.setCursor(12, 0);
    char tweet_time_elapsed[5] = "999m";
    int tweet_delta = now() - tweet_time;
    // epoch time comparison = seconds elapsed
    tweet_delta = tweet_delta / 60;
    if (tweet_delta < 1000) {
      sprintf(tweet_time_elapsed, "%dm", tweet_delta);
    }
    lcd.print(tweet_time_elapsed);
  }

  lcd.setCursor(0, 1);
  lcd.print((const char *) tweet_message);
}


void show_notifications() {

  // red background
  
  color_r = 0xff;
  color_g = 0x00;
  color_b = 0x00;

  update_backlight_color();

  // TODO: custom chars for email etc. w/b cool!
  lcd.setCursor(0, 0);
  lcd.print((const char *) notification_header);
  lcd.setCursor(0, 1);
  lcd.print((const char *) notification_message);
}


void show_debug() {
  
  // re-randomize backlight on first run
  if (current_state != last_state) {
    set_random_backlight_color();
    last_state = current_state; // special usage for this state
  }

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

void update_backlight_color() {

  // update BLE read values
  redCharacteristic.setValue(color_r);
  greenCharacteristic.setValue(color_g);
  blueCharacteristic.setValue(color_b);

  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
}

void set_random_backlight_color() {

  color_r = random(256);
  color_g = random(256);
  color_b = random(256);

  update_backlight_color();
}

