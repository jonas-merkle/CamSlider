/**
 * @brief CamSlider Arduino Sketch
 * @file CamSlider.ino
 * @date 2024-04-15
 * @author RZtronics <raj.shinde004@gmail.com> modified by Jonas Merkle [JJM] <jonas@jjm.one>
 * @license GNU General Public License v3.0
 */

///////////////////////////////////////////////////////////////////////////////////////
// Terms of use
///////////////////////////////////////////////////////////////////////////////////////
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
///////////////////////////////////////////////////////////////////////////////////////

//////////////
// Includes //
//////////////

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include "bitmap.h"


/////////////
// Defines //
/////////////

// Stepper
#define STEPPER_X_STEP_PIN
#define STEPPER_X_DIR_PIN
#define STEPPER_X_STEP_PIN
#define STEPPER_X_DIR_PIN

// Limit Switch
#define LIMIT_SWITCH_PIN

// Rotary Encoder
#define ROTARY_ENCODER_CLK_PIN
#define ROTARY_ENCODER_DT_PIN
#define ROTARY_ENCODER_SW_PIN

// OLED Display
#define OLED_RESET_PIN 4


/////////////
// Globals //
/////////////

// Stepper
AccelStepper StepperX(1, STEPPER_X_STEP_PIN, STEPPER_X_DIR_PIN);
AccelStepper StepperY(1, STEPPER_Y_STEP_PIN, STEPPER_Y_DIR_PIN);
MultiStepper StepperControl;

// OLED Display
Adafruit_SSD1306 Display(OLED_RESET_PIN);

// Variables
long gotoposition[2];
volatile long XInPoint = 0;
volatile long YInPoint = 0;
volatile long XOutPoint = 0;
volatile long YOutPoint = 0;
volatile long totaldistance = 0;
int flag = 0;
int temp = 0;
unsigned long switch0 = 0;
unsigned long rotary0 = 0;
float setspeed = 200;
float motorspeed;
float timeinsec;
float timeinmins;
volatile boolean TurnDetected;
volatile boolean rotationdirection;

/*
#define limitSwitch 11
#define PinSW 2
#define PinCLK 3
#define PinDT 8
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

AccelStepper stepper2(1, 7, 6); // (Type:driver, STEP, DIR)
AccelStepper stepper1(1, 5, 4);

MultiStepper StepperControl;
*/


//////////////////////////
// Function Definitions //
//////////////////////////

void Switch();
void Rotary();
void Home();
void SetSpeed();
void StepperPosition(int n);


///////////////////////////////
// Arduino default functions //
///////////////////////////////

void setup()
{
  // Initialize Serial Connection
  Serial.begin(9600);

  // Initialize I/O-Pins
  pinMode(STEPPER_X_STEP_PIN, OUTPUT);
  pinMode(STEPPER_X_DIR_PIN, OUTPUT);
  pinMode(STEPPER_Y_STEP_PIN, OUTPUT);
  pinMode(STEPPER_Y_DIR_PIN, OUTPUT);
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_DT_PIN, INPUT_PULLUP);
  piMode(OLED_RESET_PIN, OUTPUT);

  // Initialize Stepper Motors
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  StepperControl.addStepper(stepper1);
  StepperControl.addStepper(stepper2);

  // Initialize OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // display Boot logo
  display.drawBitmap(0, 0, CamSlider, 128, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();

  // Move into Home Position
  Home();

  // Attach Interrupts
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), Switch, RISING);
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK_PIN), Rotary, RISING);
}

void loop() {

  // Begin Setup
  if (flag == 0)
  {
    display.clearDisplay();
    display.drawBitmap(0, 0, BeginSetup, 128, 64, 1);
    display.display();
    setspeed = 200;
  }

  // SetXin
  if (flag == 1)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 28);
    display.println("Set X In");
    display.display();
    while (flag == 1)
    {
      stepperposition(1);
    }
    XInPoint = stepper1.currentPosition();
  }
  // SetYin
  if (flag == 2)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 28);
    display.println("Set Y In");
    display.display();
    while (flag == 2)
    {
      stepperposition(2);
    }
    stepper2.setCurrentPosition(0);
    YInPoint = stepper2.currentPosition();
  }
  // SetXout
  if (flag == 3)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 28);
    display.println("Set X Out");
    display.display();
    while (flag == 3)
    {
      stepperposition(1);
      Serial.println(stepper1.currentPosition());
    }
    XOutPoint = stepper1.currentPosition();
  }
  // SetYout
  if (flag == 4)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 28);
    display.println("Set Y Out");
    display.display();
    while (flag == 4)
    {
      stepperposition(2);
    }
    YOutPoint = stepper2.currentPosition();
    display.clearDisplay();

    // Go to IN position
    gotoposition[0] = XInPoint;
    gotoposition[1] = YInPoint;
    display.clearDisplay();
    display.setCursor(8, 28);
    display.println(" Preview  ");
    display.display();
    stepper1.setMaxSpeed(3000);
    StepperControl.moveTo(gotoposition);
    StepperControl.runSpeedToPosition();
  }

  // Display Set Speed
  if (flag == 5)
  {
    display.clearDisplay();
    display.setCursor(8, 28);
    display.println("Set Speed");
    display.display();
  }
  // Change Speed
  if (flag == 6)
  {
    display.clearDisplay();
    SetSpeed();
  }
  // DisplayStart
  if (flag == 7)
  {
    display.clearDisplay();
    display.setCursor(30, 27);
    display.println("Start");
    display.display();
  }
  // Start
  if (flag == 8)
  {
    display.clearDisplay();
    display.setCursor(20, 27);
    display.println("Running");
    display.display();
    Serial.println(XInPoint);
    Serial.println(XOutPoint);
    Serial.println(YInPoint);
    Serial.println(YOutPoint);

    gotoposition[0] = XOutPoint;
    gotoposition[1] = YOutPoint;
    stepper1.setMaxSpeed(setspeed);
    StepperControl.moveTo(gotoposition);
    StepperControl.runSpeedToPosition();
    flag = flag + 1;
  }
  // Slide Finish
  if (flag == 9)
  {
    display.clearDisplay();
    display.setCursor(24, 26);
    display.println("Finish");
    display.display();
  }
  // Return to start
  if (flag == 10)
  {
    display.clearDisplay();
    Home();
    flag = 0;
  }
}


//////////////////////////////
// Function Implementations //
//////////////////////////////

void Switch()
{
  if (millis() - switch0 > 500)
  {
    flag = flag + 1;
  }
  switch0 = millis();
}

void Rotary()
{
  delay(75);
  if (digitalRead(PinCLK))
    rotationdirection = digitalRead(PinDT);
  else
    rotationdirection = !digitalRead(PinDT);
  TurnDetected = true;
  delay(75);
}

void Home()
{
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  if (digitalRead(limitSwitch) == 1)
  {
    display.drawBitmap(0, 0, Homing, 128, 64, 1);
    display.display();
  }

  while (digitalRead(limitSwitch) == 1)
  {
    stepper1.setSpeed(-3000);
    stepper1.runSpeed();
  }
  delay(20);
  stepper1.setCurrentPosition(0);
  stepper1.moveTo(200);
  while (stepper1.distanceToGo() != 0)
  {
    stepper1.setSpeed(3000);
    stepper1.runSpeed();
  }
  stepper1.setCurrentPosition(0);
  display.clearDisplay();
}

void SetSpeed()
{
  display.clearDisplay();
  while (flag == 6)
  {
    if (TurnDetected)
    {
      TurnDetected = false; // do NOT repeat IF loop until new rotation detected
      if (rotationdirection)
      {
        setspeed = setspeed + 30;
      }
      if (!rotationdirection)
      {
        setspeed = setspeed - 30;
        if (setspeed < 0)
        {
          setspeed = 0;
        }
      }

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(30, 0);
      display.print("Speed");
      motorspeed = setspeed / 80;
      display.setCursor(5, 16);
      display.print(motorspeed);
      display.print(" mm/s");
      totaldistance = XOutPoint - XInPoint;
      if (totaldistance < 0)
      {
        totaldistance = totaldistance * (-1);
      }
      else
      {
      }
      timeinsec = (totaldistance / setspeed);
      timeinmins = timeinsec / 60;
      display.setCursor(35, 32);
      display.print("Time");
      display.setCursor(8, 48);
      if (timeinmins > 1)
      {
        display.print(timeinmins);
        display.print(" min");
      }
      else
      {
        display.print(timeinsec);
        display.print(" sec");
      }
      display.display();
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(30, 0);
    display.print("Speed");
    motorspeed = setspeed / 80;
    display.setCursor(5, 16);
    display.print(motorspeed);
    display.print(" mm/s");
    totaldistance = XOutPoint - XInPoint;
    if (totaldistance < 0)
    {
      totaldistance = totaldistance * (-1);
    }
    else
    {
    }
    timeinsec = (totaldistance / setspeed);
    timeinmins = timeinsec / 60;
    display.setCursor(35, 32);
    display.print("Time");
    display.setCursor(8, 48);
    if (timeinmins > 1)
    {
      display.print(timeinmins);
      display.print(" min");
    }
    else
    {
      display.print(timeinsec);
      display.print(" sec");
    }
    display.display();
  }
}

void StepperPosition(int n)
{
  stepper1.setMaxSpeed(3000);
  stepper1.setSpeed(200);
  stepper2.setMaxSpeed(3000);
  stepper2.setSpeed(200);
  if (TurnDetected)
  {
    TurnDetected = false; // do NOT repeat IF loop until new rotation detected
    if (n == 1)
    {
      if (!rotationdirection)
      {
        if (stepper1.currentPosition() - 500 > 0)
        {
          stepper1.move(-500);
          while (stepper1.distanceToGo() != 0)
          {
            stepper1.setSpeed(-3000);
            stepper1.runSpeed();
          }
        }
        else
        {
          while (stepper1.currentPosition() != 0)
          {
            stepper1.setSpeed(-3000);
            stepper1.runSpeed();
          }
        }
      }

      if (rotationdirection)
      {
        if (stepper1.currentPosition() + 500 < 61000)
        {
          stepper1.move(500);
          while (stepper1.distanceToGo() != 0)
          {
            stepper1.setSpeed(3000);
            stepper1.runSpeed();
          }
        }
        else
        {
          while (stepper1.currentPosition() != 61000)
          {
            stepper1.setSpeed(3000);
            stepper1.runSpeed();
          }
        }
      }
    }
    if (n == 2)
    {
      if (rotationdirection)
      {
        stepper2.move(-100);
        while (stepper2.distanceToGo() != 0)
        {
          stepper2.setSpeed(-3000);
          stepper2.runSpeed();
        }
      }
      if (!rotationdirection)
      {
        stepper2.move(100);
        while (stepper2.distanceToGo() != 0)
        {
          stepper2.setSpeed(3000);
          stepper2.runSpeed();
        }
      }
    }
  }
}
