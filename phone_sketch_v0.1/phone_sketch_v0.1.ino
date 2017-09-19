#include <DFRobotDFPlayerMini.h>

///// arduino code setup 
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
//
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
void hangUp(void);
void liftHandset(void);
void dialProcess (int newDigit);
void playSong(int num);
int needToPrint = 0;
int dialCount = 0;
int inputRotary = 2;  // this is the gpio on which we read the rotary dial
int inputCradle = 3;  // this is the gpio on which we read the switch for handset 
int dialLast = HIGH;
int dialState = HIGH;
long dialTime = 0;
int cradleLast = HIGH;
int cradleState = HIGH;
long cradleTime = 0;
int cleared = 0;
int playing = 0;
int listening = 0;
int inputLength = 0;

// constants
int dialHasFinishedRotatingAfterMs = 500;
int debounceDelay = 5;

// arrays
int inputNumber[15];      // an array for storing a number dialed by the user.
#define homeNumberCount 10
// fill in this array with each country's full phone number;
// put the length of each phone number in the 0th array position and then the phone number, one digit at a time, left justified.
// if the longest phone number is more than 16 digits, increase that dimension of the array. trailing zeros don't matter.
const int homeNumbers[homeNumberCount][16] =
{
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

void setup()
{
  Serial.begin(9600);
  pinMode(inputRotary, INPUT_PULLUP); // try INPUT_PULLUP BECAUSE IT ADDS A PULLUP RESISTOR. SAVES USING EXTERNAL RESISTOR. BUT REMEMBER THAT LOGIC IS REVERSED.
  pinMode(inputCradle, INPUT_PULLUP);
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  //Use softwareSerial to communicate with mp3.
  if (!myDFPlayer.begin(mySoftwareSerial)) 
  {  
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(10);  //Set volume value. From 0 to 30
  myDFPlayer.play(1);  //Play the first mp3
}

void loop()
{
  unsigned long now = millis(); // keep time
  // monitor the cradle switch for hangup/pickup
  int cradleCurrent = digitalRead(inputCradle);
  if (cradleCurrent != cradleLast)
  {
    cradleTime = now;
  }
  if ((now - cradleTime) > debounceDelay) // reading is strong and stable
  {
    cradleState = cradleCurrent;
    if (cradleState == LOW)   // the hang-up case (will need to check hardware logic)
      hangUp();
    else                      // the pick-up case (check hardware logic)
      liftHandset();
  }
  cradleLast = cradleCurrent;
  
  // monitor the rotary dial for input.
  if (listening)
  {
    int reading = digitalRead(inputRotary);
    if (reading != dialLast) 
    {
      dialTime = now;
    }
    dialLast = reading;
    unsigned long tSinceDialed = now - dialTime; // does not handle clock rollover
    if (tSinceDialed > dialHasFinishedRotatingAfterMs) 
    {
      // the dial isn't being dialed, or has just finished being dialed.
      if (needToPrint) 
      {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
      int thisDigit = dialCount % 10;
      Serial.print(thisDigit, DEC);
      dialProcess(thisDigit); // submit a new digit to the dialing processor
      needToPrint = 0;
      dialCount = 0;
      cleared = 0;
      myDFPlayer.pause();  // pause the mp3 (only needed on first digit dialed but no matter)
      }
    } 
    if (tSinceDialed > debounceDelay) // stable reading
    {
      if (reading != dialState) 
      {
        // this means that the switch has either just gone from closed->open or vice versa.
        dialState = reading;
        if (dialState == LOW) 
        {
          // increment the count of pulses if it's gone high.
          dialCount++; 
          needToPrint = 1; // we'll need to print this number (once the dial has finished rotating)
        } 
      }
    }
  } // if (listening)
} 


/// arduino error handling
void printDetail(uint8_t type, int value)
{
  switch (type) 
  {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) 
      {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/// hangup script
void hangUp (void)
{
  myDFPlayer.pause();  // pause the mp3
  playing = 0;
  listening = 0;       // don't listen for dialing
  dialCount = 0;
  Serial.flush();
}

// code to run when handset is first lifted
// play greeting sound, start lisening for dials
void liftHandset (void)
{
  myDFPlayer.play(1);  // Play the intro/dialing prompt mp3
  listening = 1;       // start listening for dialing
  inputLength = 0;
}

// this code is called each time the user dials a digit.
// target phone numbers are stored in an array where the 0th array entry is the length of each number.
// number of digits dialed is counted; compare number of digits dialed to the length of each number
// if a match, then compare the user-input number to the number in the array.
// if numbers match, then play the appropriate song.
void dialProcess (int newDigit)
{
  // record the new digit
  inputNumber[inputLength++] = newDigit;
  // see if we have a match
  for (int n = 0; n < homeNumberCount; n++)
  {
    if (homeNumbers[n][0] == inputLength)     // check for matching number of digits
    {
      for (int m = 1; m <= inputLength; m++)  // compare each digit
      {
        if (inputNumber[m] != homeNumbers[n][m])
          return;
      }
      playSong(n);        // if we are here, then we have a match
    }
  }
}

// let me play you the song of my people.
// num corresponds to the index of the homeNumber array
// we can order the mp3s in the same order or provide a mapping here.
void playSong(int num)
{
  myDFPlayer.play(num);
  playing = 1;          // does anything depend on this?
  listening = 0;        // don't listen for dialing if a song is playing
}

