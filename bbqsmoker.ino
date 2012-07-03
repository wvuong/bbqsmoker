/*
 Based on the "Analog input, analog output, serial output" example sketch.
 
 This program reads from Maverick probes via analog A0 pin and writes the observed temperature
 value to the LCD.
 */

// include the lcd library code:
#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int backLight = 13;    // pin 13 will control the backlight
// 
byte degree[8] = { // http://www.quinapalus.com/hd44780udg.html
  B01100,
  B10010,
  B10010,
  B01100,
  B00000,
  B00000,
  B00000,
};


String stringValue = "";

// probe constants
const int numProbes = 2;
const int bufferLength = 30;

// probe datastructure
struct Probe {
  String name;
  int pin;
  double A;
  double B;
  double C;
  int buffer[bufferLength]; 
  int currentTemp;
  int resistor;
};

// probes array
Probe probes[3];


void setup() {
  // set up backlight
  pinMode(backLight, OUTPUT);
  digitalWrite(backLight, HIGH); // turn backlight on. Replace 'HIGH' with 'LOW' to turn it off.
  lcd.begin(16,2);
  lcd.createChar(0, degree); // add in the degree symbol
  
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  
  // configure probes
  Probe p0 = Probe();
  p0.name = "ET-7/73";
  p0.pin = A0;
  p0.A = 2.3067434E-4;
  p0.B = 2.3696596E-4;
  p0.C = 1.2636414E-7;
  p0.resistor = 22000;
  probes[0] = p0;
  
  Probe p1 = Probe();
  p1.name = "ET-732";
  p1.pin = A1;
  /*
  p1.A = 5.36924e-4;
  p1.B = 1.91396e-4;
  p1.C = 6.60399e-8;
  */
  p1.A = 0.00043933992;
  p1.B = 0.000208342;
  p1.C = 1.2004001E-8;
  p1.resistor = 10000;
  probes[1] = p1;
  
  // loop through probes and print out a debug message
  for (int i = 0; i < numProbes; i++) {
     Probe *p = &probes[i];
     if (&p != 0) {
       Serial.print((*p).name); Serial.print(" initialized at pin ") + Serial.println((*p).pin);
     }
  }
  
  delay(2000);
}

void loop() {
  int sensorValue = 0;        // value read from the pot
  int outputValue = 0;        // value output to the PWM (analog out)

  // loop through probes 
  for (int i = 0; i < numProbes; i++) {
    Probe *ptr = &probes[i];
    
    // read the analog in value:
    sensorValue = readAndOverSample((*ptr).pin);
  
    // map it to the range of the analog out:
    outputValue = thermister_temp(sensorValue, (*ptr).A, (*ptr).B, (*ptr).C, (*ptr).resistor);
    
    // record it
    (*ptr).currentTemp = outputValue;
    // push it onto the temp buffer
    // everything on the array gets shifted over one
    // Serial.print("last temp: "); Serial.println((*ptr).buffer[0]);
    // Serial.print("direct array access: "); Serial.println((*ptr).buffer[0]);
    for (int i = bufferLength -1; i >= 1; i--) {
      (*ptr).buffer[i] = (*ptr).buffer[i-1];
    } 
    (*ptr).buffer[0] = outputValue;
    /*
    for (int i = 0; i < bufferLength; i++) {
      Serial.print((*ptr).buffer[i]); Serial.print(" ");
    }
    Serial.println();
    */
    // print the results to the serial monitor:
    Serial.print("sensor = " );                       
    Serial.print(sensorValue);      
    Serial.print("\t output = ");      
    Serial.println(outputValue);
    
  }
  
  // print all probe summary
  
  
  // fill up sixty seconds with display
  int per = 20000/numProbes/5000;
  for (int p = 0; p < per; p++) {
  for (int i = 0; i < numProbes; i++) {
    Probe *ptr = &probes[i];
    
    // calculate last five minute average
    int five, fifteen, thirty;
    int accum = 0;
    for (int i = 0; i < 5; i++) {
      if ((*ptr).buffer[i] == 0) {
        accum = 0;
        break;
      }
      accum += (*ptr).buffer[i];
    }
    five = accum/5;
    
    // calculate last 15 minute average
    accum = 0;
    for (int i = 0; i < 15; i++) {
      if ((*ptr).buffer[i] == 0) {
        accum = 0;
        break;
      }
      accum += (*ptr).buffer[i];
    }
    fifteen = accum/15;
    
    // calculate last 30 minute average
    accum = 0;
    for (int i = 0; i < 30; i++) {
      if ((*ptr).buffer[i] == 0) {
        accum = 0;
        break;
      }
      accum += (*ptr).buffer[i];
    }
    thirty = accum/30;
    
    lcd.clear();                  // start with a blank screen
    lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
    
    // first row, write probe id, name, and current temp
    // write degree symbol http://arduino.cc/forum/index.php?topic=94914.0
    lcd.print("p"); lcd.print(i); lcd.print(" "); lcd.print((*ptr).name); lcd.print(" "); lcd.print((*ptr).currentTemp); lcd.write((uint8_t)0);
    
    // second row, write last 5 minutes
    lcd.setCursor(0,1);           // set cursor to column 0, row 1
    lcd.print(five); lcd.print(" "); lcd.print(fifteen); lcd.print(" "); lcd.print(thirty);
    lcd.noAutoscroll();
    
    delay(5000);
  }
  }
}

// http://en.wikipedia.org/wiki/Oversampling
// https://github.com/CapnBry/HeaterMeter/blob/c05dc0c39672f12aaf25314c2dfe46a51fb3535d/arduino/heatermeter/grillpid.cpp
int readAndOverSample(int apin) {
  int sensorvalue = 0;
  unsigned int accumulated = 0;
  int numsamples = 64; // to get 3 more bits of precision from 10 to 13, 2^(2*3) = 64 samples
  int n = 0;
  
  sensorvalue = analogRead(apin);
  Serial.print("initial read: "); Serial.println(sensorvalue);
  delay(2);
  
  // take 64 samples
  for (int i = 0; i < numsamples; i++) {
    sensorvalue = analogRead(apin);
    // Serial.print(sensorvalue); Serial.print(" ");
    if (sensorvalue == 0 || sensorvalue >= 1023) {
      return -1;
    }
    accumulated += sensorvalue;
    n++;
    
    // wait 2 milliseconds before the next loop
    // for the analog-to-digital converter to settle
    // after the last reading:
    delay(2);
  }
  
  unsigned int oversampled = ((float)(accumulated >> 3)/((1 << 13) - 1)) * 1024;
  Serial.print("accumulated="); Serial.print(accumulated); Serial.print(", "); Serial.print(n); Serial.print("x, oversampled="); Serial.println(oversampled);
  return oversampled;
}

// function copied from http://hruska.us/tempmon/BBQ_Controller.pde for maverick bbq probes
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1246091012
// http://www.kpsec.freeuk.com/vdivider.htm


/*
925.00 voltage=4.52 R=207,424.20
sensor = 925	 output = 75

1023.00 voltage=5.00 R=22,710,622.00
sensor = 1023	 output = -89
*/
int thermister_temp(int aval, double A, double B, double C, int resistor) {
  double logR, R, T;

  // This is the value of the other half of the voltage divider
  //	Rknown = 22200;
  
  float voltage = aval/1024.0*5.0;
  
  // Do the log once so as not to do it 4 times in the equation
  //	R = log(((1024/(double)aval)-1)*(double)22200);
  R = (1 / ((1024 / (double) aval) - 1)) * (double) resistor; // this calculation is for vcc -> 22k resistor -> analog in -> thermister -> ground
  logR = log(R);
  Serial.print(aval); Serial.print(" voltage="); Serial.print(voltage); Serial.print(" R="); Serial.print(R); Serial.print("\n");
  
  // Compute degrees C from Kelvin; T = 1 / (A + B log(R) + C log(R) ^ 3)
  T = (1 / (A + B * logR + C * logR * logR * logR)) - 273.25;
  
  // return degrees F
  return ((int) ((T * 9.0) / 5.0 + 32.0));
}
