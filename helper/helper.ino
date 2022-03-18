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
      return this->higherLevelMenu;
    };
    MenuWindow* onSelect(){
      return this->lowerLevelMenu;
    };
    MenuWindow* onLeft() {
      return this->prevMenu;
    };
    MenuWindow* onRight() {
      return this->nextMenu;
    };

    int number;
    MenuWindow* higherLevelMenu;
    MenuWindow* lowerLevelMenu;
    MenuWindow* prevMenu;
    MenuWindow* nextMenu;
};

class Main : public MenuWindow {
  public:
    Main(
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

class EngineController : public MenuWindow {
  public:
    EngineController(
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

class Templates : public MenuWindow {
  public:
    Templates(
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

class Calibration : public MenuWindow {
  public:
    Calibration(
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
Main* mainWindow = new Main(0);
EngineController* engineController = new EngineController(1);
Templates* templates = new Templates(2);
Calibration* calibration = new Calibration(3);

//Current window holder
MenuWindow* currentWindow;

void setup() {
  //Menu windows init
  mainWindow->setPullOfWindows(engineController,engineController,engineController,engineController);
  engineController->setPullOfWindows(mainWindow, nullptr, calibration, templates);
  templates->setPullOfWindows(mainWindow, nullptr, engineController, calibration);
  calibration->setPullOfWindows(mainWindow, nullptr, templates, engineController);

  currentWindow = mainWindow;

  Serial.begin(9600);
  Timer2.setFrequency(1);
  Timer2.enableISR(CHANNEL_A);
  time = millis();
}

ISR(TIMER2_A)
{
  if ((millis() - time) > 100) {
    pressedButtonCode = getPressedButtonCode(analogRead(B_ANALOG_READER));

    if (previouslyPressedButtonCode == -1 && pressedButtonCode != -1) {
      Serial.println("Button was pressed");
      Serial.println(pressedButtonCode);
      buttonPressedAt = millis();
      ///////////////////////////////////////
      Serial.println(currentWindow->number);
    }

    if (previouslyPressedButtonCode != -1 && pressedButtonCode == -1) {
      Serial.println("Button was released: ");
      Serial.println(previouslyPressedButtonCode);
      buttonPressingTime = millis() - buttonPressedAt;
      Serial.println("Pressed for: ");
      Serial.println(buttonPressingTime);
      /////////////////////////////////////////
      currentWindow = currentWindow->onRight();
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