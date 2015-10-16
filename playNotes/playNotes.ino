const double PI2 = 6.283185;
const byte AMP = 127;
const byte OFFSET = 128;

//bitshift lookup tables
byte shiftLeftSix[256];
byte shiftRightTwo[256];

const int LENGTH = 32;
const byte SIN = 0;
const byte SQUARE = 1;
const byte SAW = 2;
const byte PAUSE = 3;

const byte ledPin = 5;
const byte inputButton1 = 0;

byte sinWave[LENGTH];
byte squareWave[LENGTH];
byte sawWave[LENGTH];
byte pauseWave[LENGTH];
byte *wave[4];
byte waveType;

//index of wave array we want to write next
byte waveIndex = 0;

// temporary waveIndex and waveType for pausing
byte tempWaveIndex = 0;
byte tempWaveType = PAUSE;

//define notes
const short C_6 = 1046;
const short D_6 = 1174;
const short D_6s = 1244;
const short E_6 = 1318;
const short F_6 = 1396;
const short G_6 = 1567;
const short G_6s = 1661;
const short A_6 = 1760;
const short B_6 = 1975;

const short C_7 = 2093;
const short D_7 = 2349;
const short D_7s = 2489;
const short E_7 = 2637;
const short F_7 = 2793;
const short G_7 = 3135;
const short A_7 = 3520;
const short B_7 = 3951;

const short HIGHC = 4186;

// Fur Elise notes
short notes[] = {E_7, D_7s, E_7, D_7s, E_7, B_6, D_7, C_7, A_6, 0, C_6, E_6, A_6, B_6, 0, E_6, G_6s, B_6, C_7, 0, E_6, E_7, D_7s, E_7, D_7s, E_7, B_6, D_7, C_7, A_6, 0, C_6, E_6, A_6, B_6, 0, E_6, C_7, B_6, A_6, 0, A_6, B_6, C_7, D_7, E_7, 0, G_6, F_7, E_7, D_7, 0, F_6, E_7, D_7, C_7, 0, E_6, D_7, C_7, B_6, 0, E_6};

// Fur Elise rhythm (NOTE: multiplied later in set up for right timing)
int duration[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1};

int songLen = sizeof(notes) / sizeof(short);
int songIndex = 0;
int noteDuration = 0;

const long DEBOUNCE_TIME = 500;
long pauseButtonLastPressed = 0;
long waveButtonLastPressed = 0;

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < songLen; i++) {
    duration[i] = duration[i]*100;
  }

  //set input output modes of pins that write to the DAC
  DDRB = 0B11111111; //set pins 8 to 13 as outputs
  DDRD |= 0B11110000; //set pins 4 to 7 as ouputs

  // LED Pin is already set to output as pin 5
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

void buildSinLookup() {
  for (int i = 0; i < LENGTH; i++) { // Step across wave tables
    float v = (AMP * sin((PI2 / LENGTH) * i)); // Compute value
    sinWave[i] = reverse(byte(v + OFFSET)); // Store value as integer
  }
}

void buildSquareLookup() {
  for (int i = 0; i < LENGTH / 2; i++) {
    squareWave[i] = reverse(byte(255)); // First half of square wave is high
  }
  for (int i = LENGTH / 2; i < LENGTH; i++) {
    squareWave[i] = reverse(byte(0)); // Second half is low
  }
}

void buildSawLookup() {
  for (int i = 0; i < LENGTH; i++) {
    float v = 255 * i * 1.0 / LENGTH;
    sawWave[i] = reverse(byte(v));
  }
}

void buildBitShiftTables() {
  for (int i = 0; i < 256; i++) {
    shiftLeftSix[i] = i << 6;
    shiftRightTwo[i] = i >> 2;
  }
}

//timer two takes care of changing frequencies, checking at 100 Hz
void initializeTimerTwoInterrupt() {
  TCCR2A = 0;// set entire TCCR1A register to 0
  TCCR2B = 0;// same for TCCR1B
  TCNT2  = 0;//initialize counter value to 0
  //PRESCALER is x1024
  //Set compare register
  OCR2A = (16000000L) / (100 * 1024) - 1; //(must be <256)
  // turn on CTC mode
  TCCR2B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR2B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
}


//timer one outputs waveforms
void initializeTimerOneInterrupt() {
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
  //PRESCALER is x1024
  //Set compare register
  OCR0A = (16000000L) / (100 * 1024) - 1; //(must be <256)
  // turn on CTC mode
  TCCR0B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR0B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);
}

void setTimerOneInterrupt(short compareReg) {
  //Set compare register
  OCR1A = compareReg;//(must be <65536)
}

short waveFreqToCompareReg(long waveFreq) {
  long interruptFreq = waveFreq * (long)LENGTH;
  short compareReg = (16000000L) / (interruptFreq) - 1;
  return compareReg;
}

void writeByte(byte val) {
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

void changeWaveType() {
  switch (waveType) {
    case PAUSE:
      break;
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

void togglePause() {
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

void checkPauseButton() {
   if(digitalRead(A0)){
      long timePressed = millis();
      if(timePressed - pauseButtonLastPressed > DEBOUNCE_TIME){
        togglePause();
        pauseButtonLastPressed = timePressed;
      }
   } 
}

void checkWaveChangeButton() {
   if(digitalRead(A1)){
      long timePressed = millis();
      if(timePressed - waveButtonLastPressed > DEBOUNCE_TIME){
        changeWaveType();
        waveButtonLastPressed = timePressed;
      }
   } 
}

//Timer one checks button presses
ISR(TIMER0_COMPA_vect) {
  checkPauseButton();
  checkWaveChangeButton();
}

//Timer one writes out waves to DAC
ISR(TIMER1_COMPA_vect) { //timer1 interrupt writes bytes onto D6 to D13
  //use the bitmask B00011111 to take the index mod32 and quickly index into the array
  writeByte(wave[waveType][B00011111 & waveIndex]);
  waveIndex++;
}

//100 Hz timer two interrupt changes frequencies
ISR(TIMER2_COMPA_vect) {
  if (songIndex >= songLen) {
    songIndex = 0;
  }
  noteDuration++;
  if (noteDuration > duration[songIndex]) {
    //go to the next note
    noteDuration = 0;
    songIndex++;
    setTimerOneInterrupt(waveFreqToCompareReg(notes[songIndex]));
  }
}

void loop() {
}
