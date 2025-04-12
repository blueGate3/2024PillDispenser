#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

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
#define SERVO_WAIT_TIME 300

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
uint8_t servonum = 0;

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
}

void loop() {

  if(touchscreen.touched() && touchscreen.getPoint().z > 30) {
    //tft.fillScreen(TFT_WHITE); //refreshes
    placeNumberpadUI();
    TS_Point p = touchscreen.getPoint();
    tft.drawCentreString(String(getNumberPressed(p)), (SW/2)+20, SH/2, 6);
    if(getNumberPressed(p) == 0 || getNumberPressed(p) == 2 || getNumberPressed(p) == 1) {
      dispensePill();
    }
  } else {
    // tft.fillScreen(TFT_GREEN);
    placeNumberpadUI();
  }
    
}

void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
 
  pulselength = map(pulse, 0, 180, SERVOMIN, SERVOMAX); // Map the angle to the correct pulse length
  pwm.setPWM(n, 0, pulselength);
}

void dispensePill() {
  setServoPulse(servonum, 30);//50
  delay(SERVO_WAIT_TIME);
  setServoPulse(servonum, 120);//200
  delay(SERVO_WAIT_TIME);
  setServoPulse(servonum, 30);//50
  delay(SERVO_WAIT_TIME);
  setServoPulse(servonum, 120);//200
}

//reminder to have an ifPressed() check before we call this
int getNumberPressed(TS_Point p) {
  int x = map(p.x, 200, 3700, 0, SW); //should be 0, SCREEN_WIDTH? was 1, SCREEN_WIDTH
  int y = map(p.y, 240, 3800, 0, SH);
  int i = 0;
  while (i < 13){ //repeats through each number entry, including display SHOULD BE A WHILE LOOP IN A MIN UNTIL WE GET A MATCH
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
    //draws left line ISSUES, NOT NEEDED
    // tft.drawWideLine(
    //   float(numberpadArray[i][0]),
    //   float(numberpadArray[i][1]),
    //   float(numberpadArray[i][3]),
    //   float(numberpadArray[i][0]),
    //   float(1), 
    //   TFT_BLACK
    // );
    //draws right line
    tft.drawWideLine(
      float(numberpadArray[i][2]),
      float(numberpadArray[i][1]),
      float(numberpadArray[i][2]),
      float(numberpadArray[i][3]),
      float(1), 
      TFT_BLACK
    );
    // //draws bottom line //ISSUES, NOT NEEDED
    // tft.drawWideLine(
    //   float(numberpadArray[i][3]),
    //   float(numberpadArray[i][0]),
    //   float(numberpadArray[i][2]),
    //   float(numberpadArray[i][3]),
    //   float(1), 
    //   TFT_BLACK
    // );
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
