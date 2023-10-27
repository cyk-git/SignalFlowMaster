#pragma once

#include <QWidget>

#include "labjack_u3_controller.h"
#include "operation_ui.h"
#include "ui_protocol_ui.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolUIClass;
};
QT_END_NAMESPACE

class ProtocolUI : public QWidget {
  Q_OBJECT

 public:
  using Protocol = signal_flow_master::LabJackU3Controller::Protocol;
  ProtocolUI(QWidget *parent = nullptr);
  ~ProtocolUI();

  void PutProtocol(const Protocol &protocol);
  Protocol GetProtocol();

  void AddOperation();

 private slots:
  void on_pushButton_add_clicked();
  void DeleteOperation(int row);

 private:
  Ui::ProtocolUIClass *ui;
};
