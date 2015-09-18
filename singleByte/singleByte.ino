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
  for(int i=0; i<LENGTH/2; i++){
   squareWave[i] = 0; 
  }
  for(int i=LENGTH/2; i < LENGTH; i++){
    squareWave[i] = 0xFF;
  }
  //set wave freq and interrupt handler stuff
  setWaveFreq(1);
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
  //try unsigned ints?
  long interruptFreq = waveFreq * (long)LENGTH;
  setTimerOneInterrupt(interruptFreq);
}

void writeByte2(byte val){
  //allen's writeByte function
  int pin;
  for (pin=13; pin>=6; pin--) {
    digitalWrite(pin, val&1);
    val >>= 1;
  }
}

void writeByte(byte val){
  //pins for PORTD 7 6 5 4 3 2 1 0
  byte portDbyte = (val & 000000011) << 6;
  
  // pins for PORTb
  byte portBbyte = val >> 2;
  
  PORTD = portDbyte;
  PORTB = portBbyte;
}

//Called on timer one interrupt
ISR(TIMER1_COMPA_vect){//timer1 interrupt writes bytes onto D6 to D13
  writeByte(sinWave[index]);
  index++;
}


void loop(){
  //findings: timing works correctly up to 70hz using allen's very slow writeByte function
  //once we try to get faster then that, the number of instructions the writeByte function takes becomes a limiting factor and we can't go faster
  //but writing the values directly should let us go faster
  //even writing directly with allen's values don't seem to let us get faster than ~500 Hz. Our square wave array is length 256, so maybe it's that.
  //How many values do we need to approximate a sin wave well?
  //but writing high and low every cycle
  //TODO: Fix WriteByte (possibly dealing with wire backwards order in doing so)
  //TODO: Shorten wave arrays
}
