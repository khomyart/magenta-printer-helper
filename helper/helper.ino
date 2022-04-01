#include <U8glib.h>
#include <GyverTimers.h>
#include <AccelStepper.h>

#include "AnalogKeyboard.h"

// keyboard analog reader port number
#define B_ANALOG_READER A6

// measuremenet digital reader
#define M_DIGITAL_READER 3

#define BUTTONS_VALUES_WINDOW 35

// buttons analog values
#define BUTTON_BACK 1010
#define BUTTON_SELECT 715
#define BUTTON_LEFT 466
#define BUTTON_RIGHT 238

// buttons codes
#define BUTTON_BACK_C 1
#define BUTTON_SELECT_C 2
#define BUTTON_LEFT_C 3
#define BUTTON_RIGHT_C 4

// consts for map function, which translates default analog
// signal (0 - 1023) to needed interval
#define HIGHEST_BUTTON_SIGNAL 17
#define LOWEST_BUTTON_SIGNAL 0
// A4 - SDA, A5 - SCK
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

//Step motor
#define STEP 4
#define DIR  5

unsigned long time;
unsigned long displayTime;
unsigned long readerTime;

// If keyboard needs calibration
bool isKeyboardDebugEnabled = false;
// time, when button was pressed
volatile unsigned long buttonPressedAt;
// time, when button was pressed
volatile unsigned long buttonReleasedAt;
// how long is button pressed
volatile unsigned long buttonPressingTime;
// -1 - means buttons are not pressed at all
// When we have value transition from previouslyPressedButtonCode = -1 to
// pressedButtonCode = (4,6,etc) we can be sure what a button was pressed.
// When we have value transition from previouslyPressedButtonCode = (4,6,etc) to
// pressedButtonCode = -1 we can be sure what a button was released
volatile int previouslyPressedButtonCode = -1;
volatile int pressedButtonCode = -1;
volatile unsigned long buttonHoldingTriggeredAt;

volatile int
    currentWindowNumber = 0,
    selectedWindowNumber = 0;

//Screen 
int screenFadingTime = 1; //minutes
volatile bool isRenderAllowed = true;
volatile bool showScreenSaver = false;

// Position reader variables
bool previousReaderValue = 0;
bool currentReaderValue = 0;
volatile int passedHoles = 0; // value of passed holes by detector
volatile int targetPassedHoles = 0; // value of passed holes by detector
int amountOfHolesOnWheel = 10;
int degreeToGetOneMilimeter = 250;
double mmPerHole = 0;
bool direction = UP;

//Stepper
volatile bool 
  isStepperRunning = false,
  isStepperStopped = false;
int 
  stepperSpeed = 200,
  stepperAcceleration = 50;

int keyboardDebugging(int analogButtonsReaderPortNumber, bool isMapingEnabled = false)
{
  int analogValue = analogRead(analogButtonsReaderPortNumber);

  if (isMapingEnabled == true)
  {
    Serial.println(map(analogValue, 0, 1023, LOWEST_BUTTON_SIGNAL, HIGHEST_BUTTON_SIGNAL));
  }
  else
  {
    Serial.println(analogValue);
  }
}

/**
 * @brief is analog signal value meets requirements + - borderWindowValue to buttonsAnalogReaderValue
 * Example: incoming analog signal = 85, BUTTON_BACK value = 82, border value = 10,
 * if 85-10 < 82 > 85 + 10, return = true
 *
 * @param buttonsAnalogReaderValue incoming value from analog read input
 * @param buttonDefinedValue predefined button value. Example: #defined BUTTON_BACK = 82
 * @param borderWindowValue acceptable window for incoming signal error
 * @return true
 * @return false
 */
bool isButtonSignalPassed(int buttonsAnalogReaderValue, int buttonDefinedValue, int borderWindowValue)
{
  if (buttonsAnalogReaderValue > (buttonDefinedValue - borderWindowValue) &&
      buttonsAnalogReaderValue < (buttonDefinedValue + borderWindowValue))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Get the Pressed Button Code object
 *
 * @param buttonsAnalogReaderValue analog keyboard input value
 * @return int, predefined button code values. If -1, no button pressed
 */
int getPressedButtonCode(int buttonsAnalogReaderValue)
{
  if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_BACK, BUTTONS_VALUES_WINDOW))
  {
    return BUTTON_BACK_C;
  }
  if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_SELECT, BUTTONS_VALUES_WINDOW))
  {
    return BUTTON_SELECT_C;
  }
  if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_LEFT, BUTTONS_VALUES_WINDOW))
  {
    return BUTTON_LEFT_C;
  }
  if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_RIGHT, BUTTONS_VALUES_WINDOW))
  {
    return BUTTON_RIGHT_C;
  }
  return -1;
}

/**
 * @brief clears display
 *
 */
void clearDisplay()
{
  u8g.firstPage();
  do
  {
  } while (u8g.nextPage());
};

/**
 * @brief getting milimeters per reader's hole
 *
 * @param amountOfHolesOnWheel how much holes in reader's wheel
 * @param degreeToGetOneMilimeter how much degree we need to get one milimeter
 * @return double
 */
double getMMperHole(int amountOfHolesOnWheel, double degreeToGetOneMilimeter)
{
  double mmPerOneDegree = 1 / degreeToGetOneMilimeter;
  double mmPerHole = (360 / amountOfHolesOnWheel) * mmPerOneDegree;
  return mmPerHole;
}

//Stepper
//mode, pul, dir
AccelStepper stepper(1, STEP, DIR);

// Menu windows logic declaration
MainMenu *mainMenu = new MainMenu("Main menu", 0, &u8g);
ScreenSaver *screenSaver = new ScreenSaver("Screen saver", -1, &u8g);

EngineControllerMenu *engineControllerMenu = new EngineControllerMenu("Engine control", 1, &u8g);
ManualModeMenu *manualModeMenu = new ManualModeMenu("Manual control", 11, &u8g);
ManualModeWindow *manualModeWindow = new ManualModeWindow("Manual control window", 111, &u8g);
SemiAutomaticModeMenu *semiAutoModeMenu = new SemiAutomaticModeMenu("Semi-auto control", 12, &u8g);
SemiAutomaticModeWindow *semiAutoModeWindow = new SemiAutomaticModeWindow("Semi-auto control window", 121, &u8g, &passedHoles, &targetPassedHoles);

TemplatesMenu *templatesMenu = new TemplatesMenu("Templates", 2, &u8g);
TShirtTemplate *tShirtTemplate = new TShirtTemplate("T - shirt", 21, 20, &passedHoles, &targetPassedHoles, &u8g);
SweaterTemplate *sweaterTemplate = new SweaterTemplate("Sweater", 22, 30, &passedHoles, &targetPassedHoles, &u8g);
HoodyTemplate *hoodyTemplate = new HoodyTemplate("Hoody", 23, 40, &passedHoles, &targetPassedHoles, &u8g);

CalibrationMenu *calibrationMenu = new CalibrationMenu("Calibration", 3, &u8g);
CalibrationWindow *calibrationWindow = new CalibrationWindow("Calibration window", 31, &u8g);

// Current window holder
MenuWindow *previousWindow;
MenuWindow *currentWindow;

void setup()
{
  mmPerHole = getMMperHole(amountOfHolesOnWheel, degreeToGetOneMilimeter);
  // Measurement ruler init
  pinMode(M_DIGITAL_READER, INPUT);
  previousReaderValue = digitalRead(M_DIGITAL_READER);
  currentReaderValue = digitalRead(M_DIGITAL_READER);

  // Menu windows init
  mainMenu->setPullOfWindows(engineControllerMenu, screenSaver, engineControllerMenu, engineControllerMenu);
  screenSaver->setPullOfWindows(engineControllerMenu, engineControllerMenu, engineControllerMenu, engineControllerMenu);

  engineControllerMenu->setPullOfWindows(mainMenu, manualModeMenu, calibrationMenu, templatesMenu);
  manualModeMenu->setPullOfWindows(engineControllerMenu, manualModeWindow, semiAutoModeMenu, semiAutoModeMenu);
  manualModeWindow->setPullOfWindows(manualModeMenu, nullptr, nullptr, nullptr);
  semiAutoModeMenu->setPullOfWindows(engineControllerMenu, semiAutoModeWindow, manualModeMenu, manualModeMenu);
  semiAutoModeWindow->setPullOfWindows(semiAutoModeMenu, nullptr, nullptr, nullptr);

  templatesMenu->setPullOfWindows(mainMenu, tShirtTemplate, engineControllerMenu, calibrationMenu);
  tShirtTemplate->setPullOfWindows(templatesMenu, nullptr, hoodyTemplate, sweaterTemplate);
  sweaterTemplate->setPullOfWindows(templatesMenu, nullptr, tShirtTemplate, hoodyTemplate);
  hoodyTemplate->setPullOfWindows(templatesMenu, nullptr, sweaterTemplate, tShirtTemplate);

  calibrationMenu->setPullOfWindows(mainMenu, calibrationWindow, templatesMenu, engineControllerMenu);
  calibrationWindow->setPullOfWindows(calibrationMenu, nullptr, nullptr, nullptr);

  currentWindow = mainMenu;
  previousWindow = mainMenu;

  //Stepper init
  // unsigned long stepperTime;
  stepper.setMaxSpeed(1000);
  
  clearDisplay();
  Serial.begin(9600);
  Timer2.setFrequency(1);
  Timer2.enableISR(CHANNEL_A);
  time = millis();
  displayTime = millis();
  readerTime = millis();
  buttonHoldingTriggeredAt = millis();
}

ISR(TIMER2_A)
{
  //Stepper
  if(isStepperStopped == true 
      || (currentWindow->number == 121 && 
          targetPassedHoles == passedHoles) 
      || (currentWindow->number == 111 &&
          passedHoles <= 0 && direction == DOWN)
          //all templates
      || ((currentWindow->number > 20 && currentWindow->number < 30) && 
          targetPassedHoles == passedHoles)
    ) {   
    isStepperRunning = false;
    isStepperStopped = false;
  }

  if (isStepperRunning == true) {   
    switch (direction)
    {
    case UP:
      stepper.setSpeed(-1 * stepperSpeed);
      break;
    
    case DOWN:
      stepper.setSpeed(stepperSpeed);
      break;
    }

    stepper.runSpeed();
  }

  // Reader
  currentReaderValue = digitalRead(M_DIGITAL_READER);
  if (currentReaderValue == 1 && previousReaderValue == 0)
  {
    if (direction == DOWN)
    {
      passedHoles -= 1;
    }
    else
    {
      passedHoles += 1;
    }
  }
  previousReaderValue = digitalRead(M_DIGITAL_READER);

  //Keyboard
  if ((millis() - time) > 400)
  {
    pressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));

    // on click
    // pressedButtonCode
    if (previouslyPressedButtonCode == -1 && pressedButtonCode != -1)
    {
      buttonPressedAt = millis();

      switch (pressedButtonCode)
      {
      case BUTTON_BACK_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onBack(isStepperStopped);
        break;

      case BUTTON_SELECT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onSelect(direction, passedHoles, isStepperRunning, isStepperStopped, CLICK);
        break;

      case BUTTON_LEFT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onLeft(direction, isStepperRunning, isStepperStopped, CLICK);
        break;

      case BUTTON_RIGHT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onRight(direction, isStepperRunning, isStepperStopped, CLICK);
        break;
      
      default:
        break;
      }
    } 

    // on release
    // buttonPressingTime, previouslyPressedButtonCode
    if (previouslyPressedButtonCode != -1 && pressedButtonCode == -1)
    {
      buttonReleasedAt = millis();
      buttonPressingTime = buttonReleasedAt - buttonPressedAt;
      //makes those variables = 0, becouse we need to reset their values for proper
      //tracking of button holding
      // buttonPressedAt = 0;
      // buttonReleasedAt = 0;
      // buttonPressingTime = 0;

      switch (previouslyPressedButtonCode)
      {
      case BUTTON_BACK_C:
        // previousWindow = currentWindow;
        // currentWindow = currentWindow->onBack(isStepperStopped);
        break;

      case BUTTON_SELECT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onSelect(direction, passedHoles, isStepperRunning, isStepperStopped, RELEASE);
        break;

      case BUTTON_LEFT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onLeft(direction, isStepperRunning, isStepperStopped, RELEASE);
        break;

      case BUTTON_RIGHT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onRight(direction, isStepperRunning, isStepperStopped, RELEASE);
        break;

      default:
        break;
      }
    }

    previouslyPressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));
  }
}

void loop()
{  
  // Keyboard
  if (isKeyboardDebugEnabled != true)
  {

  }
  else
  {
    // Put some debug here
    keyboardDebugging(B_ANALOG_READER, false);
  }

  // Show screen saver trigger
  if (millis() - buttonReleasedAt > 10000 && 
      showScreenSaver == false && 
      buttonReleasedAt > buttonPressedAt) {
    showScreenSaver = true;
  }

  //buttonReleasedAt < buttonPressedAt means what button has been pressed
  //millis() - buttonPressedAt > 1000 pressed more than 1000 in this case
  if (millis() - buttonPressedAt > 1000 && buttonReleasedAt <= buttonPressedAt) {
    if (millis() - buttonHoldingTriggeredAt > 50) {
      //Semi auto window button holding
      if (currentWindow->number == 121) {
        if (pressedButtonCode == BUTTON_LEFT_C) {
          if (targetPassedHoles - 1 < 0) {
            targetPassedHoles = 0;
          } else {
            targetPassedHoles -= 1;
          }
        }
        if (pressedButtonCode == BUTTON_RIGHT_C) {
          targetPassedHoles += 1;
        }
      }
      buttonHoldingTriggeredAt = millis();
    }
  }

  // Shows screen saver
  if (showScreenSaver == true && isRenderAllowed == true) {
    screenSaver->setPullOfWindows(currentWindow, currentWindow, currentWindow, currentWindow);
    currentWindow = screenSaver;
    u8g.firstPage();
    do
    {
      currentWindow->draw(passedHoles, mmPerHole);
    } while (u8g.nextPage());
    previouslyPressedButtonCode = -1;
    pressedButtonCode = 0;
  }

  //Track window changing 
  if (previousWindow->number != currentWindow->number) {
    if (currentWindow->number != -1) {
      showScreenSaver = false;
    }
    currentWindow->init(isRenderAllowed);
  }

  //allow render with 10 fps and when current screen is not a screenSaver
  if (isRenderAllowed == true) {
    if ((millis() - displayTime) > 1000 / 10) {
      u8g.firstPage();
      do
      {
        currentWindow->draw(passedHoles, mmPerHole);
      } while (u8g.nextPage());

      displayTime = millis();
    }
  }
}