#include <math.h>

/*
  

 Author: Allen Downey 
 
 Based on http://arduino.cc/en/Tutorial/AnalogInput
 Created by David Cuartielles
 modified 30 Aug 2011
 By Tom Igoe

 License: Public Domain
 
 */
int ledPin = 5;       // select the pin for the LED
int buttonPin1 = 2;
int buttonPin2 = 3;

uint8_t isinTable8[] = { 
  //  AUTHOR: Rob Tillaart
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87, 
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128, 

  131, 135, 139, 143, 146, 150, 153, 157, 160, 164, 
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221, 

  223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 
}; 

int isin(int x)
{
  //  AUTHOR: Rob Tillaart
  //  takes value between 0 and 360, and returns a value between -128 and 128 
  boolean pos = true;  // positive - keeps an eye on the sign.
  uint8_t idx;
  // remove next 6 lines for fastestl!
   if (x < 0) 
    {
      x = -x;
      pos = !pos;  
    }  
   if (x >= 360) x %= 360;   
  if (x > 180) 
  {
    idx = x - 180;
    pos = !pos;
  }
  else idx = x;
  if (idx > 90) idx = 180 - idx;
  if (pos) return isinTable8[idx]/2 ;
  return -(isinTable8[idx]/2);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(buttonPin1, INPUT_PULLUP);  
  pinMode(buttonPin2, INPUT_PULLUP);  

  pinMode(ledPin, OUTPUT);
  
  pinMode(13, OUTPUT);  
  pinMode(12, OUTPUT);  
  pinMode(11, OUTPUT);  
  pinMode(10, OUTPUT);  
  pinMode(9, OUTPUT);  
  pinMode(8, OUTPUT);  
  pinMode(7, OUTPUT);  
  pinMode(6, OUTPUT);
}

void writeByte(int x) {
  int pin;
  
  for (pin=13; pin>=6; pin--) {
    digitalWrite(pin, x&1);
    x >>= 1;
  }
}

int calcSin(int t, double low, double high, double freq){
  float r = t/1000.0;
  double amplitude = high - low;
  return  0.5 * amplitude + 0.5 * amplitude * sin(r * 2.0 * PI * freq) + low;
}

int calcSin2(unsigned long t, int low, int high, int freq) {
  int amplitude = high - low;
  return isin(freq*t) / 128.0 * amplitude + 0.5*amplitude + low;
}

// TODO
// try to do it without millis() or micros() (just using a counter and scoping)

int low = 36;
int high = 255;
double freq = 3;

void loop() {
  int button1 = digitalRead(buttonPin1);
  if (button1) return;
  //long current = micros();

  writeByte(calcSin(millis(), low, high, freq));

  /*Serial.print(current);
  Serial.print(",");
  Serial.println(calcSin2(current, low, high, freq));*/
}
