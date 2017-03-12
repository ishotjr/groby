/*
 * -----------------------------------------------------------------------------
 *
 * HelloCurie - getting to know the Arduino 101!
 * 
 * -----------------------------------------------------------------------------
 *
 */

// for Grove LCD RGB Backlight
#include <Wire.h>
#include "rgb_lcd.h"


// pin setup
const int button_pin = 3;
const int led_pin = 5;      // must support PWM


rgb_lcd lcd;

static byte color_r = 255;
static byte color_g = 0;
static byte color_b = 0;


void setup() {

  // randomize based on unconnected pin noise
  randomSeed(analogRead(0));

  // set up button
  pinMode(button_pin, INPUT);


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

  lcd.print(millis() / 1000);

  delay(100);
}

void SetRandomBacklightColor() {

  color_r = random(256);
  color_g = random(256);
  color_b = random(256);
  
  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
}

