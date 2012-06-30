/*
 Based on the "Analog input, analog output, serial output" example sketch.
 
 This program reads from Maverick probes via analog A0 pin and writes the observed temperature
 value to the LCD.
 */

// include the library code:
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int backLight = 13;    // pin 13 will control the backlight

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
  
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
}

void loop() {
  // read the analog in value:
  sensorValue = analogRead(analogInPin);            
  // map it to the range of the analog out:
  outputValue = thermister_temp(sensorValue);
  // change the analog out value:      

  // print the results to the serial monitor:
  Serial.print("sensor = " );                       
  Serial.print(sensorValue);      
  Serial.print("\t output = ");      
  Serial.println(outputValue);   

  if (sensorValue == 0 || sensorValue == 1023) {
    stringValue = "No signal";
  }
  else {
    stringValue = outputValue + "°";
  }

  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("Current temp:");    // change this text to whatever you like. keep it clean.
  lcd.setCursor(0,1);           // set cursor to column 0, row 1
  lcd.print(stringValue);
  lcd.noAutoscroll();

  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2000);                     
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
int thermister_temp(float aval, double A, double B, double C) {
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
