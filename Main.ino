/*************************************************** 
  This is an example for our Adafruit 16-channel PWM & Servo driver
  Servo test - this will drive 8 servos, one after the other on the
  first 8 pins of the PCA9685

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815
  
  These drivers use I2C to communicate, 2 pins are required to  
  interface.

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include <TimeLib.h>
//possible project names AI generated:
//MEDIC: Measured Efficient Dose Intake Control
//MIND: Medication Intake Notification Device
//DOSE: Digitally Optimized Supply Engine
//iDOSE: Intelligent Digital Optimized Serving Equipment


#include <SPI.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd/ or https://RandomNerdTutorials.com/esp32-tft/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen
// Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SDA_PIN 22
#define SCL_PIN 27
#define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // This is the 'maximum' pulse length count (out of 4096)
#define USMIN  600 // This is the rounded 'minimum' microsecond length based on the minimum pulse of 150
#define USMAX  2400 // This is the rounded 'maximum' microsecond length based on the maximum pulse of 600
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2
uint8_t servonum = 0;

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

//times will be in minutes since 12:00 am, delays will be in ms
const int mStart = 480; //8:00 AM
const int mStop = 600; //10:00 AM
const int nStart = 1200; //8:00 PM
const int nStop = 1400; // 10:00 PM

const int settingsXMin = 280;
const int settingsYMin = 200;


bool hadMorning = false;
bool hadNight = false;
bool canResetBools = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, -1, 17);
  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(60);

  delay(10);

  tft.fillScreen(TFT_GREEN);
  setTime(23,59,45,0,0,0); // setTime(hr,min,sec,day,month,yr);
 



 
  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  touchscreen.setRotation(3);

  // Start the tft display
  tft.init();
  // Set the TFT display rotation in landscape mode
  tft.setRotation(1);

  // Clear the screen before writing to it
  // tft.fillScreen(TFT_WHITE);
  // tft.setTextColor(TFT_BLACK, TFT_WHITE);
  
  // Set X and Y coordinates for center of display
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
}

void loop() {
  // put your main code here, to run repeatedly:
  time_t t = now();
  scan(2);
  //resets pill bools at the end of the day

  if(minute(t) == 0 && hour(t) == 0 && canResetBools) { 
    hadMorning = false;
    hadNight = false;
    canResetBools = false;
  } if(minute(t) == 1 && hour(t) == 0) {
    canResetBools = true;
  }

  int currentTimeInMins = (hour(t)*60) + minute(t);
  

  if(!(5 < currentTimeInMins < 1435)) {

  } else {
    if(isAM()) {
      runAM(currentTimeInMins);
    } else {
      runPM(currentTimeInMins);
    }
  }

}


void changeLED(String color) {
  if(color == "red") {
  tft.fillScreen(TFT_RED);
  } else if (color == "green") {
    tft.fillScreen(TFT_GREEN);
  } else if (color == "blue") {
    tft.fillScreen(TFT_BLUE);
  }
}

void playSound() {

}

int scan(int whichAxis) {

  if (touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    if(x > settingsXMin && y > settingsYMin) {
      runSettings(); 
      x = -1;
      y = -1;
    
    } else if (touchscreen.touched()) {

    x=x;
    y=y;

    } else {
    x = -1;
    y = -1;
    }
  }

  if(whichAxis == 0) {
    return x;
  } else if (whichAxis == 1) {
    return y;
  } else if(whichAxis == 2) {
    return -1;
  }

}

void dispensePills(int servonum) { // NOTE: ASSUMING A 1:1 GEARING TO THE ROTATOR AND NO LOAD, WILL NEED TO CALIBRATE
//ADDITIONAL NOTE: WE NEED ONLY TO MOVE THE CAM A QUARTER WAY THEN BACK TO WHERE IT WAS
//ADDITIONAL NOTE 2.0: CAM NEEDS TO BE ROTATED TWICE SO MAKE THIS RUN TWICE
  delay(300);
  setServoPulse(servonum, 50);
  delay(300);
  setServoPulse(servonum, 200);
  delay(300);
  setServoPulse(servonum, 50);
  delay(300);
  setServoPulse(servonum, 200);
  
}

void alertMom() {

}



void runAM(time_t currentTime) {
  
  //if within time boundaries
  if((!hadMorning) && (mStart < currentTime < mStop)) {

    changeLED("green");
    playSound();

    while(!hadMorning) {
      int xVal = scan(0);
      int yVal = scan(1);
      time_t a = now();
      int morningTime = (hour(a)*60) + minute(a);

      if((mStart < morningTime < mStop) && (xVal != -1) && yVal != -1) {
        dispensePills(0); //0 = first tray
        changeLED("red");
        hadMorning = true;
      } else if (morningTime >= mStop) {
        alertMom();
        changeLED("red");
      }


    }

  } 

}

void runPM(time_t currentTime) {
  
  //if within time boundaries
  if((!hadNight) && (nStart < currentTime < nStop)) {

    changeLED("green");
    playSound();

    while(!hadNight) {
      int xVal = scan(0);
      int yVal = scan(1);
      time_t a = now();
      int nightTime = (hour(a)*60) + minute(a);

      if((nStart < nightTime < nStop) && (xVal != -1) && yVal != -1) {
        dispensePills(1); //0 = first tray
        changeLED("red");
        hadNight = true;
      } else if (nightTime >= nStop) {
        alertMom();
        changeLED("red");
      }


    }

  } 

}

void runSettings() {
  changeLED("blue");
}



void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
  
  pulselength = map(pulse, 0, 180, SERVOMIN, SERVOMAX); // Map the angle to the correct pulse length
  pwm.setPWM(n, 0, pulselength);
}