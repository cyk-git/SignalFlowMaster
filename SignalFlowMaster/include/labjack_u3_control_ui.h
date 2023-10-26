#pragma once

#include <QtWidgets/QMainWindow>
#include <memory>

#include "labjack_u3_controller.h"
#include "labjack_u3_ctrl_ui_interface.h"
#include "ui_labjack_u3_control_ui.h"
#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LabJackU3ControlUIClass;
};
QT_END_NAMESPACE

class LabJackU3ControlUI : public QMainWindow,
                           signal_flow_master::LabJackU3CtrlUIInterface {
  Q_OBJECT

 public:
  LabJackU3ControlUI(const std::string &address, MainWindow *parent = nullptr);
  ~LabJackU3ControlUI();

  void OpenDevice();
  void CloseDevice();

 private:
  MainWindow *ptr_mainwindow;
  Ui::LabJackU3ControlUIClass *ui;
  std::string kAddress_;
  signal_flow_master::LabJackU3Controller controller_;
  
};
