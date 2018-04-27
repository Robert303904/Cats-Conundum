#include <SparkFun_TB6612.h>

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <RFM69_OTA.h>
#include <RFM69registers.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////    FEATHER/RECEIVER CODE   //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
// This code was taken from the GATEWAY.INO file and modified heavily to make our feather a receiver
// After the feather receives a command from the nano via the RFM69 chip, it translates the signal to the motors
// Made by Thane Reishus-O'Brien

// ample RFM69 receiver/gateway sketch, with ACK and optional encryption, and Automatic Transmission Control
// Passes through any wireless received messages to the serial port & responds to ACKs
// It also looks for an onboard FLASH chip, if present
// RFM69 library and sample code by Felix Rusu - http://LowPowerLab.com/contact
// Copyright Felix Rusu (2015)

//////////////////////#include libraries section///////////////////////////////////////////////
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)save them to a string

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NANOID        16    //subnet for nano
#define FEATHERID     32    //subnet for feather
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Arduino:
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "rfm69hcwarduinos" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////#DEFINE SECTION AND GLOBAL VARIABLES/////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

//the speed at which the serial stream operates, keep less than or equal to 11250 for stability purposes
#define SERIAL_BAUD   9600

//********************Pin mapping for arduino feather 32u4*******************************//
  #define RFM69_CS      8
  #define RFM69_IRQ     7
  #define RFM69_IRQN    4  // Pin 7 is IRQ 4!
  #define RFM69_RST     4
  #define LED           13
  #define LED_ON        HIGH
  #define LED_OFF       LOW
  
  //pins used for the motor driver
  #define VCC           A4
  #define GND           11
  #define STBY          A3
  #define PWMA          5
  #define AIN1          A1
  #define AIN2          6
  #define PWMB          10
  #define BIN1          A2
  #define BIN2          9
  
  //pins used for Signal Strength Indicator
  #define LED1          12
  #define LED2          A0
  #define LED3          A5

  Motor motor1 = Motor(AIN1, AIN2, PWMA, 1, STBY);
  Motor motor2 = Motor(BIN1, BIN2, PWMB, 1, STBY);

  //turn on Automatic Transmission Control (if enabled)
  #ifdef ENABLE_ATC
    RFM69_ATC radio = RFM69_ATC(RFM69_CS, RFM69_IRQ, true, RFM69_IRQN);
  #else
    RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, true, RFM69_IRQN);
  #endif
  
  //set to 'true' to sniff all packets on the same network
  bool promiscuousMode = false;
  //Power level variable, 2 is medium speed
  int power = 5;
  //For pulse width conversion
  int pwm = 51*power;
  //Count of how many packets were received/transmitted
  uint32_t packetCount = 0;
  //Used for converting received packets to a string value
  String data;
  //Used for a timer for when the LEDs go into standby mode
  unsigned long startTime;
  unsigned long currentTime;
  int counter = 0;

  //For moving the motor forward
  void forward_drive(int pulse_width)
  {
    digitalWrite(BIN2,HIGH);
    digitalWrite(BIN1,LOW);
    analogWrite(PWMB,pulse_width);
    //digitalWrite(PWMB,HIGH);
    digitalWrite(AIN2,HIGH);
    digitalWrite(AIN1,LOW);
    analogWrite(PWMA,pulse_width);
    //digitalWrite(PWMA,HIGH);
  }
  
  //For moving the motor backwards
  void backward_drive(int pulse_width)
  {
    digitalWrite(BIN2,LOW);
    digitalWrite(BIN1,HIGH);
    analogWrite(PWMB,pulse_width);
    digitalWrite(AIN2,LOW);
    digitalWrite(AIN1,HIGH);
    analogWrite(PWMA,pulse_width);
  }
  
  //For turning left
  void turnLeft(int pulse_width)
  {
    digitalWrite(BIN2,HIGH);
    digitalWrite(BIN1,LOW);
    analogWrite(PWMB,pulse_width);
    digitalWrite(AIN2,LOW);
    digitalWrite(AIN1,HIGH);
    analogWrite(PWMA,pulse_width);
  }
  
  //For turning right
  void turnRight(int pulse_width)
  {
    digitalWrite(BIN2,LOW);
    digitalWrite(BIN1,HIGH);
    analogWrite(PWMB,pulse_width);
    digitalWrite(AIN2,HIGH);
    digitalWrite(AIN1,LOW);
    analogWrite(PWMA,pulse_width);
  }

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////SETUP() SECTION//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  //Sets up Signal Strength Indicator
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  //we have to set PWM pins as output
  pinMode(VCC, OUTPUT);     //Vcc
  pinMode(GND, OUTPUT);     //GND
  pinMode(STBY, OUTPUT);    //STBY
  
  pinMode(PWMA, OUTPUT);    //PWMA
  pinMode(AIN1, OUTPUT);    //Ain1
  pinMode(AIN2, OUTPUT) ;   //AIN2
  
  pinMode(PWMB, OUTPUT);    //PWMB
  pinMode(BIN1, OUTPUT);    //Bin1
  pinMode(BIN2, OUTPUT);    //Bin2

  //Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  //Prepare RFM69 radio for use
  radio.initialize(FREQUENCY,FEATHERID,NETWORKID);

    //Turns on high power mode if it is the HW (high power) chip
  #ifdef IS_RFM69HW
    radio.setHighPower(); //only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  //radio.setFrequency(919000000); //set frequency to some custom frequency

  //sets up the on-board LED
  pinMode(LED, OUTPUT);
  
  //enables automatic transmission control if set to true in the define statement
  #ifdef ENABLE_ATC
    radio.enableAutoPower(-80);
  #endif

  //resets timer
  startTime = 0;
  currentTime = millis();
  }

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////MAIN() SECTION///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //forward(255);
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// FLASH LEDS WHILE WAITING FOR A SIGNAL////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
    currentTime = millis();
    if(currentTime - startTime >= 400) {
      if(counter == 0) {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        counter++;
      }
      else if(counter == 1) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, LOW);
        counter++;
      }
      else if(counter == 2) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, HIGH);
        counter++;
      }
      else if(counter == 3) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        counter = 0;
      }
      startTime = currentTime;
    }

/*****************************************************************************************/
/***************************Setup for TB6612 Motor Driver*********************************/
/*****************************************************************************************/
  digitalWrite(STBY,HIGH);
  digitalWrite(VCC,HIGH);
  digitalWrite(GND,LOW);

  //resets motors
  digitalWrite(BIN2,LOW);
  digitalWrite(BIN1,LOW);
  digitalWrite(AIN2,LOW);
  digitalWrite(AIN1,LOW);

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////RECEPTION SECTION/////////////////////////////////////////////
////////////////////////RECEIVE SIGNAL FROM NANO///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
  if (radio.receiveDone())
  {
    //save received symbols to string
    data = (char*)radio.DATA;

    /********************** SECTION FOR RECEIVING DIGITAL BUTTONS**********************************/
    //if a digital value was sent, drive motors
    if(data.length() == 1) {
      //if the down button was pressed, reverse
        if(data[0] == 'a') {
          forward(motor1, motor2, pwm);
          delay(10);
        }
        //if the up button was pressed, drive forwards
        if(data[0] == 'y') {
          forward(motor1, motor2, -pwm);
          delay(10);
        }
        //if the right button was pressed, turn right
        if(data[0] == 'b') {
          turnRight(int(pwm/(1.5)));
          delay(10);
        }
        //if the left button was pressed, turn left
        if(data[0] == 'x') {
          turnLeft(int(pwm/(1.5)));
          delay(10);
        }
        //if the power down button is pressed, power motors down
        if(data[0] == 'd') {
          if(power > 1)
          power-=1;
        }
        //if the power up button is pressed, increase max power to motors
        /******** ADD COMMANDS FOR LED DISPLAY FOR THE FUTURE***********/
        if(data[0] == 'u') {
          if(power < 5)
          power+=1;
        }
    }
    
    /********************** SECTION FOR RECEIVING ANALOG JOYSTICK**********************************/
    if(data.length() > 1) {
        //grab the data and save it as a variable
        int num = data.toInt();
        //analog conversion factor
        double analog_conversion = 0.007;
        //numbers less than 320 are left oriented x axis values, so turn left
        if(data.toInt() < 320) {
          //for testing purposes
          Serial.println("Turning left with analog!");
          //turnLeft(pwm-pwm*analog_conversion*data.toInt());
        }
        //numbers greater than 380 and less than 1000 are right oriented x axis values, so turn right
        if(data.toInt() > 380 && data.toInt() < 1000) {
          //for testing purposes
          Serial.println("Turning right with analog!");
          //turnRight(pwm*analog_conversion*data.toInt());
        }
        //numbers greater than 1000 and less than 1320 are down oriented y axis values, so reverse
        num -= 1000; //to put the value back to the original range (same range as x axis values)
        if(data.toInt() >= 1000 && data.toInt() < 1320) {
          //for testing purposes
          Serial.println("Reversing with analog!");
          //backward(pwm-pwm*analog_conversion*(data.toInt() - 1000));
        }
        //numbers greater than 1380 are up oriented y axis values, so forwards
        if(data.toInt() > 1380) {
          //for testing purposes
          Serial.println("Driving forward with analog!");
          //forward(pwm*analog_conversion*(data.toInt() - 1000));
        }    
    }

    /********************** SECTION FOR SIGNAL STRENGTH INDICATOR **********************************/
    int signal_strength = radio.RSSI;
    //very strong signal
    if(signal_strength > -25) {
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
    }
    //strong signal
    else if(signal_strength <= -25 && signal_strength >= -50) {
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
    }
    //medium strength signal
    else if(signal_strength < -50 && signal_strength >= -75) {
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, LOW);
    }
    //weak signal
    //this is the "floor" 
    else if(signal_strength < -75 && signal_strength >= -120) {
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
    }
    
    //Flash LED to show that the Feather received a signal
    Blink(LED,3);
    //reset timer so there is a gap between showing the signal strength and the standby pattern
    startTime = currentTime;
    counter = 0;
  }
}

//Pulses on-board LED
void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
