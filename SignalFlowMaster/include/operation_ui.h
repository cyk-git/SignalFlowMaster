#pragma once

#include <QWidget>

#include "labjack_u3_controller.h"
#include "ui_operation_ui.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class OperationUIClass;
};
QT_END_NAMESPACE

class OperationUI : public QWidget {
  Q_OBJECT

 public:
  using Operation = signal_flow_master::LabJackU3Controller::Operation;
  OperationUI(QWidget *parent = nullptr);
  ~OperationUI();

  void PutOperation(const Operation& operation);
  Operation GetOperation();

  void setEnabled(bool enabled);

 private:
  Ui::OperationUIClass *ui;
};
