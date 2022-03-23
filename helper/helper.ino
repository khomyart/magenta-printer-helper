#include <U8glib.h>
#include <GyverTimers.h>
#include <Vector.h>

//keyboard analog reader port number
#define B_ANALOG_READER A6
//measuremenet digital reader
#define M_DIGITAL_READER 3

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

//buttons states
#define CLICK    0
#define RELEASE  1

//consts for map function, which translates default analog
//signal (0 - 1023) to needed interval
#define HIGHEST_BUTTON_SIGNAL 17
#define LOWEST_BUTTON_SIGNAL   0
//A4 - SDA, A5 - SCK
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

unsigned long time;
unsigned long displayTime;
unsigned long readerTime;

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
  currentWindowNumber = 0,
  selectedWindowNumber = 0;

//Position reader variables
bool previousReaderValue = 0;
bool currentReaderValue = 0;
volatile double passedHoles = 0; // value of passed holes by detector
int amountOfHolesOnWheel = 10;
bool direction = false; //if direction == true, platform is going up. if false - down 

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

/**
 * @brief clears display
 * 
 */
void clearDisplay() {
  u8g.firstPage();
  do {
  } while ( u8g.nextPage() );
};

//Entities declaration
class MenuWindow {
  public:
    char* title;
    int number;
    U8GLIB_SH1106_128X64* u8g;
    MenuWindow* higherLevelMenu;
    MenuWindow* lowerLevelMenu;
    MenuWindow* prevMenu;
    MenuWindow* nextMenu;

    MenuWindow(
      char* title, 
      int number,
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) {
        this->title = title;
        this->number = number;
        this->u8g = u8g;
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

    /**
     * @brief Draws content on display
     * 
     */
    void draw() {
      this->u8g->setFont(u8g_font_helvR10);
      this->u8g->setPrintPos(128/2 - u8g->getStrWidth(this->title)/2, 39);
      this->u8g->print(this->title);
    }

    MenuWindow* onBack() {
      if (this->higherLevelMenu != nullptr) {
        return this->higherLevelMenu;
      } else {
        return this;
      }
    };
    MenuWindow* onSelect(volatile double &passedHoles, int mode){
      if (mode == CLICK) {
        if (this->lowerLevelMenu != nullptr) {
          return this->lowerLevelMenu;
        } else {
          return this;
        }
      }

      if (mode == RELEASE) {

      }
      
    };
    MenuWindow* onLeft(bool &direction, int mode) {
      if (mode == CLICK) {
        if (this->prevMenu != nullptr) {
          return this->prevMenu;
        } else {
          return this;
        }
      }

      if (mode == RELEASE) {
      
      }
      
    };
    MenuWindow* onRight(bool &direction, int mode) {
      if (mode == CLICK) {
        if (this->nextMenu != nullptr) {
          return this->nextMenu;
        } else {
          return this;
        }
      }

      if (mode == RELEASE) {

      }
      
    };
};

class MainMenu : public MenuWindow {
  public:
    MainMenu(
      char* title,
      int number,
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class EngineControllerMenu : public MenuWindow {
  public:
    EngineControllerMenu(
      char* title,
      int number,
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class TemplatesMenu : public MenuWindow {
  public:
    TemplatesMenu(
      char* title,
      int number,
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class CalibrationMenu : public MenuWindow {
  public:
    CalibrationMenu(
      char* title,
      int number, 
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class ManualModeMenu : public MenuWindow {
  public:
    ManualModeMenu(
      char* title,
      int number, 
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

class SemiAutomaticModeMenu : public MenuWindow {
  public:
    SemiAutomaticModeMenu(
      char* title,
      int number, 
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {}
};

//Menu windows logic declaration
MainMenu* mainMenu = new MainMenu("Main menu", 0, &u8g);

EngineControllerMenu* engineControllerMenu = new EngineControllerMenu("Engine control", 1, &u8g);
ManualModeMenu* manualModeMenu = new ManualModeMenu("Manual control", 11, &u8g);
SemiAutomaticModeMenu* semiAutoModeMenu = new SemiAutomaticModeMenu("Semi-auto control", 12, &u8g);

TemplatesMenu* templatesMenu = new TemplatesMenu("Templates", 2, &u8g);
CalibrationMenu* calibrationMenu = new CalibrationMenu("Calibration", 3, &u8g);

//Current window holder
MenuWindow* currentWindow;
MenuWindow* previousWindow;

void setup() {
  //Measurement ruler init
  pinMode(M_DIGITAL_READER, INPUT);
  previousReaderValue = digitalRead(M_DIGITAL_READER);
  currentReaderValue = digitalRead(M_DIGITAL_READER);
  
  //Menu windows init
  mainMenu->setPullOfWindows(engineControllerMenu,engineControllerMenu,engineControllerMenu,engineControllerMenu);
  
  engineControllerMenu->setPullOfWindows(mainMenu, manualModeMenu, calibrationMenu, templatesMenu);
  manualModeMenu->setPullOfWindows(engineControllerMenu, nullptr, semiAutoModeMenu, semiAutoModeMenu);
  semiAutoModeMenu->setPullOfWindows(engineControllerMenu, nullptr, manualModeMenu, manualModeMenu);

  templatesMenu->setPullOfWindows(mainMenu, nullptr, engineControllerMenu, calibrationMenu);
  calibrationMenu->setPullOfWindows(mainMenu, nullptr, templatesMenu, engineControllerMenu);

  currentWindow = mainMenu;
  previousWindow = mainMenu;

  clearDisplay();
  Serial.begin(9600);
  Timer2.setFrequency(1);
  Timer2.enableISR(CHANNEL_A);
  time = millis();
  displayTime = millis();
  readerTime = millis();
}

ISR(TIMER2_A)
{
  Serial.println(digitalRead(M_DIGITAL_READER));
  //Reader
  if ((millis() - readerTime) > 100) {
    currentReaderValue = digitalRead(M_DIGITAL_READER);

    if (currentReaderValue == 1 && previousReaderValue == 0) {
      if (direction == true) {
        passedHoles -= 1;
      } else {
        passedHoles += 1;
      }

      Serial.println(passedHoles);
    }

    previousReaderValue = digitalRead(M_DIGITAL_READER);
    readerTime = millis();
  }

  if ((millis() - time) > 200) {
    pressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));

    //on click
    //pressedButtonCode
    if (previouslyPressedButtonCode == -1 && pressedButtonCode != -1) {
      buttonPressedAt = millis();

      switch (pressedButtonCode)
      {
      case BUTTON_BACK_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onBack();
        break;
      
      case BUTTON_SELECT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onSelect(passedHoles, CLICK);
        break;

      case BUTTON_LEFT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onLeft(direction, CLICK);
        break;

      case BUTTON_RIGHT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onRight(direction, CLICK);
        break;
      
      default:
        break;
      }
    }

    //on release
    //buttonPressingTime, previouslyPressedButtonCode
    if (previouslyPressedButtonCode != -1 && pressedButtonCode == -1) {
      buttonPressingTime = millis() - buttonPressedAt;

      switch (pressedButtonCode)
      {
      case BUTTON_BACK_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onBack();
        break;
      
      case BUTTON_SELECT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onSelect(passedHoles, RELEASE);
        break;

      case BUTTON_LEFT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onLeft(direction, RELEASE);
        break;

      case BUTTON_RIGHT_C:
        previousWindow = currentWindow;
        currentWindow = currentWindow->onRight(direction, RELEASE);
        break;
      
      default:
        break;
      }
    }

    previouslyPressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));
  }
}

void loop() {
  //Keyboard
  if (isKeyboardDebugEnabled != true) {
    if (previousWindow != currentWindow) {
      previousWindow = currentWindow;
      Serial.println(currentWindow->title);
    }
  } else {
    //put some debug here
    keyboardDebugging(B_ANALOG_READER, false);
  }

  if (millis() - displayTime > 1000/10) {
    u8g.firstPage();
    do {
      currentWindow->draw();
    } while ( u8g.nextPage() );

    displayTime = millis();
  }
}