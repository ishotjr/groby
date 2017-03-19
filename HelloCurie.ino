/*
 * -----------------------------------------------------------------------------
 *
 * HelloCurie - getting to know the Arduino 101!
 * 
 * -----------------------------------------------------------------------------
 *
 */

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


rgb_lcd lcd;

static byte color_r = 255;
static byte color_g = 0;
static byte color_b = 0;


void setup() {

  // randomize based on unconnected pin noise
  randomSeed(analogRead(0));

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
  lcd.begin(16,2);

  SetRandomBacklightColor();
  
  lcd.print("Hello, Curie!");
}

void loop() {

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

  //lcd.print(millis() / 1000);
  
  float ax, ay, az;   //scaled accelerometer values

  // read accelerometer measurements from device, scaled to the configured range
  CurieIMU.readAccelerometerScaled(ax, ay, az);
  lcd.print(ax);

  lcd.setCursor(6, 1);
  lcd.print(ay);

  lcd.setCursor(12, 1);
  lcd.print(az);

  delay(100);
}

void SetRandomBacklightColor() {

  color_r = random(256);
  color_g = random(256);
  color_b = random(256);
  
  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
}

