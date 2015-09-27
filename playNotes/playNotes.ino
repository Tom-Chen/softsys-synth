const double PI2 = 6.283185;
const byte AMP = 127;
const byte OFFSET = 128;

//bitshift lookup tables
byte shiftLeftSix[256];
byte shiftRightTwo[256];

const int LENGTH = 32;
const char SIN = 0;
const char SQUARE = 1;
const char SAW = 2;
byte sinWave[LENGTH];
byte squareWave[LENGTH];
byte sawWave[LENGTH];
byte *wave[3];
byte waveType;

//index of wave array we want to write next
byte waveIndex = 0;

//first note in notes and duration array is a sentinel value that is not actually played
//last note will be played forever
short notes[] = {0,200};
int duration[] = {0,100}; // in .01s increments
int songLen = sizeof(notes)/sizeof(short);
int songIndex = 0;
int noteDuration = 0;

void setup(){
  Serial.begin(9600);
  //set input output modes of pins that write to the DAC
  DDRB = 0B11111111; //set pins 8 to 13 as outputs
  DDRD |= 0B11111100; //set pins 2 to 7 as ouputs
  buildSinLookup();
  buildSquareLookup();
  buildSawLookup();
  wave[SIN] = sinWave;
  wave[SQUARE] = squareWave;
  wave[SAW] = sawWave;
  waveType = SIN;
  buildBitShiftTables();
  //set wave freq and interrupt handler stuff
  //turn off interrupts
  cli();
  initializeTimerTwoInterrupt();
  initializeTimerOneInterrupt();
  setTimerOneInterrupt(waveFreqToCompareReg(3000));
  //enable interrupts
  sei();
}

void buildSinLookup(){
  for (int i=0; i<LENGTH; i++) { // Step across wave tables
   float v = (AMP*sin((PI2/LENGTH)*i)); // Compute value
   sinWave[i] = reverse(byte(v+OFFSET)); // Store value as integer
  }
}

void buildSquareLookup(){
  for (int i=0; i<LENGTH/2; i++) {
   squareWave[i] = reverse(byte(255)); // First half of square wave is high
  }
  for (int i=LENGTH/2; i<LENGTH; i++) {
   squareWave[i] = reverse(byte(0)); // Second half is low
  }
}

void buildSawLookup(){
  for (int i=0; i<LENGTH; i++) {
   float v = 255 * i * 1.0 / LENGTH;
   sawWave[i] = reverse(byte(v));
  }
}

void buildBitShiftTables(){
    for(int i = 0; i < 256; i++){
      shiftLeftSix[i] = i << 6;
      shiftRightTwo[i] = i >> 2;
    }
}

//timer two takes care of changing frequencies, checking at 100 Hz
void initializeTimerTwoInterrupt(){
  TCCR2A = 0;// set entire TCCR1A register to 0
  TCCR2B = 0;// same for TCCR1B
  TCNT2  = 0;//initialize counter value to 0
  //PRESCALER is x1024
  //Set compare register
  OCR2A = (16000000L) / (100*1024) - 1;//(must be <256)
  // turn on CTC mode
  TCCR2B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR2B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE1A);
} 


//timer one outputs waveforms
void initializeTimerOneInterrupt(){
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  //PRESCALER = x1
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 bit for 1 prescaler
  TCCR1B |= (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
} 

void setTimerOneInterrupt(short compareReg){
  //Set compare register
  OCR1A = compareReg;//(must be <65536)
}

short waveFreqToCompareReg(long waveFreq){
  long interruptFreq = waveFreq * (long)LENGTH;
  short compareReg = (16000000L) / (interruptFreq) - 1;
  return compareReg;
}

void writeByte(byte val){
  //use lookup tables instead of actual shift operations to save cycles
  byte portDbyte = shiftLeftSix[val];
  byte portBbyte = shiftRightTwo[val];
  PORTD = portDbyte;
  PORTB = portBbyte;
}

byte reverse(byte inb) {
  //reverse function taken from http://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-byte
   byte b = inb;
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

//Timer one writes out waves to DAC
ISR(TIMER1_COMPA_vect){//timer1 interrupt writes bytes onto D6 to D13
  //use the bitmask B00011111 to take the index mod32 and quickly index into the array
  writeByte(wave[waveType][B00011111 & waveIndex]);
  waveIndex++;
}

//100 Hz timer two interrupt changes frequencies
ISR(TIMER2_COMPA_vect){
  noteDuration++;
  if(noteDuration > duration[songIndex] && songIndex < songLen - 1){
    //go to the next note
    noteDuration = 0;
    songIndex++;
    setTimerOneInterrupt(waveFreqToCompareReg(notes[songIndex]));
  }
}

void loop(){
}
