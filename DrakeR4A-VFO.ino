/*  
 * ------------------------------------------------------------------------
 * "PH2LB LICENSE" (Revision 1) : (based on "THE BEER-WARE LICENSE" Rev 42) 
 * <lex@ph2lb.nl> wrote this file. As long as you retain this notice
 * you can do modify it as you please. It's Free for non commercial usage 
 * and education and if we meet some day, and you think this stuff is 
 * worth it, you can buy me a beer in return
 * Lex Bolkesteijn 
 * ------------------------------------------------------------------------ 
 * Filename : DRAKER4A-VFO.ino  
 * Version  : 0.1(DRAFT)
 * ------------------------------------------------------------------------
 * Description : A Arduino DDS based VFO for the DRAKE R4A with US bandplan
 * ------------------------------------------------------------------------
 * Revision : 
 *  - 2017-may-9  0.1 initial version 
 * ------------------------------------------------------------------------
 * Hardware used : 
 *  - Arduino Uno R3 
 *  - QRP-Labs Arduino Shields (http://qrp-labs.com/uarduino.html) 
 *  - LCD keypad shield (http://www.hobbytronics.co.uk/arduino-lcd-keypad-shield)  
 *  - Simpel rotary encoder (remember you need  real gray encoder not a pulsetrain)
 *  - potentiometer when RIT is required.
 * ------------------------------------------------------------------------
 * Software used : 
 *  - AD9850 library from Poul-Henning Kamp  
 * ------------------------------------------------------------------------ 
 * TODO LIST : 
 *  - add more sourcode comment
 *  - add debounce for the keys  
 * ------------------------------------------------------------------------ 
 */

// choose one of below
 

// some offsets for the VFO (there can be a few hz off)
// measure it with values 0 and when to much use negfreqoffset
// when not enought use posfreqoffset
uint32_t negfreqoffset = 679; // in hz.
uint32_t posfreqoffset = 0; // in hz
 
 
#include <LiquidCrystal.h> 
#include "AD9850.h" 
AD9850 ad(3, A5, A4); // w_clk, fq_ud, d7 
// Display shield : 
LiquidCrystal lcd(8,9,4,5,6,7);   // check you're pinning!!!! 
#define buttonAnalogInput A0   // A0 on the LCD shield (save some pins)
#define ritAnalogInput A3 
#define encoderPin1 2
#define encoderPin2 A2        // cant use 3 because of AD9850 :-(

 
// DRAKER4 VFO limits (4955Khz - 5495Khz)
uint32_t vfofreqlowerlimit = 4955e3; 
uint32_t vfofrequpperlimit = 5495e3; 
uint32_t rit = 0; // a 0..2048 freq offset.


// for a little debounce (needs to be better then now)
int_fast32_t prevtimepassed = millis(); // int to hold the arduino miilis since startup

// define the bandstruct
typedef struct 
{
  char * Text;
  uint32_t FreqLowerLimit; // lower limit acording to band plan
  uint32_t FreqUpperLimit; // upper limit according to bandplan
  uint32_t FreqBase; // the base frequency (first full 500Khz block from FreqLowerLimit)
  uint32_t Freq; // the current frequency on that band (set with default)
} 
BandStruct;
 
// define the band enum
typedef enum 
{
  BANDMIN = 0,
  B160M = 0,
  B80M = 1,
  B40M = 2,
  B30M = 3,
  B20M = 4,
  B17M = 5, 
  B15M = 6,
  B12M = 7,
  B10AM = 8,
  B10BM = 9,
  B10CM = 10,
  B10DM = 11,
  BANDMAX = 11
} 
BandEnum;

// define the bandstruct array (PA country full-license HF Band plan)
BandStruct  Bands [] =
{
  { (char *)"160M", (uint32_t)1810e3, (uint32_t)2000e3, (uint32_t)1500e3, (uint32_t)1860e3  }, // (uint32_t) to prevent > warning: narrowing conversion of '1.86e+6' from 'double' to 'uint32_t {aka long unsigned int}' inside { } [-Wnarrowing]
  { (char *)" 80M", (uint32_t)3500e3, (uint32_t)4000e3, (uint32_t)3500e3, (uint32_t)3650e3  }, // (char *) to prevent > warning: deprecated conversion from string constant to 'char*' [-Wwrite-strings]
  { (char *)" 40M", (uint32_t)7000e3, (uint32_t)7300e3, (uint32_t)7000e3, (uint32_t)7100e3  },
  { (char *)" 30M", (uint32_t)10100e3, (uint32_t)10150e3, (uint32_t)10000e3, (uint32_t)10100e3  },
  { (char *)" 20M", (uint32_t)14000e3, (uint32_t)14350e3, (uint32_t)14000e3, (uint32_t)14175e3  },
  { (char *)" 17M", (uint32_t)18068e3, (uint32_t)18168e3, (uint32_t)18000e3, (uint32_t)18100e3  },
  { (char *)" 15M", (uint32_t)21000e3, (uint32_t)21450e3, (uint32_t)21000e3, (uint32_t)21225e3  },
  { (char *)" 12M", (uint32_t)24890e3, (uint32_t)24990e3, (uint32_t)24500e3, (uint32_t)24900e3  },
  { (char *)"10Ma", (uint32_t)28000e3, (uint32_t)28500e3, (uint32_t)28000e3, (uint32_t)28225e3  },
  { (char *)"10Mb", (uint32_t)28500e3, (uint32_t)29000e3, (uint32_t)28500e3, (uint32_t)28750e3  },
  { (char *)"10Mc", (uint32_t)29000e3, (uint32_t)29500e3, (uint32_t)29000e3, (uint32_t)29250e3  },
  { (char *)"10Md", (uint32_t)29500e3, (uint32_t)29700e3, (uint32_t)29500e3, (uint32_t)29600e3  }
}; 

// define the stepstruct
typedef struct 
{
  char * Text;
  uint32_t Step;
} 
StepStruct;

// define the step enum
typedef enum 
{
  STEPMIN = 0,
  S10 = 0,
  S25 = 1,
  S100 = 2,
  S250 = 3,
  S1KHZ = 4,
  S2_5KHZ = 5,
  S10KHZ = 6,
  STEPMAX = 6
} 
StepEnum;

// define the bandstruct array
StepStruct  Steps [] =
{
  { (char *)"10Hz", 10 }, 
  { (char *)"25Hz", 25 }, 
  { (char *)"100Hz", 100 }, 
  { (char *)"250Hz", 250 }, 
  { (char *)"1KHz", 1000 }, 
  { (char *)"2.5KHz", 2500 }, 
  { (char *)"10KHz", 10000 }
};

// Switching band stuff
boolean switchBand = false;
int currentBandIndex = (int)B40M; // my default band 
int currentFreqStepIndex = (int)S10; // default 10Hz.


// Encoder stuff
int encoder0PinALast = LOW; 

volatile int lastEncoded = 0;
volatile long encoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;


#define debugSerial Serial

#define debugPrintLn(...) { if (debugSerial) debugSerial.println(__VA_ARGS__); }
#define debugPrint(...) { if (debugSerial) debugSerial.print(__VA_ARGS__); }


#define STEPUP    60
#define STEPDOWN  600
#define BANDUP    200
#define BANDDOWN  400
#define SELECT    800

int nrOfSteps = 0;

// LCD stuff
boolean updatedisplayfreq = false;
boolean updatedisplaystep = false;
boolean updatedisplaybandselect= false;

// Playtime
void setup() 
{
  debugSerial.begin(57600); 
  debugPrintLn(F("setup"));
    
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2); 
  
  lcd.setCursor(0,0); 
  lcd.print(F("DRAKER4 DDS VFO ")); 
  lcd.setCursor(0,1);
  lcd.print(F("    by PH2LB    ")); 
  delay(1000);  
 
  updatedisplayfreq = true;
  updatedisplaystep = true;
  
  updateDisplays();

  debugPrintLn(F("start loop")); 
 

  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, FALLING);  
  setFreq();
} 


void updateEncoder()
{

  // a falling edge on encoderPin1 will call this routine.
  // keep it short and simple. 
#ifdef TRUEGRAYENCODER
  // when having a true gray encoder (one click = one bit change)
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time
#else
  // when using a cheap gray encoder (one click = a full train pulses)
  int encoderPin2State = digitalRead(encoderPin2); //LSB = least significant bit
  if (encoderPin2State == 1)  
    encoderValue ++;
  else
    encoderValue --;
#endif

}




// function to set the AD9850 to the VFO frequency depending on bandplan.
void setFreq()
{ 
 

  // now we know what we want, so scale it back to the 5Mhz - 5.5Mhz freq
  uint32_t frequency = Bands[currentBandIndex].Freq - Bands[currentBandIndex].FreqBase; 
  uint32_t draker4freq = frequency + vfofreqlowerlimit - negfreqoffset + posfreqoffset - 1024 + rit;
 
  ad.setfreq(draker4freq); 
} 

// function to update . . the LCD 
void updateDisplays()
{
  if (updatedisplayfreq)
  {
    float freq = Bands[currentBandIndex].Freq / 1e3; // because we display Khz and the Freq is in Mhz.
 
    lcd.setCursor(0,0);
    lcd.print(F("                "));
    lcd.setCursor(2,0);
    if (freq < 10e3)
    {
      lcd.setCursor(3,0);
    }
    lcd.print(freq);
    lcd.print(F(" KHz"));
    // add RIT marker 0..1000 = -, 1048..2048 = +)
    if (rit < 500)
      lcd.print(F("--")); 
    else if (rit < 1000)
      lcd.print(F("-"));
    else if (rit > 1548)
      lcd.print(F("++"));
    else if (rit > 1048)
      lcd.print(F("+"));
  }

  if (updatedisplaystep)
  { 
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(2,1);
    lcd.print(Bands[currentBandIndex].Text);
    lcd.print(F("    "));
    lcd.print(Steps[currentFreqStepIndex].Text);
 
  }
  
  if (updatedisplaybandselect)
  {
    updatedisplaybandselect = false; 
    lcd.setCursor(0,0);
    lcd.print(F("  Band select   "));
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(0,1);

    if (currentBandIndex > BANDMIN)
    {
      lcd.print(Bands[currentBandIndex - 1].Text);
    }
    lcd.print(F(" "));
    lcd.print(F("["));
    lcd.print(Bands[currentBandIndex].Text);
    lcd.print(F("]"));
    lcd.print(F(" "));
    if (currentBandIndex < BANDMAX)
    {
      lcd.print(Bands[currentBandIndex + 1].Text);
    } 
  }
}
 
boolean ccw = false;
boolean cw = false;


// the main loop
void loop() 
{  
  boolean updatefreq = false;

  updatedisplayfreq = false;
  updatedisplaystep = false;

  nrOfSteps = encoderValue;
   
  encoderValue = 0; 

  int_fast32_t timepassed = millis(); // int to hold the arduino miilis since startup
  if (((timepassed - prevtimepassed) > 100 && !switchBand ) || 
      ((timepassed - prevtimepassed) > 250 && switchBand ) ||
      (prevtimepassed > timepassed) || 
      nrOfSteps != 0)
  { 
    
    ccw = false;
    cw = false;
    if (nrOfSteps != 0)
    {
      debugPrintLn(nrOfSteps); 
      if ( nrOfSteps < 0)
        ccw = true;
      else 
        cw = true;
    }

    uint32_t currentrit = analogRead(ritAnalogInput) * 2;  // range 0 .. 1023 so center is aprx 512 scale it up to 0...2048 (apr +- 1Khz rit)
    debugPrint(F("rit = "));
    debugPrintLn(rit);
    if (currentrit > rit + 5 || currentrit < rit - 5) // +- 5 for jitter.
    {
       rit = currentrit;
       updatefreq = true;
       updatedisplayfreq = true;
    }
      
    prevtimepassed = timepassed;
    int button;
    // check if a button is pressed
    button = analogRead (buttonAnalogInput);
    
   if (button < STEPUP) 
    {
      //right 
      if (currentFreqStepIndex < STEPMAX)
      {
        currentFreqStepIndex = (StepEnum)currentFreqStepIndex + 1;
        updatedisplaystep = true;
      }
    }
    else if (button < BANDUP || cw) 
    {
      // up
      if (switchBand)
      {
        if (currentBandIndex < BANDMAX)
        {
          currentBandIndex = (BandEnum)currentBandIndex + 1;
          updatedisplaybandselect = true;
        }
      }
      else
      {
        if (Bands[currentBandIndex].Freq < Bands[currentBandIndex].FreqUpperLimit)
        {
          Bands[currentBandIndex].Freq = Bands[currentBandIndex].Freq + Steps[currentFreqStepIndex].Step;
          updatefreq = true;
          updatedisplayfreq = true;
        }
        if (Bands[currentBandIndex].Freq > Bands[currentBandIndex].FreqUpperLimit)
        {
          Bands[currentBandIndex].Freq = Bands[currentBandIndex].FreqUpperLimit;
        }
      }
    }
    else if (button < BANDDOWN || ccw)
    {
      // down
      if (switchBand)
      {
        if (currentBandIndex > BANDMIN)
        {
          currentBandIndex = (BandEnum)currentBandIndex - 1;
          updatedisplaybandselect = true;
        }
      }
      else
      {
        if (Bands[currentBandIndex].Freq > Bands[currentBandIndex].FreqLowerLimit)
        {
          Bands[currentBandIndex].Freq = Bands[currentBandIndex].Freq - Steps[currentFreqStepIndex].Step;
          updatefreq = true;
          updatedisplayfreq = true;
        }
        if (Bands[currentBandIndex].Freq < Bands[currentBandIndex].FreqLowerLimit)
        {
          Bands[currentBandIndex].Freq = Bands[currentBandIndex].FreqLowerLimit;
        }
      }
    }
    else if (button < STEPDOWN)
    {
      //left
      if (currentFreqStepIndex > STEPMIN)
      {
        currentFreqStepIndex = (StepEnum)currentFreqStepIndex - 1;
        updatedisplaystep = true;
      }
    }
    else if (button < SELECT)
    {
      // select
      if (switchBand)
      {
        switchBand = false;
        // force update
        updatedisplayfreq = true;
        updatedisplaystep = true;
        updatefreq = true;
      }
      else
      {
        switchBand = true;
        updatedisplaybandselect = true;
      }
    }
  }

  if (updatefreq)
  { 
    setFreq();
  }

  updateDisplays(); 
 
}

