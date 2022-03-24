#include "AnalogKeyboard.h"
   
void CalibrationWindow::draw(volatile int &passedHoles, double mmPerHole)  {
    this->u8g->setFont(u8g_font_helvR10);
    this->u8g->setPrintPos(90, 39);
    this->u8g->print(passedHoles * mmPerHole);

    this->u8g->setPrintPos(10, 39);
    this->u8g->print("Calibration:");
}

CalibrationWindow* CalibrationWindow::onSelect(volatile int &passedHoles, int mode)  {
    if (mode == CLICK) {
        passedHoles = 0;
    }

    return this;
};