//====================================
//Frequency Counter using Timers 1 & 2
// Modified from code found here: https://akuzechie.blogspot.com/2021/02/frequency-counter-using-arduino-timers.html
// Information on Arduino UNO counter use here: https://docs.arduino.cc/tutorials/generic/secrets-of-arduino-pwm
//====================================
// Input on pin D5
// Output PWM to fan on D6 through a transistor which inverts the PWM.
//====================================

#define PROG_NAME "**** FrequencyCounterusingTimers1and2 ****"
#define VERSION "Rev: 0.2"
#define BAUDRATE 115200


#define PLOTTING true

//For serial input
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

int fanPWMvalue = 0;
void updateFanPWM(String inputString){
    int fanPWMset = 0;
    fanPWMvalue = inputString.toInt();
    fanPWMvalue = max(fanPWMvalue, 0);
    fanPWMvalue = min(fanPWMvalue, 255);
    fanPWMset = 255 - fanPWMvalue;    //Inverted PWM sense because of transistor on GPIO output.
    analogWrite(6, fanPWMset );  //To Fan PWM.
  
}//end update fan pwm

    
//For frequency counter
volatile unsigned long totalCounts;
volatile bool finishedCount;
volatile unsigned long Timer1overflowCounts;
volatile unsigned long overflowCounts;
unsigned int counter, countPeriod;
//=================================================================
void setup()
{
  Serial.begin(BAUDRATE);
  delay(100);
  Serial.println("Fan_test RPM "); 
  analogWrite(6, (255 - fanPWMvalue));  //To Fan PWM.
  delay(1000); //So that fan can get to set speed.
  //  Make a single read to get the count setup.
  startCount(1000);
  while (!finishedCount) {}
  startCount(1000);
}
//=================================================================
void loop()
{

  while (finishedCount) {
    startCount(1000);
    Serial.print(fanPWMvalue*10);
    Serial.print(" ");  //Print base line at zero
    Serial.print(totalCounts * 30);
    Serial.println();
  }

  // Get user input, a string when a newline arrives:
  if (stringComplete) {
//    Serial.println(inputString);

    updateFanPWM(inputString);
    inputString = "";
    stringComplete = false;
  }//end processing string.

}//end of loop()
//=================================================================
void startCount(unsigned int period)
{
  finishedCount = false;
  counter = 0;
  Timer1overflowCounts = 0;
  countPeriod = period;

  //Timer 1: overflow interrupt due to rising edge pulses on D5
  //Timer 2: compare match interrupt every 1ms
  noInterrupts();
  TCCR1A = 0; TCCR1B = 0; //Timer 1 reset
  TCCR2A = 0; TCCR2B = 0; //Timer 2 reset
  TIMSK1 |= 0b00000001;   //Timer 1 overflow interrupt enable
  TCCR2A |= 0b00000010;   //Timer 2 set to CTC mode
  OCR2A = 124;            //Timer 2 count upto 125
  TIMSK2 |= 0b00000010;   //Timer 2 compare match interrupt enable
  TCNT1 = 0; TCNT2 = 0;   //Timer 1 & 2 counters set to zero
  TCCR2B |= 0b00000101;   //Timer 2 prescaler set to 128
  TCCR1B |= 0b00000111;   //Timer 1 external clk source on pin D5
  interrupts();
}
//=================================================================
ISR(TIMER1_OVF_vect)
{
  Timer1overflowCounts++;
}
//=================================================================
ISR (TIMER2_COMPA_vect)
{
  overflowCounts = Timer1overflowCounts;
  counter++;
  if (counter < countPeriod) return;

  TCCR1A = 0; TCCR1B = 0;   //Timer 1 reset
  TCCR2A = 0; TCCR2B = 0;   //Timer 2 reset
  TIMSK1 = 0; TIMSK2 = 0;   //Timer 1 & 2 disable interrupts

  totalCounts = (overflowCounts * 65536) + TCNT1;
  finishedCount = true;
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
