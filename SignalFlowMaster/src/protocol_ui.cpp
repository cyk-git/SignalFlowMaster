#include "protocol_ui.h"

#include <QGridLayout>
#include <QPushButton>

ProtocolUI::ProtocolUI(QWidget* parent)
    : QWidget(parent), ui(new Ui::ProtocolUIClass()) {
  ui->setupUi(this);
}

ProtocolUI::~ProtocolUI() { delete ui; }

void ProtocolUI::PutProtocol(const Protocol& protocol) {
  ui->checkBox_infinite->setChecked(protocol.infinite_repetition);
  ui->spinBox_repetitions->setValue(protocol.repetitions);
  DeleteAllOperation();
  for (const Operation& op : protocol.operations) {
    AddOperation(op);
    //LOG_TRACE("Adding operation with duration: {} ms", op.duration_in_ms);
    //LOG_TRACE("Adding operation with EIO states: {}", op.eioStates);
  }
}

ProtocolUI::Protocol ProtocolUI::GetProtocol() {
  using Operation = signal_flow_master::LabJackU3Controller::Operation;
  std::vector<Operation> vec_operations;
  int repetitions = ui->spinBox_repetitions->value();
  bool infinite_repetition = ui->checkBox_infinite->isChecked();

  // Get the layout from frame_operations
  QGridLayout* layout =
      qobject_cast<QGridLayout*>(ui->frame_operations->layout());
  if (!layout) {
    return Protocol(repetitions, vec_operations,
                    infinite_repetition);  // or log an error
  }

  // Iterate through the rows in the layout
  for (int row = 0; row < layout->rowCount(); ++row) {
    QLayoutItem* operationItem = layout->itemAtPosition(row, 0);

    // Skip if no item at this position
    if (!operationItem) {
      continue;
    }

    QWidget* widget = operationItem->widget();

    // Skip if no widget at this item (empty row)
    if (!widget) {
      continue;
    }

    // Cast widget to your custom OperationUI class
    OperationUI* operationUI = qobject_cast<OperationUI*>(widget);

    // If successfully casted, get the operation and add to the vec_operations
    if (operationUI) {
      Operation operation = operationUI->GetOperation();
      vec_operations.push_back(operation);
    }
  }
  return Protocol(repetitions, vec_operations, infinite_repetition);
}

void ProtocolUI::on_pushButton_add_clicked() { AddOperation(); }

void ProtocolUI::AddOperation(const Operation& operation) {
  QFrame* frame = ui->frame_operations;
  // Assuming frame's layout is already a QGridLayout
  QGridLayout* layout = qobject_cast<QGridLayout*>(frame->layout());

  // If the frame does not already have a QGridLayout, create one
  if (!layout) {
    layout = new QGridLayout();
    frame->setLayout(layout);
  }

  // Create your custom QWidget (OperationUI) and QPushButton
  OperationUI* operation_ui = new OperationUI();
  operation_ui->PutOperation(operation);
  QPushButton* btn = new QPushButton("Delete");

  // Get the next available row in the layout
  int row = layout->rowCount();

  // Add the custom QWidget and QPushButton to the layout
  layout->addWidget(operation_ui, row, 0);  // Add to row, column 0
  layout->addWidget(btn, row, 1);           // Add to row, column 1

  // Connect the button to DeleteOperation
  // Assuming 'this' is the object where DeleteOperation is defined
  connect(btn, &QPushButton::clicked, [=]() { DeleteOperation(row); });
  connect(this, &ProtocolUI::selectSetOpHighlight, operation_ui,
          &OperationUI::selectSetHighlighted);
}

void ProtocolUI::AddOperation() {
  Operation op;
  AddOperation(op);
}

void ProtocolUI::DeleteAllOperation() {
  QFrame* frame = ui->frame_operations;
  QGridLayout* layout = qobject_cast<QGridLayout*>(frame->layout());
  if (!layout) return;
  // Clear all items in the layout
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (item->widget()) {
      delete item->widget();
    }
    delete item;
  }
}

//std::vector<OperationUI*> ProtocolUI::GetAllOperationUI() {
//  std::vector<OperationUI*> operation_uis;
//  QFrame* frame = ui->frame_operations;
//  QGridLayout* layout = qobject_cast<QGridLayout*>(frame->layout());
//  if (!layout) return operation_uis;
//  // Clear all items in the layout
//  while (QLayoutItem* item = layout->takeAt(0)) {
//    OperationUI* op = qobject_cast<OperationUI*>(item->widget());
//    if (op) {
//      operation_uis.push_back(op);
//    }
//  }
//  return operation_uis;
//}

void ProtocolUI::DeleteOperation(int row) {
  QFrame* frame = ui->frame_operations;
  QGridLayout* layout = qobject_cast<QGridLayout*>(frame->layout());
  if (!layout) return;

  // Remove and delete the widgets in the specified row
  for (int col = 0; col < layout->columnCount(); ++col) {
    QLayoutItem* item = layout->itemAtPosition(row, col);
    if (item) {
      QWidget* widget = item->widget();
      layout->removeWidget(widget);  // remove it from layout
      delete widget;                 // delete the QWidget
    }
  }
}

void ProtocolUI::SelectHighlightOp(QUuid uuid) {
  emit selectSetOpHighlight(uuid, true);
}

void ProtocolUI::SelectDeHighlightOp(QUuid uuid) {
  emit selectSetOpHighlight(uuid, false);
}
