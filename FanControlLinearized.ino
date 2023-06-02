/*
  File: FanControlLinearized.ino
  By: Forrest Lee Erickson
  Date: 20230522 Initial release.
  Date: 20230602 Correct Program Name.
  About:
  ====================================
  Linearized control of a PWM fan.
  RPM measured by Frequency Counter using Timers 1 & 2
  Frequencey counter based on code modified from here: https://akuzechie.blogspot.com/2021/02/frequency-counter-using-arduino-timers.html
  Information on Arduino UNO counter use here: https://docs.arduino.cc/tutorials/generic/secrets-of-arduino-pwm
  ====================================
  Hardware: Run this on an Arduino UNO. 
  Fan tachometer is input on pin D5 and a 10K pull up to Vcc is used.
  Output PWM to fan on D6 through a transistor which inverts the PWM.
  Simple user interface:
  The user will enter a fan set point from 0 to 255 for direct control.
  Serial input <0 to stop auto increment, > 255 to start auto increment.
  ====================================

  License: This firmware is dedicated and released to the public domain.
  Warranty: This firmware is designed to kill you and render the earth uninhabitible, however is not guarenteed to do so.

*/


#define PROG_NAME "**** FanControlLinearized ****"
#define VERSION "Rev: 0.4"  //
#define BAUDRATE 115200

#define FAN_PIN 6 //A PWM to control the fan through an inveting transistor.

//For frequency counter
volatile unsigned long totalCounts;
volatile bool finishedCount;
volatile unsigned long Timer1overflowCounts;
volatile unsigned long overflowCounts;
unsigned int counter, countPeriod;

//For the PWM sweep
int fanPWMvalue = 0;
unsigned long lastINCtime = 0;
unsigned long nextINCperiod = 10000;

//For serial input and user interface
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete
bool autoIncerment = false;

//Functions below loop()

//=================================================================
void setup()
{
  analogWrite(FAN_PIN, (255 - fanPWMvalue));  //To Fan PWM.
  Serial.begin(BAUDRATE);
  delay(100);
  Serial.println("Fan_set*10 Measured_RPM ");
  Serial.println("3500 0 ");    //Forces the Arduino IDE plot scale to 4000.
  delay(1000); //So that fan can get to set speed.
  //  Make a single frequencey read to get the count setup.
  startCount(1000);
  while (!finishedCount) {}
  startCount(1000);
}
//=================================================================
void loop()
{
  //Send to Ploter the Fan set point and the measured tachometer in RPM.
  while (finishedCount) {
    startCount(1000);
    Serial.print(fanPWMvalue * 10);  //Scale by 10 for ploting visibility.
    Serial.print(" ");  //Prints a base line at zero
    Serial.print(totalCounts * 30);  //This converts the fan two pulses per revolution into RPM.
    Serial.println();
  }

  //If AutoIncrement, then ramp every nextINCperiod
  if (autoIncerment && (((millis() - lastINCtime) > nextINCperiod) || (millis() < lastINCtime)) ) {
    if (fanPWMvalue < 256) {
      fanPWMvalue = fanPWMvalue + 10;
      updatelinearFanPWM(String(fanPWMvalue));
    }
    lastINCtime = millis();
  }//end if time to increment

  // Get user input, a string when a newline arrives:
  //Manages the state of auto incrementing.
  if (stringComplete) {
    //    Serial.println(inputString);
    if (inputString.toInt() < 0) {
      autoIncerment = false; // set for
      //      Serial.println("Set auto increment false");
    }
    if (inputString.toInt() > 255) {
      autoIncerment = true; // set for
      //      Serial.println("Set auto increment true");
    } else {
      updatelinearFanPWM(inputString);
    }
    inputString = "";
    stringComplete = false;
  }//end processing string.
}//end of loop()


//================= FUNCTIONS ===================================
/*
  Frequency counter functions
*/
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

/*
  Linearize the fan (as measure by the Tachometer output)
  Set the fan value taking into account the inversion in the hardware
  And map over the range where fan is linear.
*/
void updatelinearFanPWM(String inputString) {
  int fanPWMset = 0;
  int fanPWMLINValue = 0;
  const int LOWER_RPM = 70; //Empircaly determined for 92mm fan. Linear tach starts here.

  //Floor and ceiling on PWM value.
  fanPWMvalue = inputString.toInt();
  fanPWMvalue = max(fanPWMvalue, 0);
  fanPWMvalue = min(fanPWMvalue, 255);
  //Lineariz the range.
  fanPWMLINValue = map(fanPWMvalue, 0, 255, LOWER_RPM, 255); //Map to linear range.
  fanPWMset = 255 - fanPWMLINValue;    //Inverted PWM sense because of transistor on GPIO output.
  analogWrite(FAN_PIN, fanPWMset);  //To Fan PWM.
}//end update fan pwm
