#include "AnalogKeyboard.h"

void CalibrationWindow::draw(volatile int &passedHoles, double mmPerHole)  {
    this->u8g->setFont(u8g_font_helvR08);
    this->u8g->setPrintPos(10, 30);
    this->u8g->print(passedHoles * mmPerHole);

    this->u8g->setPrintPos(10, 10);
    this->u8g->print("Calibration:");
}

// CalibrationWindow* CalibrationWindow::onSelect(volatile int &passedHoles, int mode)  {
//     if (mode == CLICK) {
//         passedHoles = 0;
//         this->stepper->setSpeed(0);
//         this->stepper->stop();
//     }

//     return this;
// };

CalibrationWindow* CalibrationWindow::onSelect(int mode)  {
    if (mode == CLICK) {
        this->stepper->setSpeed(0);
        this->stepper->stop();
    }

    return this;
};

CalibrationWindow* CalibrationWindow::onLeft(bool &direction, int mode) {
    if (mode == CLICK) {
        this->stepper->setSpeed(0);
        this->stepper->stop();
    }

    return this;
};
CalibrationWindow* CalibrationWindow::onRight(bool &direction, int mode) {
    if (mode == CLICK) {
        this->stepper->setSpeed(0);
        this->stepper->stop();
    }

    return this;
};