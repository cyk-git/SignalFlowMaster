#include "operation_ui.h"

OperationUI::OperationUI(QWidget *parent)
    : QWidget(parent), ui(new Ui::OperationUIClass()) {
  ui->setupUi(this);
}

OperationUI::~OperationUI() { delete ui; }

void OperationUI::PutOperation(const OperationUI::Operation &operation) {
  ui->doubleSpinBox_duration->setValue(operation.duration_in_ms / 1000.0);
  ui->checkBox_1->setChecked(operation.eioStates[1]);
  ui->checkBox_2->setChecked(operation.eioStates[2]);
  ui->checkBox_3->setChecked(operation.eioStates[3]);
  ui->checkBox_4->setChecked(operation.eioStates[4]);
  ui->checkBox_5->setChecked(operation.eioStates[5]);
  ui->checkBox_6->setChecked(operation.eioStates[6]);
  ui->checkBox_7->setChecked(operation.eioStates[7]);
  ui->checkBox_8->setChecked(operation.eioStates[8]);
}

OperationUI::Operation OperationUI::GetOperation() {
  int duration_in_ms = ui->doubleSpinBox_duration->value() * 1000;
  std::array<bool, 8> eioStates{
      ui->checkBox_1->isChecked(), ui->checkBox_2->isChecked(),
      ui->checkBox_3->isChecked(), ui->checkBox_4->isChecked(),
      ui->checkBox_5->isChecked(), ui->checkBox_6->isChecked(),
      ui->checkBox_7->isChecked(), ui->checkBox_8->isChecked(),
  };
  return Operation(duration_in_ms, eioStates);
}

void OperationUI::setEnabled(bool enabled) {
  ui->doubleSpinBox_duration->setEnabled(enabled);
  ui->checkBox_1->setEnabled(enabled);
  ui->checkBox_2->setEnabled(enabled);
  ui->checkBox_3->setEnabled(enabled);
  ui->checkBox_4->setEnabled(enabled);
  ui->checkBox_5->setEnabled(enabled);
  ui->checkBox_6->setEnabled(enabled);
  ui->checkBox_7->setEnabled(enabled);
  ui->checkBox_8->setEnabled(enabled);
}