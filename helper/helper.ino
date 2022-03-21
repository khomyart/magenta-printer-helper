#include <U8glib.h>
#include <GyverTimers.h>
#include <Vector.h>

//keyboard analog reader port number
#define B_ANALOG_READER A6

#define BUTTONS_VALUES_WINDOW 20

//buttons analog values
#define BUTTON_BACK     1023
#define BUTTON_SELECT    715
#define BUTTON_LEFT      466
#define BUTTON_RIGHT     238

//buttons codes
#define BUTTON_BACK_C    1
#define BUTTON_SELECT_C  2
#define BUTTON_LEFT_C    3
#define BUTTON_RIGHT_C   4

//consts for map function, which translates default analog
//signal (0 - 1023) to needed interval
#define HIGHEST_BUTTON_SIGNAL 17
#define LOWEST_BUTTON_SIGNAL   0
//A4 - SDA, A5 - SCK
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

unsigned long time;
//If keyboard needs calibration
bool isKeyboardDebugEnabled = false;
//time, when button was pressed
volatile unsigned long buttonPressedAt;
//how long is button pressed
volatile unsigned long buttonPressingTime;
//-1 - means buttons are not pressed at all
//When we have value transition from previouslyPressedButtonCode = -1 to 
//pressedButtonCode = (4,6,etc) we can be sure what a button was pressed.
//When we have value transition from previouslyPressedButtonCode = (4,6,etc) to 
//pressedButtonCode = -1 we can be sure what a button was released
volatile int previouslyPressedButtonCode = -1;
volatile int pressedButtonCode = -1;

volatile int 
  currentWindowNumber  = 0,
  selectedWindowNumber = 0;

volatile int 
  currentPosition = 0;

int keyboardDebugging (int analogButtonsReaderPortNumber, bool isMapingEnabled = false) {
  int analogValue = analogRead(analogButtonsReaderPortNumber);
  
  if (isMapingEnabled == true) {
    Serial.println(map(analogValue, 0, 1023, LOWEST_BUTTON_SIGNAL, HIGHEST_BUTTON_SIGNAL));
  } else {
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
bool isButtonSignalPassed(int buttonsAnalogReaderValue, int buttonDefinedValue, int borderWindowValue) {
  if (buttonsAnalogReaderValue > (buttonDefinedValue - borderWindowValue) &&
      buttonsAnalogReaderValue < (buttonDefinedValue + borderWindowValue)) {
    return true;
  } else {
    return false;
  }
}

/**
 * @brief Get the Pressed Button Code object
 * 
 * @param buttonsAnalogReaderValue analog keyboard input value
 * @return int, predefined button code values. If -1, no button pressed 
 */
int getPressedButtonCode(int buttonsAnalogReaderValue) {
 if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_BACK, BUTTONS_VALUES_WINDOW)) {
   return BUTTON_BACK_C;
 }
 if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_SELECT, BUTTONS_VALUES_WINDOW)) {
   return BUTTON_SELECT_C;
 }
 if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_LEFT, BUTTONS_VALUES_WINDOW)) {
   return BUTTON_LEFT_C;
 }
 if (isButtonSignalPassed(buttonsAnalogReaderValue, BUTTON_RIGHT, BUTTONS_VALUES_WINDOW)) {
   return BUTTON_RIGHT_C;
 }
 return -1;
}

//Entities declaration
class MenuWindow {
  public:
    MenuWindow(
      int number, 
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) {
        this->number = number;
        this->higherLevelMenu = higherLevelMenu;
        this->lowerLevelMenu = lowerLevelMenu;
        this->prevMenu = prevMenu;
        this->nextMenu = nextMenu;
    }

    /**
     * @brief setting up windows to create menu structure
     * 
     * @param higherLevelMenu BACK button
     * @param lowerLevelMenu SELECT button
     * @param prevMenu LEFT button
     * @param nextMenu RIGHT button
     */
    void setPullOfWindows(
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) {
        this->higherLevelMenu = higherLevelMenu;
        this->lowerLevelMenu = lowerLevelMenu;
        this->prevMenu = prevMenu;
        this->nextMenu = nextMenu;
    }

    MenuWindow* onBack() {
      if (this->higherLevelMenu != nullptr) {
        return this->higherLevelMenu;
      } else {
        return this;
      }
    };
    MenuWindow* onSelect(){
      if (this->lowerLevelMenu != nullptr) {
        return this->lowerLevelMenu;
      } else {
        return this;
      }
    };
    MenuWindow* onLeft() {
      if (this->prevMenu != nullptr) {
        return this->prevMenu;
      } else {
        return this;
      }
    };
    MenuWindow* onRight() {
      if (this->nextMenu != nullptr) {
        return this->nextMenu;
      } else {
        return this;
      }
    };

    int number;
    MenuWindow* higherLevelMenu;
    MenuWindow* lowerLevelMenu;
    MenuWindow* prevMenu;
    MenuWindow* nextMenu;
};

class MainMenu : public MenuWindow {
  public:
    MainMenu(
      int number,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class EngineControllerMenu : public MenuWindow {
  public:
    EngineControllerMenu(
      int number,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class TemplatesMenu : public MenuWindow {
  public:
    TemplatesMenu(
      int number,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class CalibrationMenu : public MenuWindow {
  public:
    CalibrationMenu(
      int number, 
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class ManualModeMenu : public MenuWindow {
  public:
    ManualModeMenu(
      int number, 
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class SemiAutomaticModeMenu : public MenuWindow {
  public:
    SemiAutomaticModeMenu(
      int number, 
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      number, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

//Menu windows logic declaration
MainMenu* mainMenu = new MainMenu(0);

EngineControllerMenu* engineControllerMenu = new EngineControllerMenu(1);
ManualModeMenu* manualModeMenu = new ManualModeMenu(11);
SemiAutomaticModeMenu* semiAutoModeMenu = new SemiAutomaticModeMenu(12);

TemplatesMenu* templatesMenu = new TemplatesMenu(2);
CalibrationMenu* calibrationMenu = new CalibrationMenu(3);

//Current window holder
MenuWindow* currentWindow;

void setup() {
  //Menu windows init
  mainMenu->setPullOfWindows(engineControllerMenu,engineControllerMenu,engineControllerMenu,engineControllerMenu);
  
  engineControllerMenu->setPullOfWindows(mainMenu, manualModeMenu, calibrationMenu, templatesMenu);
  manualModeMenu->setPullOfWindows(engineControllerMenu, nullptr, semiAutoModeMenu, semiAutoModeMenu);
  semiAutoModeMenu->setPullOfWindows(engineControllerMenu, nullptr, manualModeMenu, manualModeMenu);

  templatesMenu->setPullOfWindows(mainMenu, nullptr, engineControllerMenu, calibrationMenu);
  calibrationMenu->setPullOfWindows(mainMenu, nullptr, templatesMenu, engineControllerMenu);

  currentWindow = mainMenu;

  Serial.begin(9600);
  Timer2.setFrequency(1);
  Timer2.enableISR(CHANNEL_A);
  time = millis();
}

ISR(TIMER2_A)
{
  if ((millis() - time) > 50) {
    pressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));

    //on click
    //pressedButtonCode
    if (previouslyPressedButtonCode == -1 && pressedButtonCode != -1) {
      buttonPressedAt = millis();

      /*
        Switching between winows
      */
      switch (pressedButtonCode)
      {
      case BUTTON_BACK_C:
        currentWindow = currentWindow->onBack();
        break;
      
      case BUTTON_SELECT_C:
        currentWindow = currentWindow->onSelect();
        break;

      case BUTTON_LEFT_C:
        currentWindow = currentWindow->onLeft();
        break;

      case BUTTON_RIGHT_C:
        currentWindow = currentWindow->onRight();
        break;
      
      default:
        break;
      }
    }

    //on release
    //buttonPressingTime, previouslyPressedButtonCode
    if (previouslyPressedButtonCode != -1 && pressedButtonCode == -1) {
      buttonPressingTime = millis() - buttonPressedAt;

      Serial.println(currentWindow->number);
    }

    previouslyPressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));
  }
}

void loop() {
  if (isKeyboardDebugEnabled != true) {
    
  } else {
    //put some debug here
    keyboardDebugging(B_ANALOG_READER, false);
  }
}