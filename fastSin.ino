//
//    FILE: isinSpeedTest.ino
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.00
// PURPOSE: 
//    DATE: 2013-11-03
//     URL:
//
// Released to the public domain
//

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
volatile uint8_t x = 0;


void setup() 
{
  Serial.begin(115200);
  Serial.println("Start ");

  uint32_t start = micros();
  for (int i=0; i<360; i++)
  {
    x = isin(i);
  }
  uint32_t diff = micros() - start;
  Serial.println(diff/360.0);

  for (int i=0; i<360; i++)
  {
    if (i%10 ==0) Serial.println();
    Serial.print(isin(i));
    Serial.print('\t');
  }
  Serial.println("\n...done...");
}

void loop() {}
