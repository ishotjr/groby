/*
 * -----------------------------------------------------------------------------
 *
 * HelloCurie - getting to know the Arduino 101!
 * 
 * -----------------------------------------------------------------------------
 *
 */

#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

const int color_r = 255;
const int color_g = 0;
const int color_b = 0;


void setup() {
  
  // set up LCD rows/cols
  lcd.begin(16,2);

  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
  
  lcd.print("Hello, Curie!");
}

void loop() {

  // set cursor to beginning of second (numbered from 0) row
  lcd.setCursor(0, 1);

  lcd.print(millis() / 1000);

  delay(100);
}
