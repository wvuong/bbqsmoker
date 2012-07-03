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
  p1.A = 5.36924e-4;
  p1.B = 1.91396e-4;
  p1.C = 6.60399e-8;

  p1.resistor = 10000;
  probes[1] = p1;
  
  // loop through probes and print out a debug message
  lcd.clear();
  lcd.autoscroll();
  lcd.setCursor(16,0);
  for (int i = 0; i < numProbes; i++) {
     Probe *p = &probes[i];
     if (&p != 0) {
       Serial.print((*p).name); Serial.print(" initialized at pin ") + Serial.println((*p).pin);
       lcd.write("p"); lcd.print(i); lcd.print("="); lcd.print((*p).pin); lcd.write(" ");
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
    outputValue = readAndOverSample((*ptr).pin, (*ptr).A, (*ptr).B, (*ptr).C, (*ptr).resistor);

    // record it into temperature reading buffer
    (*ptr).currentTemp = outputValue;
    // push it onto the temp buffer
    // everything on the array gets shifted over one
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
  }
  
  // print all probe summary
  lcd.clear();
  lcd.noAutoscroll();
  
  for (int i = 0; i < numProbes; i++) {
    Probe *ptr = &probes[i];
    
    // this only supports max 4 probes
    int x; int y;
    if (i % 2 == 0) {
      x = 0;
    }
    else if (i % 2 == 1) {
      x = 8; 
    }
    if (i / 2 == 0) {
      y = 0;
    }
    else if (i / 2 == 1) {
      y = 1; 
    }
    
    lcd.setCursor(x, y);
    lcd.print("p"); lcd.print(i); lcd.print("="); lcd.print((*ptr).currentTemp); lcd.write((uint8_t)0); lcd.print(" ");
  }
  delay(10000);
  
  // fill up sixty seconds with display
  int per = 50000/numProbes/5000;
  
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
      lcd.print(five); lcd.write((uint8_t)0); lcd.print(" "); lcd.print(fifteen); lcd.write((uint8_t)0); lcd.print(" "); lcd.print(thirty); lcd.write((uint8_t)0);
      lcd.noAutoscroll();
      
      delay(5000);
    }
  }
}

// function was based on http://hruska.us/tempmon/BBQ_Controller.pde for maverick bbq probes, then rewritten
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1246091012
// http://www.kpsec.freeuk.com/vdivider.htm
// http://en.wikipedia.org/wiki/Oversampling
int readAndOverSample(int apin, double A, double B, double C, int resistor) {
  int sensorvalue = 0;
  unsigned int accumulated = 0;
  int numsamples = 64; // to get 3 more bits of precision from 10 to 13, 2^(2*3) = 64 samples
  int n = 0;
  
  sensorvalue = analogRead(apin);
  Serial.print("pin: "); Serial.println(apin);
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
  
  Serial.print("accumulated="); Serial.print(accumulated); Serial.print(", "); Serial.print(n); Serial.print("x"); Serial.println();
 
  // https://github.com/CapnBry/HeaterMeter/blob/c05dc0c39672f12aaf25314c2dfe46a51fb3535d/arduino/heatermeter/grillpid.cpp 
  unsigned int ADCmax = (1 << 13) - 1;
  unsigned int ADCval = accumulated >> 3;
  Serial.print("ADCmax="); Serial.print(ADCmax); Serial.print(", "); Serial.print("ADCval="); Serial.print(ADCval); Serial.println();
  float R, T;
  R = log( resistor / ((ADCmax/(float)ADCval) - 1.0f));
  // R = log(C * ADCmax / (float)5.0 - C);
  Serial.print("R="); Serial.println(R);
  T = 1.0f / ((C * R * R + B) * R + A);
  Serial.print("T="); Serial.println(T);
  float temp = ((T - 273.15f) * (9.0f / 5.0f)) + 32.0f;
  Serial.print("temp="); Serial.println(temp);
  Serial.println();
  return temp;
}
