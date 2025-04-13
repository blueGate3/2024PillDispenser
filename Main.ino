#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <TimeLib.h>

TFT_eSPI tft = TFT_eSPI();// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS
#define SDA_PIN 22
#define SCL_PIN 27
#define SERVOMIN 150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX 600 // This is the 'maximum' pulse length count (out of 4096)
#define USMIN  600 // This is the rounded 'minimum' microsecond length based on the minimum pulse of 150
#define USMAX  2400 // This is the rounded 'maximum' microsecond length based on the maximum pulse of 600
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define SW 320 //screen width
#define SH 240 //screen height
#define SERVO_WAIT_TIME 700

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
uint8_t servonum = 0;

bool hasTimeSet = false;
bool hasReset = false; //has refreshed the screen

bool canReleaseMorning = true;
bool canReleaseNight = true; 
//MORNING: START AT 6 AM, GO TO NOON
//EVENING: START AT 8, GO TO MIDNIGHT

int mTime[] = {6, 0, 12, 0}; //start hr start min end hr end min
int nTime[] = {20, 0, 23, 59}; //start hr start min end hr end min

//converted all to minutes
int mSt = (mTime[0] * 60) + mTime[1];
int mSp = (mTime[2] * 60) + mTime[3];
int nSt = (nTime[0] * 60) + nTime[1];
int nSp = (nTime[2] * 60) + nTime[3];



  //first two are top left of the rectangle, second two are bottom right. Ordered X,Y or Width, Height. 
  int numberpadArray[12][5] = {
    { 0, 0, (SW/4), (SH/3), 1 }, //1
    { (SW/4) , 0, (SW/2), (SH/3), 2 }, //2
    { (SW/2), 0, ((SW*3)/4), (SH/3), 3 }, //3
    { ((SW*3)/4), 0, SW, (SH/3), 4 }, //4
    { 0, (SH/3), (SW/4), ((2*SH)/3), 5 }, //5
    { (SW/4), (SH/3), (SW/2), ((2*SH)/3), 10 }, //display
    { (SW/2), (SH/3), ((3*SW)/4), ((2*SH)/3), 11 }, //display
    { ((SW*3)/4), (SH/3), SW, ((2*SH)/3), 6 }, //6
    { 0, ((2*SH)/3), (SW/4), SH, 7 }, //7
    { (SW/4), (2*SH/3), (SW/2), SH, 8 }, //8
    { (SW/2), (2*SH/3), (3*SW/4), SH, 9 }, //9
    { (3*SW/4), (2*SH/3), SW, SH, 0 }  //0
  };

void setup() {
  // put your setup code here, to run once: 
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, -1, 17);
  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(60);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(3); 
  tft.init();
  tft.setRotation(3);

  placeNumberpadUI();
  
    setTime();
}

void loop() {
  // if(!hasTimeSet) {
  //   setTime();
  //   hasTimeSet = true; 
  // }
  Routine();
  // if(touchscreen.touched() && touchscreen.getPoint().z > 30) {
  //   //tft.fillScreen(TFT_WHITE); //refreshes
  //   placeNumberpadUI();
  //   TS_Point p = touchscreen.getPoint();
  //   tft.drawCentreString(String(getNumberPressed(p)), (3*SW/8), SH/2, 6);
  // } else {
  //   // tft.fillScreen(TFT_GREEN);
  //   placeNumberpadUI();
  // }
    
}

void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
 
  pulselength = map(pulse, 0, 180, SERVOMIN, SERVOMAX); // Map the angle to the correct pulse length
  pwm.setPWM(n, 0, pulselength);
}

//releases a pill at the desired leve. zero is for 1st, 1 is for 2nd, etc. 
void dispensePillServo(int level) {
  setServoPulse(level, 50);//50
  delay(SERVO_WAIT_TIME);
  setServoPulse(level, 200);//200
  delay(SERVO_WAIT_TIME);
  setServoPulse(level, 50);//50
  delay(SERVO_WAIT_TIME);
  setServoPulse(level, 200);//200
}

//reminder to have an ifPressed() check before we call this
int getNumberPressed(TS_Point p) {
  int x = map(p.x, 200, 3700, 0, SW); //should be 0, SCREEN_WIDTH? was 1, SCREEN_WIDTH
  int y = map(p.y, 240, 3800, 0, SH);
  int i = 0;
  while (i < 13){
    bool xMatch = (numberpadArray[i][0] < x && x < numberpadArray[i][2]);
    bool yMatch = (numberpadArray[i][1] < y && y < numberpadArray[i][3]);
    if(xMatch && yMatch) {
      return numberpadArray[i][4];
      break;
    }
    i++;
  }
}

void placeNumberpadUI() {
  for(int i =0; i<= 11; i++) {
    //draws top line
    tft.drawWideLine(
      float(numberpadArray[i][0]),
      float(numberpadArray[i][1]),
      float(numberpadArray[i][2]),
      float(numberpadArray[i][1]),
      float(1), 
      TFT_BLACK
    );
    //draws right line
    tft.drawWideLine(
      float(numberpadArray[i][2]),
      float(numberpadArray[i][1]),
      float(numberpadArray[i][2]),
      float(numberpadArray[i][3]),
      float(1), 
      TFT_BLACK
    );
  if(i == 5 || i == 6) {
    //avoids putting numbers in the center display
  } else {
   tft.drawCentreString(
      String(numberpadArray[i][4]), 
      (((numberpadArray[i][2] - numberpadArray[i][0]))/2) + numberpadArray[i][0], //places the number in the center of the x and y of the boxes
      (((numberpadArray[i][3] - numberpadArray[i][1]))/2) + numberpadArray[i][1],
      6
    ); 
  }

  }
}

void setTime() {
  //Gets the four numbers needed. Not based on anything time delay, we just wait until we have input
  int numbersRegistered = 0;
  int initialTime[4] = {0, 0, 0, 0};
  while(numbersRegistered < 4) { //4 total runs, 0-3 to get all 4 numbers
    if(touchscreen.touched()) {
       TS_Point point = touchscreen.getPoint();
      initialTime[numbersRegistered] = getNumberPressed(point);
      delay(800);
      numbersRegistered++;
    }
  }
  //Concatenates integers so our times are correct. Typing in 6, 0, will get you 60. But 0,6, will not get you 06, only 6. 
    int startHour = concatenateIntegers(initialTime[0], initialTime[1]);
    int startMinute = concatenateIntegers(initialTime[2], initialTime[3]);
  //Adds "12" to the hours if it's in PM

  //Sets the time within the clock
  setTime(startHour, startMinute,55,0,0,0); // setTime(hr,min,sec,day,month,yr); //55 for seconds is temporary so i don't have to wait a full minute sometimes
  // String timeString = String(startHour + " : " + startMinute);
  // tft.drawCentreString(timeString, (SW/4), SH/2, 4);
  
}

void writeTime(uint32_t color) {
  time_t currentTime = now();
  tft.drawCentreString(String(hour()), 100, 100, 4); //temp
  tft.drawCentreString(String(minute()), 125, 125, 4); //temp
  tft.drawCentreString(String(second()), 150, 150, 4); //temp
  if ((second() == 0 || second() == 10  || second() == 20  || second() == 30  || second() == 40  || second() == 50) && !hasReset) {
    tft.fillScreen(color);
    hasReset = true;
  } else if (second() != 0 || second() != 10  || second() != 20  || second() != 30  || second() != 40  || second() != 50) {
    hasReset = false; 
  }
}

//gets the current clock time. if true, returns hour. else minute
int getTime(bool returnHour) {
  time_t currentTime = now();
  int returnValue = -1;
  if (returnHour) {
    returnValue = hour();
  } else {
    returnValue = int(minute());
  }
}

int concatenateIntegers(int first, int second) {
  return (first*10) + second;
}

//returns 1 for morning, 2 for night, or 3 for no. 
int withinTimeConstraints() {
  int minuteTime = (hour() * 60) + minute(); //converts current time into minutes

  if((mSt <= minuteTime) && (minuteTime <= mSp)) {
    return 1;
  } else if ((nSt <= minuteTime) && (minuteTime <= nSp)) {
    return 2;
  } else {
    return 3;    
  }
}

void Routine() {
  uint32_t color = TFT_WHITE;
  if ((withinTimeConstraints() == 1) && canReleaseMorning) { //for morning
    color = TFT_GREEN;
    if(touchscreen.touched()) {
      canReleaseMorning = false; //flip first thing so it cannot reset.
      dispensePillServo(0);
      color = TFT_RED;
    }
  } else if (withinTimeConstraints() == 2 && canReleaseNight) {
    color = TFT_YELLOW; //change to green later, this is just for testing.
    if(touchscreen.touched()) {
      canReleaseNight = false; 
      dispensePillServo(1);
      color = TFT_RED;
    }
  } else if ((withinTimeConstraints() == 3)){
    //resets the booleans in the middle of the day. it won't happen while either the morning or night functions are primed, but it will at all other times when the current time isn't within the ranges of the dispense times. 
    canReleaseMorning = true;
    canReleaseNight = true;
    color = TFT_RED;
  }  else if (!canReleaseMorning || !canReleaseNight) {
    color = TFT_RED;
  }
  writeTime(color);
}
