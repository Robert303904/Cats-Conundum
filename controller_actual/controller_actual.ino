////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////    NANO/TRANSMITTER CODE   //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
// This code was taken from the NODE.INO file and modified heavily to make our nano talk with the gamepad
// After the controller receives a command from the gamepad, it forwards the signal to the feather via the RFM69 chip
// Made by Thane Reishus-O'Brien

// Sample RFM69 receiver/gateway sketch, with ACK and optional encryption, and Automatic Transmission Control
// Passes through any wireless received messages to the serial port & responds to ACKs
// It also looks for an onboard FLASH chip, if present
// RFM69 library and sample code by Felix Rusu - http://LowPowerLab.com/contact
// Copyright Felix Rusu (2015)

//////////////////////#include libraries section///////////////////////////////////////////////
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NANOID        16    //subnet for nano
#define FEATHERID     32    //subnet for feather
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Arduino
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

//mapping for controller buttons on feather controller
  #define TOP_BUTTON    9
  #define BOTTOM_BUTTON 10
  #define LEFT_BUTTON   12
  #define RIGHT_BUTTON  6
  #define POWER_UP      11
  #define POWER_DOWN    5
  #define PIN_ANALOG_X  A2
  #define PIN_ANALOG_Y  A4
  #define RESET_BUTTON  A5
  
//Pin mapping for arduino feather 32u4
  #define RFM69_CS      8
  #define RFM69_IRQ     7
  #define RFM69_IRQN    4  // Pin 7 is IRQ 4!
  #define RFM69_RST     4
  #define LED           13
  #define LED_ON        HIGH
  #define LED_OFF       LOW
  
//turn on Automatic Transmission Control (if enabled)
#ifdef ENABLE_ATC
  RFM69_ATC radio = RFM69_ATC(RFM69_CS, RFM69_IRQ, true, RFM69_IRQN);
#else
  RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, true, RFM69_IRQN);
#endif

//set to 'true' to sniff all packets on the same network
bool promiscuousMode = false;

//Count of how many packets were received/transmitted
uint32_t packetCount = 0;

//For transmission packets
char data[4];

//For resetting the controller and forcing it to connect to computer
int reset = 0;

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////SETUP() SECTION//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  //Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  //Prepare RFM69 radio for useuse
  radio.initialize(FREQUENCY,NANOID,NETWORKID);

  //Turns on high power mode if it is the HW (high power) chip
#ifdef IS_RFM69HW
  radio.setHighPower(); //only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  //radio.setFrequency(919000000); //set frequency to some custom frequency
  
   // to enable pull up resistors first write pin mode
   // and then make that pin HIGH
   // sets up the digital pins as input from the gamepad
   pinMode(BOTTOM_BUTTON, INPUT);
   digitalWrite(BOTTOM_BUTTON, HIGH);
  
   pinMode(TOP_BUTTON, INPUT);
   digitalWrite(TOP_BUTTON, HIGH);
  
   pinMode(LEFT_BUTTON, INPUT);
   digitalWrite(LEFT_BUTTON, HIGH);
   
   pinMode(RIGHT_BUTTON, INPUT);
   digitalWrite(RIGHT_BUTTON, HIGH);
  
   pinMode(POWER_UP, INPUT);
   digitalWrite(POWER_UP, HIGH);
  
   pinMode(POWER_DOWN, INPUT);
   digitalWrite(POWER_DOWN, HIGH);

   pinMode(RESET_BUTTON, INPUT);
   digitalWrite(RESET_BUTTON, HIGH);
   
//enables automatic transmission control if set to true in the define statement
#ifdef ENABLE_ATC
  radio.enableAutoPower(-20);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////MAIN() SECTION///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void loop() {

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////TRANSMISSION SECTION//////////////////////////////////////////
////////////////////////////SEND SIGNAL TO FEATHER/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/********************** SECTION FOR READING DIGITAL PINS**********************************/
    if (digitalRead(BOTTOM_BUTTON) == LOW) {
      //sends the character "a" to the receiving radio
      radio.send(FEATHERID, "a", 1, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //right button on controller
    //turn car right
    if (digitalRead(RIGHT_BUTTON) == LOW) {
      //sends the character "b" to the receiving radio
      radio.send(FEATHERID, "b", 1, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //top button on controller
    //drive motor forwards
    if (digitalRead(TOP_BUTTON) == LOW) {
      //sends the character "y" to the receiving radio
      radio.send(FEATHERID, "y", 1, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //left button on controller
    //turn car left
    if (digitalRead(LEFT_BUTTON) == LOW) {
      //sends the character "x" to the receiving radio
      radio.send(FEATHERID, "x", 1, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //small right button on controller
    //turn motor speed up
    if (digitalRead(POWER_UP) == LOW) {
      //sends the character "u" to the receiving radio
      radio.send(FEATHERID, "u", 1, true);
      delay(500);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //small left button on controller
    //turn motor speed down
    if (digitalRead(POWER_DOWN) == LOW) {
      //sends the character "d" to the receiving radio
      radio.send(FEATHERID, "d", 1, true);
      delay(500);
      //Flash LED to show that the controller sent a signal
      Blink(LED,3);
    }
    //analog button on controller
    //puts controller into a state that should never have issues connecting with a PC
    if (digitalRead(RESET_BUTTON) == LOW) {
      reset++;
      Blink(LED,3);
      while(reset == 5) {
      //Flash LED to show that the controller sent a signal
      Blink(LED,20);
      if(digitalRead(RESET_BUTTON) == LOW)
        reset = 0;
      }
      delay(500);
    }
    
/********************** SECTION FOR READING ANALOG PINS**********************************/
    //reads analog values/positions and temporarily saves them
    int int_x = analogRead(PIN_ANALOG_X);
    int int_y = analogRead(PIN_ANALOG_Y);
    
    //if x axis has been moved, send current value
    if(int_x < 320 || int_x > 380) {
      //wraps the digital value of the analog pin in a string and sends the string
      itoa(analogRead(PIN_ANALOG_X), data, 10);
      //sends the x analog value to the receiving radio
      radio.send(FEATHERID, data, 3, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      //Blink(LED,3);
    }
    //if y axis has been moved, send current value
    if(int_y < 320 ||int_y > 380) {
      //wraps the digital value of the analog pin in a string and sends the string
      itoa(analogRead(PIN_ANALOG_Y) + 1000, data, 10);
      //sends the y analog value to the receiving radio
      radio.send(FEATHERID, data, 4, true);
      delay(10);
      //Flash LED to show that the controller sent a signal
      //Blink(LED,3);
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
