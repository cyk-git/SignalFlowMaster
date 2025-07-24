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
 signals:
  void selectSetOpHighlight(QUuid uuid, bool highlight);

 public:
  using Protocol = signal_flow_master::LabJackU3Controller::Protocol;
  using Operation = signal_flow_master::LabJackU3Controller::Operation;
  ProtocolUI(QWidget *parent = nullptr);
  ~ProtocolUI();

  void PutProtocol(const Protocol &protocol);
  Protocol GetProtocol();

  void AddOperation(const Operation &operation);
  void AddOperation();
  void DeleteAllOperation();
  // std::vector<OperationUI *> GetAllOperationUI();

 public slots:
  void on_pushButton_add_clicked();
  void DeleteOperation(int row);
  void SelectHighlightOp(QUuid uuid);
  void SelectDeHighlightOp(QUuid uuid);

 private:
  Ui::ProtocolUIClass *ui;
};