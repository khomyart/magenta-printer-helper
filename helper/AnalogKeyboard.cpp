#include "AnalogKeyboard.h"

void ScreenSaver::draw(volatile int &passedHoles, double mmPerHole)  {}

ScreenSaver* ScreenSaver::init(volatile bool &isRenderAllowed) {
    isRenderAllowed = false;
    return this;
}

void CalibrationWindow::draw(volatile int &passedHoles, double mmPerHole)  {
    int 
        initNumberLength = 0,
        numberFontSize = 6;

    if ((passedHoles * mmPerHole) < 0) {
        initNumberLength = (8 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    } else {
        initNumberLength = (7 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    }
    
    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(10, 31);
    this->u8g->print("Calibration:");

    this->u8g->setPrintPos(64 - initNumberLength / 2, 50);
    this->u8g->print(passedHoles * mmPerHole);
    this->u8g->print(" mm");
}

CalibrationWindow* CalibrationWindow::onSelect(bool &direction, volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode)  {
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

//Semi - auto engine control
SemiAutomaticModeWindow* SemiAutomaticModeWindow::init(volatile bool &isRenderAllowed) {
    if (*this->passedHoles < 0) {
        *this->targetPassedHoles = 0;
    } else {
        *this->targetPassedHoles = *this->passedHoles;
    }
    return this;
}

void SemiAutomaticModeWindow::draw(volatile int &passedHoles, double mmPerHole)  {   
    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(10, 29);
    this->u8g->print("Current: ");
    this->u8g->print(passedHoles * mmPerHole);
    this->u8g->print(" mm");

    this->u8g->setPrintPos(15, 47);
    this->u8g->print("Target: ");
    this->u8g->print(*this->targetPassedHoles * mmPerHole);
    this->u8g->print(" mm");
}

SemiAutomaticModeWindow* SemiAutomaticModeWindow::onSelect(bool &direction, volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode)  {
    if (mode == CLICK) {
        if (isStepperRunning == true) {
            isStepperRunning = false;
        } else {
            if (*this->targetPassedHoles > *this->passedHoles) {
                direction = UP;
            }

            if (*this->targetPassedHoles < *this->passedHoles) {
                direction = DOWN;
            }

            if (*this->targetPassedHoles == *this->passedHoles) {
                return this;
            }

            isStepperRunning = true;
        }        
    }

    return this;
};

SemiAutomaticModeWindow* SemiAutomaticModeWindow::onLeft(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
    //run stepper on click
    if (mode == CLICK) {
        if (*this->targetPassedHoles - 1 < 0) {
            *this->targetPassedHoles = 0;
        } else {
            *this->targetPassedHoles -= 1;
        }
    }

    //stop stepper on release
    if (mode == RELEASE) {

    }

    return this;
};
SemiAutomaticModeWindow* SemiAutomaticModeWindow::onRight(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
    //run stepper on release
    if (mode == CLICK) {
        *this->targetPassedHoles += 1;
    }

    //stop stepper on release
    if (mode == RELEASE) {

    }

    return this;
};

//Manual engine control
void ManualModeWindow::draw(volatile int &passedHoles, double mmPerHole) {
    int 
        initNumberLength = 0,
        numberFontSize = 6;

    if ((passedHoles * mmPerHole) < 0) {
        initNumberLength = (8 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    } else {
        initNumberLength = (7 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    }
    
    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(10, 31);
    this->u8g->print("Manual mode:");

    this->u8g->setPrintPos(64 - initNumberLength / 2, 50);
    this->u8g->print(passedHoles * mmPerHole);
    this->u8g->print(" mm");
}

ManualModeWindow* ManualModeWindow::onLeft(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
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
ManualModeWindow* ManualModeWindow::onRight(bool &direction, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode) {
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

//Templates
void TemplateWindow::draw(volatile int &passedHoles, double mmPerHole) {
    int 
        initNumberLength = 0,
        numberFontSize = 6;

    if ((passedHoles * mmPerHole) < 0) {
        initNumberLength = (8 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    } else {
        initNumberLength = (7 + abs((int)(passedHoles * mmPerHole) / 10)) * numberFontSize;
    }

    double amountOfHoles = this->targetPosition/mmPerHole;
    int roundedAmountOfHoles = (int)(amountOfHoles + 0.5 - (amountOfHoles < 0));

    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(128 / 2 - (this->u8g->getStrWidth(this->title) + 10 * numberFontSize + 3) / 2, 30);
    this->u8g->print(this->title);
    this->u8g->print(" (");
    this->u8g->print(roundedAmountOfHoles * mmPerHole);
    this->u8g->print(" mm)");

    this->u8g->setPrintPos(64 - initNumberLength / 2, 52);
    this->u8g->print(*this->passedHoles * mmPerHole);
    this->u8g->print(" mm");
}

TemplateWindow* TemplateWindow::onSelect(bool &direction, volatile int &passedHoles, volatile bool &isStepperRunning, volatile bool &isStepperStopped, int mode)  {
    double amountOfHoles = this->targetPosition/this->mmPerHole;
    int roundedAmountOfHoles = (int)(amountOfHoles + 0.5 - (amountOfHoles < 0));

    if (mode == CLICK) {

        if (isStepperRunning == true) {
            isStepperRunning = false;
        } else {
            *this->targetPassedHoles = roundedAmountOfHoles;

            if (*this->targetPassedHoles > *this->passedHoles) {
                direction = UP;
            }

            if (*this->targetPassedHoles < *this->passedHoles) {
                direction = DOWN;
            }

            if (*this->targetPassedHoles == *this->passedHoles) {
                return this;
            }

            isStepperRunning = true;
        }        
    }
    
    return this;
};
