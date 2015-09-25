const double PI2 = 6.283185;
const byte AMP = 127;
const byte OFFSET = 128;

//bitshift lookup tables
byte shiftLeftSix[256];
byte shiftRightTwo[256];

const int LENGTH = 32;
byte sinWave[LENGTH];
byte squareWave[LENGTH];
byte sawWave[LENGTH];

//index of wave array we want to write next
byte index = 0;

void setup(){
  Serial.begin(9600);
  //set input output modes of pins that write to the DAC
  DDRB = 0B11111111; //set pins 8 to 13 as outputs
  DDRD |= 0B11111100; //set pins 2 to 7 as ouputs
  buildSinLookup();
  buildBitShiftTables();
  //set wave freq and interrupt handler stuff
  setWaveFreq(5000);
}

void buildSinLookup(){
  //pregenerate wave forms
  for (int i=0; i<LENGTH; i++) { // Step across wave tables
   float v = (AMP*sin((PI2/LENGTH)*i)); // Compute value
   sinWave[i] = reverse(byte(v+OFFSET)); // Store value as integer
  }
}

void buildBitShiftTables(){
    for(int i = 0; i < 256; i++){
      shiftLeftSix[i] = i << 6;
      shiftRightTwo[i] = i >> 2;
    }
}

void setTimerOneInterrupt(long interruptFreq){
  //disable interrupts
  cli();
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  //PRESCALER = x1
  //Set compare register
  //can we do this with an XOR?
  OCR1A = (16000000L) / (interruptFreq) - 1;//(must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 bit for 1 prescaler
  TCCR1B |= (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  //enable interrupts
  sei();
} 

void setWaveFreq(long waveFreq){
  long interruptFreq = waveFreq * (long)LENGTH;
  setTimerOneInterrupt(interruptFreq);
}

void writeByte(byte val){
  //use lookup tables instead of actual shift operations to save cycles
  byte portDbyte = shiftLeftSix[val];
  byte portBbyte = shiftRightTwo[val];
  PORTB = portBbyte;
  PORTD = portDbyte;
}

byte reverse(byte inb) {
  //reverse function taken from http://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
   byte b = inb;
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

//Called on timer one interrupt
ISR(TIMER1_COMPA_vect){//timer1 interrupt writes bytes onto D6 to D13
  //use the bitmask B00011111 to take the index mod32 and quickly index into the array
  writeByte(sinWave[B00011111 & index]);
  index++;
}


void loop(){
}
