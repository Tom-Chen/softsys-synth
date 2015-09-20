const double PI2 = 6.283185;
const byte AMP = 127;
const byte OFFSET = 128;

const int LENGTH = 256;
byte sinWave[LENGTH];
byte squareWave[LENGTH];
byte sawWave[LENGTH];

//index of wave array we want to write next
byte index = 0;

//for testing w/ square waves. remove later
boolean high = true;

void setup(){
  Serial.begin(9600);
  //set input output modes of pins that write to the DAC
  DDRB = 0B11111111; //set pins 8 to 13 as outputs
  DDRD |= 0B11111100; //set pins 2 to 7 as ouputs
  //pregenerate wave forms
  for (int i=0; i<LENGTH; i++) { // Step across wave tables
   float v = (AMP*sin((PI2/LENGTH)*i)); // Compute value
   sinWave[i] = int(v+OFFSET); // Store value as integer
  }
}

void writeByte(byte val){
  //pins for PORTD 7 6 5 4 3 2 1 0
  byte portDbyte = (val & B00000011) << 6;
  
  // pins for PORTB
  byte portBbyte = val >> 2;
  Serial.print("Port D ");
  Serial.println(portDbyte, BIN);
  Serial.print("Port B ");
  Serial.println(portBbyte, BIN);
  
  PORTD = portDbyte;
  PORTB = portBbyte;
}

void writeByte2(byte val){
  //allen's writeByte function
  int pin;
  for (pin=13; pin>=6; pin--) {
    digitalWrite(pin, val&1);
    val >>= 1;
  }
}


void loop(){
  // For some reason pin 9 is being written to when b00000010 is written
  writeByte2(index);
  index++;
}
