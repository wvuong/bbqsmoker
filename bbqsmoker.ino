/*
 Based on the "Analog input, analog output, serial output" example sketch.
 
 This program reads from Maverick probes via analog A0 pin and writes the observed temperature
 value to the LCD.
 */

// include the library code:
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int backLight = 13;    // pin 13 will control the backlight
byte degree[8] = { // http://www.quinapalus.com/hd44780udg.html
  B01100,
  B10010,
  B10010,
  B01100,
  B00000,
  B00000,
  B00000,
};

// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = A0;  // Analog input pin that the probe is attached to

int sensorValue = 0;        // value read from the pot
int outputValue = 0;        // value output to the PWM (analog out)
String stringValue = "";

void setup() {
  // set up backlight
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  lcd.begin(16,2);
  lcd.createChar(0, degree); // add in the degree symbol
  
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
}

void loop() {
  String probes[] = {"ET-7x", "ET-732", "Polder"};
  double Avalues[] = {};
  
  
  // read the analog in value:
  sensorValue = readAndOverSample(analogInPin);
  
  // map it to the range of the analog out:
  outputValue = thermister_temp(sensorValue);
  
  // print the results to the serial monitor:
  Serial.print("sensor = " );                       
  Serial.print(sensorValue);      
  Serial.print("\t output = ");      
  Serial.println(outputValue);   

  if (sensorValue == 0 || sensorValue == 1023) {
    stringValue = "No signal";
  }
  else {
    stringValue = String(outputValue);
  }

  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("Current:"); // change this text to whatever you like. keep it clean.
  lcd.setCursor(0,1);           // set cursor to column 0, row 1
  lcd.print(stringValue); lcd.write((uint8_t)0); // write degree symbol http://arduino.cc/forum/index.php?topic=94914.0
  lcd.noAutoscroll();

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2000);                     
}

// http://en.wikipedia.org/wiki/Oversampling
// https://github.com/CapnBry/HeaterMeter/blob/c05dc0c39672f12aaf25314c2dfe46a51fb3535d/arduino/heatermeter/grillpid.cpp
int readAndOverSample(int apin) {
  int sensorvalue = 0;
  unsigned int accumulated = 0;
  int numsamples = 64; // to get 3 more bits of precision from 10 to 13, 2^(2*3) = 64 samples
  int n = 0;
  
  // take 64 samples
  for (int i = 0; i < numsamples; i++) {
    sensorvalue = analogRead(apin);
    // Serial.print(sensorvalue); Serial.print(" ");
    if (sensorValue == 0 || sensorValue >= 1023) {
      return -1;
    }
    accumulated += sensorvalue;
    n++;
  }
  
  unsigned int oversampled = ((float)(accumulated >> 3)/((1 << 13) - 1)) * 1024;
  Serial.print("accumulated="); Serial.print(accumulated); Serial.print(", "); Serial.print(n); Serial.print("x, oversampled="); Serial.println(oversampled);
  return oversampled;
}

// function copied from http://hruska.us/tempmon/BBQ_Controller.pde for maverick bbq probes
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1246091012
// http://www.kpsec.freeuk.com/vdivider.htm

/*
  // pit et732 probe
  double A = 5.36924e-4;
  double B = 1.91396e-4;
  double C = 6.60399e-8;
*/

int thermister_temp(int aval) {
  double A = 2.3067434E-4; //  ET-73, ET-7
  double B = 2.3696596E-4;
  double C = 1.2636414E-7;
  return thermister_temp(aval, A, B, C);
}

/*
925.00 voltage=4.52 R=207,424.20
sensor = 925	 output = 75

1023.00 voltage=5.00 R=22,710,622.00
sensor = 1023	 output = -89
*/
int thermister_temp(int aval, double A, double B, double C) {
  double logR, R, T;

  // This is the value of the other half of the voltage divider
  //	Rknown = 22200;
  
  float voltage = aval/1024*5.0;
  
  // Do the log once so as not to do it 4 times in the equation
  //	R = log(((1024/(double)aval)-1)*(double)22200);
  R = (1 / ((1024 / (double) aval) - 1)) * (double) 22200; // this calculation is for vcc -> 22k resistor -> analog in -> thermister -> ground
  logR = log(R);
  Serial.print(aval); Serial.print(" voltage="); Serial.print(voltage); Serial.print(" R="); Serial.print(R); Serial.print("\n");
  
  // Compute degrees C from Kelvin; T = 1 / (A + B log(R) + C log(R) ^ 3)
  T = (1 / (A + B * logR + C * logR * logR * logR)) - 273.25;
  
  // return degrees F
  return ((int) ((T * 9.0) / 5.0 + 32.0));
}
