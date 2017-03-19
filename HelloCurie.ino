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

  // TODO: fix calculation; 
/*
    sensor_value_tmp = (float)(average_value / 4095 * 2.95 * 2 / 3.3 * 1023)
    resistance = (float)(1023 - sensor_value_tmp) * 10000 / sensor_value_tmp
    temperature = round((float)(1 / (math.log(resistance / 10000) / bValue + 1 / 298.15) - 273.15), 2)
 */
 /*
 // TODO: move to own fn etc.
  float thermistor_value = analogRead(thermistor_pin);
  int b_value = 4250; //4275; //4200; //4250;
  // TODO: 3.3 or 5.0?
  float x = (float)(thermistor_value / 4095 * 2.95 * 2 / 3.3 * 1023);
  //float x = (float)(thermistor_value / 4095 * 2.95 * 2 / 5.0 * 1023);
  
  float resistance = (float)(1023 - x) * 10000 / x;
  float temperature_c = (float)(1 / (log(resistance / 10000) / b_value + 1 / 298.15) - 273.15);
  //float resistance = (float)(1023 - x) * 100000 / x;
  //float temperature_c = (float)(1 / (log(resistance / 100000) / b_value + 1 / 298.15) - 273.15);

  //lcd.print(temperature_c);
  lcd.print(thermistor_value);

  lcd.setCursor(8, 1);
  float temperature_f = temperature_c * 9/5 + 32;
  lcd.print(temperature_f);
*/
/*
  float thermistor_value = analogRead(thermistor_pin);
  float average;
  average = 1023 / thermistor_value - 1;
  average = 100000 / average;
  //average = 100000 * average;
  lcd.print(average);

  float steinhart;
  steinhart = average / 100000;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= 4275;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25 + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C

  lcd.setCursor(11, 1);
  //lcd.print(steinhart);
  float temperature_f = steinhart * 9/5 + 32;
  lcd.print(temperature_f);
*/
/*
  //float thermistor_value = analogRead(thermistor_pin);
  int thermistor_value = analogRead(thermistor_pin);
  float R = 1023.0/((float)thermistor_value)-1.0;  
  //R = 100000.0*R;
  R = 100000.0/R;
 
  float temperature=1.0/(log(R/100000.0)/4275+1/298.15)-273.15;//convert to temperature via datasheet ;

  float steinhart;
  steinhart = R / 100000;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= 4275;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25 + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  
  lcd.print(temperature);
  lcd.setCursor(8, 1);
  //float temperature_f = steinhart * 9/5 + 32;
  //lcd.print(temperature_f);
  lcd.print(steinhart);

  delay(100);
*/
//resistance = (float)(1023 - a) * 10000 / a
//temperature = (float) (1 / (math.log(resistance / 10000) / bValue + 1 / 298.15) - 273.15)
  int thermistor_value = analogRead(thermistor_pin);
  float R = (float)(1023 - thermistor_value) * 100000 / thermistor_value;
  float temperature = (float) (1 / (log(R / 100000) / 4275 + 1 / 298.15) - 273.15);
  lcd.print(temperature);
  delay(100);
}

void SetRandomBacklightColor() {

  color_r = random(256);
  color_g = random(256);
  color_b = random(256);
  
  // set LCD backlight color
  lcd.setRGB(color_r, color_g, color_b);
}

