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

/**
 * @attention
 *
 * The following function must be changed in the Multistepper library:
 * void MultiStepper::moveTo(long absolute[])
{
    // First find the stepper that will take the longest time to move
    float longestTime = 0.0;

    uint8_t i;
    for (i = 0; i < _num_steppers; i++)
    {
        long thisDistance = absolute[i] - _steppers[i]->currentPosition();
        float thisTime = abs(thisDistance) / _steppers[i]->maxSpeed();

        if (thisTime > longestTime) {
            longestTime = thisTime;
        }
    }

    if (longestTime > 0.0)
    {
        // Now work out a new max speed for each stepper so they will all
        // arrived at the same time of longestTime
        for (i = 0; i < _num_steppers; i++) {
            long thisDistance = absolute[i] - _steppers[i]->currentPosition();
            float thisSpeed = thisDistance / longestTime;
            _steppers[i]->moveTo(absolute[i]); // New target position (resets speed)
            if (abs(thisSpeed) > _steppers[i]->maxSpeed()) {
                if (thisSpeed >= 0) {
                    _steppers[i]->setSpeed(_steppers[i]->maxSpeed()); // New speed
                }
                else {
                    _steppers[i]->setSpeed(-_steppers[i]->maxSpeed()); // New speed
                }
            }
            else {
                _steppers[i]->setSpeed(thisSpeed);
            }
        }
    }
}
 */

//////////////
// Includes //
//////////////

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include "bitmap.h"


/////////////
// Defines //
/////////////

// Stepper
#define STEPPER_X_STEP_PIN 7
#define STEPPER_X_DIR_PIN 4
#define STEPPER_Y_STEP_PIN 6
#define STEPPER_Y_DIR_PIN 5

// Limit Switch
#define LIMIT_SWITCH_PIN 11

// Rotary Encoder
#define ROTARY_ENCODER_CLK_PIN 3
#define ROTARY_ENCODER_DT_PIN 9
#define ROTARY_ENCODER_SW_PIN 2

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
int lastStateCLK;
int currentStateCLK;
unsigned long switch0 = 0;
unsigned long rotary0 = 0;
float setspeed = 200;
float motorspeed;
float timeinsec;
float timeinmins;
volatile boolean TurnDetected;
volatile boolean rotationdirection;


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

void setup() {
    // Initialize Serial Connection
    Serial.begin(9600);

    // Initialize I/O-Pins
    pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ROTARY_ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(OLED_RESET_PIN, OUTPUT);

    // Initialize Stepper Motors
    StepperX.setMaxSpeed(1000);
    StepperX.setSpeed(200);
    StepperY.setMaxSpeed(1000);
    StepperY.setSpeed(200);
    StepperControl.addStepper(StepperX);
    StepperControl.addStepper(StepperY);

    // Initialize OLED Display
    Display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    Display.clearDisplay();

    // Display Boot logo
    Display.drawBitmap(0, 0, CamSlider, 256, 128, 1);
    Display.display();
    delay(2000);
    Display.clearDisplay();

    // Move into Home Position
    Home();

    // Attach Interrupts
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), Switch, RISING);
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK_PIN), Rotary, RISING);
    // Read the initial state of CLK
    lastStateCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);
}

void loop() {

    // Begin Setup
    if (flag == 0) {
        Display.clearDisplay();
        Display.drawBitmap(0, 0, BeginSetup, 256, 128, 1);
        Display.display();
        setspeed = 200;
    }

    // SetXin
    if (flag == 1) {
        Display.clearDisplay();
        Display.setTextSize(2);
        Display.setTextColor(WHITE);
        Display.setCursor(20, 10);
        Display.println("Set X In");
        Display.display();
        while (flag == 1) {
            StepperPosition(1);
        }
        XInPoint = StepperX.currentPosition();
    }
    // SetYin
    if (flag == 2) {
        Display.clearDisplay();
        Display.setTextSize(2);
        Display.setTextColor(WHITE);
        Display.setCursor(20, 10);
        Display.println("Set Y In");
        Display.display();
        while (flag == 2) {
            StepperPosition(2);
        }
        StepperY.setCurrentPosition(0);
        YInPoint = StepperY.currentPosition();
    }
    // SetXout
    if (flag == 3) {
        Display.clearDisplay();
        Display.setTextSize(2);
        Display.setTextColor(WHITE);
        Display.setCursor(20, 10);
        Display.println("Set X Out");
        Display.display();
        while (flag == 3) {
            StepperPosition(1);
        }
        XOutPoint = StepperX.currentPosition();
    }
    // SetYout
    if (flag == 4) {
        Display.clearDisplay();
        Display.setTextSize(2);
        Display.setTextColor(WHITE);
        Display.setCursor(20, 10);
        Display.println("Set Y Out");
        Display.display();
        while (flag == 4) {
            StepperPosition(2);
        }
        YOutPoint = StepperY.currentPosition();
        Display.clearDisplay();

        // Go to IN position
        gotoposition[0] = XInPoint;
        gotoposition[1] = YInPoint;
        Display.clearDisplay();
        Display.setCursor(20, 10);
        Display.println(" Preview  ");
        Display.display();
        StepperX.setMaxSpeed(800);
        StepperX.setSpeed(100);
        StepperY.setMaxSpeed(800);
        StepperY.setSpeed(100);
        StepperControl.moveTo(gotoposition);
        StepperControl.runSpeedToPosition();
    }

    // Display Set Speed
    if (flag == 5) {
        Display.clearDisplay();
        Display.setCursor(20, 10);
        Display.println("Set Speed");
        Display.display();
    }
    // Change Speed
    if (flag == 6) {
        Display.clearDisplay();
        SetSpeed();
    }
    // DisplayStart
    if (flag == 7) {
        Display.clearDisplay();
        Display.setCursor(20, 10);
        Display.println("Start");
        Display.display();
    }
    // Start
    if (flag == 8) {
        Display.clearDisplay();
        Display.setCursor(20, 10);
        Display.println("Running");
        Display.display();

        gotoposition[0] = XOutPoint;
        gotoposition[1] = YOutPoint;
        StepperX.setMaxSpeed(setspeed);
        StepperControl.moveTo(gotoposition);
        StepperControl.runSpeedToPosition();
        flag = flag + 1;
    }
    // Slide Finish
    if (flag == 9) {
        Display.clearDisplay();
        Display.setCursor(45, 14);
        Display.println("Finish");
        Display.display();
    }
    // Return to start
    if (flag == 10) {
        Display.clearDisplay();
        Home();
        flag = 0;
    }
}


//////////////////////////////
// Function Implementations //
//////////////////////////////

void Switch() {
    if (millis() - switch0 > 500) {
        flag = flag + 1;
    }
    switch0 = millis();
}

void Rotary() {
    //delay(75);
    // Read the current state of CLK
    currentStateCLK = digitalRead(ROTARY_ENCODER_CLK_PIN);
    if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
        if (digitalRead(ROTARY_ENCODER_DT_PIN) != currentStateCLK) {
            rotationdirection = true;
            lastStateCLK = currentStateCLK;
        } else {
            rotationdirection = false;
        }
        TurnDetected = true;
    }
    // Remember last CLK state
    lastStateCLK = currentStateCLK;
}

void Home() {
    StepperX.setMaxSpeed(1000);
    StepperX.setSpeed(900);
    StepperY.setMaxSpeed(1000);
    StepperY.setSpeed(900);
    if (digitalRead(LIMIT_SWITCH_PIN) == 1) {
        Display.drawBitmap(0, 0, Homing, 256, 128, 1);
        Display.display();
    }

    while (digitalRead(LIMIT_SWITCH_PIN) == 1) {
        StepperX.setSpeed(-900);
        StepperX.runSpeed();
    }
    delay(20);
    StepperX.setCurrentPosition(0);
    StepperX.moveTo(200);
    while (StepperX.distanceToGo() != 0) {
        StepperX.setSpeed(900);
        StepperX.run();
    }
    StepperX.setCurrentPosition(0);
    Display.clearDisplay();
}

void SetSpeed() {
    Display.clearDisplay();
    while (flag == 6) {
        if (TurnDetected) {
            TurnDetected = false; // do NOT repeat IF loop until new rotation detected
            if (rotationdirection) {
                setspeed = setspeed + 30;
            }
            if (!rotationdirection) {
                setspeed = setspeed - 30;
                if (setspeed < 0) {
                    setspeed = 0;
                }
            }

            Display.clearDisplay();
            Display.setTextSize(2);
            Display.setTextColor(WHITE);
            Display.setCursor(30, 0);
            Display.print("Speed");
            motorspeed = setspeed / 80;
            Display.setCursor(5, 16);
            Display.print(motorspeed);
            Display.print(" mm/s");
            totaldistance = XOutPoint - XInPoint;
            if (totaldistance < 0) {
                totaldistance = totaldistance * (-1);
            } else {
            }
            timeinsec = (totaldistance / setspeed);
            timeinmins = timeinsec / 60;
            Display.setCursor(35, 32);
            Display.print("Time");
            Display.setCursor(8, 48);
            if (timeinmins > 1) {
                Display.print(timeinmins);
                Display.print(" min");
            } else {
                Display.print(timeinsec);
                Display.print(" sec");
            }
            Display.display();
        }
        Display.clearDisplay();
        Display.setTextSize(2);
        Display.setTextColor(WHITE);
        Display.setCursor(30, 0);
        Display.print("Speed");
        motorspeed = setspeed / 80;
        Display.setCursor(5, 16);
        Display.print(motorspeed);
        Display.print(" mm/s");
        totaldistance = XOutPoint - XInPoint;
        if (totaldistance < 0) {
            totaldistance = totaldistance * (-1);
        } else {
        }
        timeinsec = (totaldistance / setspeed);
        timeinmins = timeinsec / 60;
        Display.setCursor(35, 32);
        Display.print("Time");
        Display.setCursor(8, 48);
        if (timeinmins > 1) {
            Display.print(timeinmins);
            Display.print(" min");
        } else {
            Display.print(timeinsec);
            Display.print(" sec");
        }
        Display.display();
    }
}

void StepperPosition(int n) {
    StepperX.setMaxSpeed(1000);
    StepperX.setSpeed(200);
    StepperY.setMaxSpeed(1000);
    StepperY.setSpeed(200);
    if (TurnDetected) {
        TurnDetected = false; // do NOT repeat IF loop until new rotation detected
        if (n == 1) {
            if (!rotationdirection) {
                if (StepperX.currentPosition() - 500 > 0) {
                    StepperX.move(-500);
                    while (StepperX.distanceToGo() != 0) {
                        StepperX.setSpeed(-300);
                        StepperX.run();
                    }
                } else {
                    while (StepperX.currentPosition() != 0) {
                        StepperX.setSpeed(-300);
                        StepperX.run();
                    }
                }
            }

            if (rotationdirection) {
                if (StepperX.currentPosition() + 500 < 61000) {
                    StepperX.move(500);
                    while (StepperX.distanceToGo() != 0) {
                        StepperX.setSpeed(300);
                        StepperX.run();
                    }
                } else {
                    while (StepperX.currentPosition() != 61000) {
                        StepperX.setSpeed(300);
                        StepperX.run();
                    }
                }
            }
        }
        if (n == 2) {
            if (rotationdirection) {
                StepperY.move(-25);
                while (StepperY.distanceToGo() != 0) {
                    StepperY.setSpeed(-100);
                    StepperY.run();
                }
            }
            if (!rotationdirection) {
                StepperY.move(25);
                while (StepperY.distanceToGo() != 0) {
                    StepperY.setSpeed(100);
                    StepperY.run();
                }
            }
        }
    }
}
