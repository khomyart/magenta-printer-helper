#include <U8glib.h>
#include <AccelStepper.h>

//buttons states
#define CLICK    0
#define RELEASE  1

//Step motor
#define STEP 4
#define DIR  5
#define EN   6

// digital reader states
#define UP 1
#define DOWN 0

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
    virtual void draw(volatile int &passedHoles, double mmPerHole) {
      this->u8g->setFont(u8g_font_helvR10);
      this->u8g->setPrintPos(128/2 - u8g->getStrWidth(this->title)/2, 39);
      this->u8g->print(this->title);
    }

    virtual MenuWindow* onBack(volatile bool &isStepperStopped) {
      if (this->higherLevelMenu != nullptr) {
        return this->higherLevelMenu;
      } else {
        return this;
      }
    }

    virtual MenuWindow* onSelect(volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode){
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
    virtual MenuWindow* onLeft(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
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
    virtual MenuWindow* onRight(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
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

class CalibrationWindow : public MenuWindow {
  public:
    AccelStepper* stepper;

    CalibrationWindow(
      char* title,
      int number, 
      U8GLIB_SH1106_128X64* u8g,
      AccelStepper* stepper,
      MenuWindow* higherLevelMenu = nullptr,
      MenuWindow* lowerLevelMenu = nullptr,
      MenuWindow* prevMenu = nullptr,
      MenuWindow* nextMenu = nullptr) 
    : 
    MenuWindow(
      title, number, u8g, higherLevelMenu, lowerLevelMenu,
      prevMenu, nextMenu) {
        this->stepper = stepper;
      }

    void CalibrationWindow::draw(volatile int &passedHoles, double mmPerHole);
    
    CalibrationWindow* CalibrationWindow::onSelect(volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode);
    CalibrationWindow* CalibrationWindow::onLeft(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode);
    CalibrationWindow* CalibrationWindow::onRight(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode);
};

class CalibrationMenu : public MenuWindow {
  public:
    CalibrationMenu(
      char* title,
      int number, 
      U8GLIB_SH1106_128X64* u8g,
      MenuWindow* higherLevelMenu = nullptr,
      CalibrationWindow* lowerLevelMenu = nullptr,
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

