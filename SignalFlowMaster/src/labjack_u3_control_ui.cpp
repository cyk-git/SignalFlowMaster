#include "labjack_u3_control_ui.h"

LabJackU3ControlUI::LabJackU3ControlUI(const std::string& address,
                                       MainWindow* parent)
    : kAddress_(address),
      QMainWindow(parent),
      ui(new Ui::LabJackU3ControlUIClass()),
      ptr_mainwindow(parent),
      controller_(address) {
  ui->setupUi(this);
}

LabJackU3ControlUI::~LabJackU3ControlUI() {
  CloseDevice();
  delete ui;
}

void LabJackU3ControlUI::OpenDevice() { 
  controller_.OpenDevice(); 
}

void LabJackU3ControlUI::CloseDevice() {
  controller_.CloseDevice();
  ptr_mainwindow->DeviceClosed(kAddress_);
}
