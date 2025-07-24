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

  void setHighlighted(bool highlighted) {
    if (highlighted) {
      //ui->label->setStyleSheet("background-color: yellow;");
      ui->widget_container->setStyleSheet(
          "color: rgb(255, 255, 255);\nbackground-color: rgb(0, 170, 0);");
    } else {
      //ui->label->setStyleSheet("");
      ui->widget_container->setStyleSheet("");
    }
  }

  public slots:
  void selectSetHighlighted(QUuid uuid, bool highlighted) { 
    if (uuid_ == uuid) {
      setHighlighted(highlighted);
    }
  }

 private:
  Ui::OperationUIClass *ui;
  QUuid uuid_;
};
