#include "AnalogKeyboard.h"

void CalibrationWindow::draw(volatile int &passedHoles, double mmPerHole)  {
    int 
        initNumberLength = 0,
        numberFontSize = 7;

    if ((passedHoles * mmPerHole) < 0) {
        initNumberLength = (8 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    } else {
        initNumberLength = (7 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    }

    this->u8g->setFont(u8g_font_helvR12);
    
    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(10, 26);
    this->u8g->print("Calibration:");

    this->u8g->setPrintPos(64 - initNumberLength / 2, 43);
    this->u8g->print(passedHoles * mmPerHole);
    this->u8g->print(" mm");
}

CalibrationWindow* CalibrationWindow::onSelect(volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode)  {
    if (mode == CLICK) {
        passedHoles = 0;
    }

    return this;
};

CalibrationWindow* CalibrationWindow::onLeft(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
    direction = DOWN;
    //run stepper on click
    if (mode == CLICK) {
        if (isStepperRunning == false) {
            isStepperRunning = true;
        }
    }

    //stop stepper on release
    if (mode == RELEASE) {
        if (isStepperRunning == true) {
            isStepperRunning = false;
            isStepperStopped = true;
        }
    }

    return this;
};
CalibrationWindow* CalibrationWindow::onRight(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
    direction = UP;
    //run stepper on release
    if (mode == CLICK) {
        if (isStepperRunning == false) {
            isStepperRunning = true;
        }
    }

    //stop stepper on release
    if (mode == RELEASE) {
        if (isStepperRunning == true) {
            isStepperRunning = false;
            isStepperStopped = true;
        }
    }

    return this;
};