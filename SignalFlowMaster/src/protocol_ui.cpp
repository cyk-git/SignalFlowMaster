#include "protocol_ui.h"

#include <QGridLayout>
#include <QPushButton>

ProtocolUI::ProtocolUI(QWidget* parent)
    : QWidget(parent), ui(new Ui::ProtocolUIClass()) {
  ui->setupUi(this);
}

ProtocolUI::~ProtocolUI() { delete ui; }

void ProtocolUI::PutProtocol(const Protocol& protocol) {}

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

void ProtocolUI::AddOperation() {
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
  QPushButton* btn = new QPushButton("Delete");

  // Get the next available row in the layout
  int row = layout->rowCount();

  // Add the custom QWidget and QPushButton to the layout
  layout->addWidget(operation_ui, row, 0);  // Add to row, column 0
  layout->addWidget(btn, row, 1);           // Add to row, column 1

  // Connect the button to DeleteOperation
  // Assuming 'this' is the object where DeleteOperation is defined
  connect(btn, &QPushButton::clicked, [=]() { DeleteOperation(row); });
}

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