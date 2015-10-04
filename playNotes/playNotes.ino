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
const char PAUSE = 3;

const byte ledPin = 5;
const byte buttonPin1 = 2;
const byte buttonPin2 = 3;
const byte inputButton1 = 0;

byte button1Press; // 0 for LOW and 0B00100000 for HIGH

byte sinWave[LENGTH];
byte squareWave[LENGTH];
byte sawWave[LENGTH];
byte pauseWave[LENGTH] = {0};
byte *wave[4];
byte waveType;

//index of wave array we want to write next
byte waveIndex = 0;

// temporary waveIndex and waveType for pausing
byte tempWaveIndex = 0;
byte tempWaveType = PAUSE;

//define notes
const short C = 2093;
const short D = 2349;
const short E = 2637;
const short F = 2793;
const short G = 3135;
const short A = 3520;
const short B = 3951;
const short HIGHC = 4186;

//first note in notes and duration array is a sentinel value that is not actually played
//song will loop once it reaches the end of the array
//short notes[] = {0,C,D,E,F,G,A,B,HIGHC};
short notes[] = {0, 200, 400, 800, 1600, 800, 400, 200, 100};
int duration[] = {0,1000,1000,1000,1000,1000,1000,1000,1000}; // in .01s increments
int songLen = sizeof(notes)/sizeof(short);
int songIndex = 0;
int noteDuration = 0;

const long DEBOUNCE_TIME = 3000;
long button0PressedTime = 0;
long button1PressedTime = 0;

void setup(){
  Serial.begin(9600);

  button1Press = 1;
  
  //set input output modes of pins that write to the DAC
  DDRB = 0B11111111; //set pins 8 to 13 as outputs
  DDRD |= 0B11110000; //set pins 4 to 7 as ouputs

  // LED Pin is already set to output as pin 5

  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  
  buildSinLookup();
  buildSquareLookup();
  buildSawLookup();
  wave[SIN] = sinWave;
  wave[SQUARE] = squareWave;
  wave[SAW] = sawWave;
  wave[PAUSE] = pauseWave;
  waveType = SIN;
  buildBitShiftTables();
  //set wave freq and interrupt handler stuff
  //turn off interrupts
  cli();
  initializeTimerTwoInterrupt();
  initializeTimerOneInterrupt();
  initializeTimerZeroInterrupt();
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

void initializeTimerZeroInterrupt() {
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;
  // set compare register for 1 kHz increments
  OCR0A = (16000000L) / (1000*64) - 1; // (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS01) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
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

/*
void writeByte(byte val){
  //use lookup tables instead of actual shift operations to save cycles
  byte portDbyte = shiftLeftSix[val];
  byte portBbyte = shiftRightTwo[val];
  PORTD = portDbyte;
  PORTB = portBbyte;
}
*/

void writeByte(byte val){
  /* TODO
   * Bitwise AND with some variable for setting bit 5 (the LED pin)
   * that variable can be accessed in a global scope and used by the other function
   */
  byte portDbyte = shiftLeftSix[val];
  byte portBbyte = shiftRightTwo[val];
  PORTD = portDbyte;
  //PORTD = portDbyte | button1Press;
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

void changeWaveType(){
  switch(waveType){
    case SIN:
      waveType = SAW;
      break;
    case SAW:
      waveType = SQUARE;
      break;
    case SQUARE:
      waveType = SIN;
      break;
  }
}

void togglePause(){
  if (waveType == PAUSE) {
    waveType = tempWaveType;
    waveIndex = tempWaveIndex;
  }
  else {
    tempWaveType = waveType;
    tempWaveIndex = waveIndex;
    waveType = PAUSE;
  }
}

void checkPauseButton(){
  if (digitalRead(A0)) {
    button0PressedTime++;
    if (button0PressedTime >= DEBOUNCE_TIME){
      togglePause();
      button0PressedTime = 0;
    }    
  } else {
    button0PressedTime = 0;
  }
}

void checkWaveChangeButton(){
    if (digitalRead(A1)) {
    button1PressedTime++;
    if (button1PressedTime >= DEBOUNCE_TIME){
      changeWaveType();
      button1PressedTime = 0;
    }    
  } else {
    button1PressedTime = 0;
  }
}

ISR(TIMER0_COMPA_vect) {
  checkPauseButton();
  checkWaveChangeButton();
}

//Timer one writes out waves to DAC
ISR(TIMER1_COMPA_vect){//timer1 interrupt writes bytes onto D6 to D13
  //use the bitmask B00011111 to take the index mod32 and quickly index into the array
  writeByte(wave[waveType][B00011111 & waveIndex]);
  waveIndex++;
}

//100 Hz timer two interrupt changes frequencies
ISR(TIMER2_COMPA_vect){
  if(songIndex >= songLen){
   songIndex = 0; 
  }
  noteDuration++;
  if(noteDuration > duration[songIndex]){
    //go to the next note
    noteDuration = 0;
    songIndex++;
    setTimerOneInterrupt(waveFreqToCompareReg(notes[songIndex]));
  }
}

void loop(){
}
