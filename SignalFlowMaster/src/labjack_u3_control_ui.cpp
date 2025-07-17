#include "labjack_u3_control_ui.h"

#include <CppToolkit\q_status_bar_sink.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

#include "protocol_ui.h"

LabJackU3ControlUI::LabJackU3ControlUI(const std::string& address,
                                       BrowseDeviceUI* parent)
    : kAddress_(address),
      QMainWindow(parent, Qt::Window),
      ui(new Ui::LabJackU3ControlUIClass()),
      ptr_mainwindow(parent),
      controller_(address/*, this*/) {
  ui->setupUi(this);
  ::cpptoolkit::AddQStatusBarSink(statusBar());
  // Set default store path
  {
    std::string path = controller_.get_store_path();
    QFileInfo fileInfo(QString::fromStdString(path));
    QString fileName = fileInfo.fileName();
    QString dirPath = fileInfo.absolutePath();
    fileName.prepend("<timestamp>");
    QDir dir(dirPath);
    ui->lineEdit_storePath->setText(dir.filePath(fileName));
  }
}

LabJackU3ControlUI::~LabJackU3ControlUI() {
  CloseDevice();
  ::cpptoolkit::RemoveStatusBarSink(statusBar());
  delete ui;
}

void LabJackU3ControlUI::OpenDevice() { controller_.OpenDevice(); }

void LabJackU3ControlUI::CloseDevice() {
  controller_.CloseDevice();
  ptr_mainwindow->DeviceClosed(kAddress_);
}

void LabJackU3ControlUI::on_pushButton_collect_clicked() {
  if (!isCollecting_) {
    StartCollectSignal();
  } else {
    StopCollectSignal();
  }
}

void LabJackU3ControlUI::on_pushButton_reset_clicked() {
  controller_.ResetAllCounters();
  ui->statusBar->showMessage(tr("Frame counter Reset to 0"));
}

void LabJackU3ControlUI::on_pushButton_add_clicked() { AddProtocol(); }

void LabJackU3ControlUI::on_pushButton_runAll_clicked() {
  if (ui->scrollArea_protocols->isEnabled()) {
    RunProtocolListAsync(GetAllProtocol());
  } else {
    controller_.InterruptProtocol();  
  }
}

void LabJackU3ControlUI::DeleteProtocol(int row) {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    return;  // or log an error
  }

  // Find widgets to delete
  QLayoutItem* protocolItem = layout->itemAtPosition(row, 0);
  QLayoutItem* buttonItem = layout->itemAtPosition(row, 1);

  if (protocolItem && buttonItem) {
    QWidget* protocolWidget = protocolItem->widget();
    QWidget* buttonWidget = buttonItem->widget();

    // Remove widgets from layout
    layout->removeWidget(protocolWidget);
    layout->removeWidget(buttonWidget);

    // Delete widgets
    delete protocolWidget;
    delete buttonWidget;

    //Please note that shift remaining rows upward will make the row do 
    //not match the delete button. 
    //So, never do things like those codes below!!!
    //// Optionally, shift remaining rows upward to fill the empty space
    //for (int i = row + 1; i < layout->rowCount(); ++i) {
    //  for (int j = 0; j < layout->columnCount(); ++j) {
    //    QLayoutItem* item = layout->itemAtPosition(i, j);
    //    if (item) {
    //      QWidget* widget = item->widget();
    //      layout->removeWidget(widget);
    //      layout->addWidget(widget, i - 1, j);
    //    }
    //  }
    //}
  }
}

void LabJackU3ControlUI::Run(int row) {
  Protocol protocol = GetProtocol(row);
  RunProtocolListAsync({protocol});
  return;
}

void LabJackU3ControlUI::StartCollectSignal() {
  controller_.CollectSignalDataAsync();

  //using Protocol = signal_flow_master::LabJackU3Controller::Protocol;
  //using Operation = signal_flow_master::LabJackU3Controller::Operation;

  //Operation op1(100, {1, 1, 1, 1, 1, 1, 1, 1});
  //Operation op2(100, {0, 0, 0, 0, 0, 0, 0, 0});
  //Protocol pr(1, {op1, op2}, true);
  //std::vector<Protocol> protocal_list({pr});

  //controller_.ExecuteProtocolListAsync(protocal_list);

  isCollecting_ = true;
  ui->pushButton_collect->setText(tr("Stop Signal Collecting"));
  ui->label_collect->setText(tr("Collecting Signal"));
}

void LabJackU3ControlUI::StopCollectSignal() {
  //controller_.InterruptProtocol();
  controller_.StopCollectData();
  //CollectSignalDataEnded();
  isCollecting_ = false;
  ui->pushButton_collect->setText(tr("Start Signal Collecting"));
  ui->label_collect->setText(tr("Not Collecting Signal"));
}

void LabJackU3ControlUI::AddProtocol() {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    layout = new QGridLayout();
    container->setLayout(layout);
  }

  ProtocolUI* protocol_ui = new ProtocolUI();
  QPushButton* deleteBtn = new QPushButton("Delete");
  QPushButton* runBtn = new QPushButton("Run");

  QVBoxLayout* btnLayout = new QVBoxLayout();
  btnLayout->addWidget(deleteBtn);
  btnLayout->addWidget(runBtn);

  QWidget* btnContainer = new QWidget();
  btnContainer->setLayout(btnLayout);

  int row = layout->rowCount();

  layout->addWidget(protocol_ui, row, 0);
  layout->addWidget(btnContainer, row, 1);

  // Connect the deleteBtn to DeleteProtocol
  connect(deleteBtn, &QPushButton::clicked, [=]() { DeleteProtocol(row); });

  // Connect the runBtn to Run
  connect(runBtn, &QPushButton::clicked, [=]() { Run(row); });
}

LabJackU3ControlUI::Protocol LabJackU3ControlUI::GetProtocol(int row) {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    return Protocol();  // or log an error
  }

  // Find ProtocolUI widget
  QLayoutItem* protocolItem = layout->itemAtPosition(row, 0);
  if (!protocolItem) {
    return Protocol();
  }
  QWidget* widget = protocolItem->widget();
  // Cast widget to your custom ProtocolUI class
  ProtocolUI* protocolUI = qobject_cast<ProtocolUI*>(widget);
  if (!protocolUI) {
    return Protocol();
  }
  
  return protocolUI->GetProtocol();
}

std::vector<LabJackU3ControlUI::Protocol> LabJackU3ControlUI::GetAllProtocol() {
  std::vector<Protocol> vec_protocols;

  // Get the layout from frame_protocols
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    return std::vector<Protocol>();  // or log an error
  }

  // Iterate through the rows in the layout
  for (int row = 0; row < layout->rowCount(); ++row) {
    QLayoutItem* protocolItem = layout->itemAtPosition(row, 0);

    // Skip if no item at this position
    if (!protocolItem) {
      continue;
    }

    QWidget* widget = protocolItem->widget();

    // Skip if no widget at this item (empty row)
    if (!widget) {
      continue;
    }

    // Cast widget to your custom ProtocolUI class
    ProtocolUI* protocolUI = qobject_cast<ProtocolUI*>(widget);

    // If successfully casted, get the protocol and add to the vec_protocols
    if (protocolUI) {
      Protocol protocol = protocolUI->GetProtocol();
      vec_protocols.push_back(protocol);
    }
  }
  return vec_protocols;
}

void LabJackU3ControlUI::RunProtocolListAsync(
    const std::vector<Protocol>& vec_protocol) {
  // Disable all run buttons
  ui->scrollArea_protocols->setEnabled(false);
  ui->pushButton_runAll->setText(tr("Stop Protocol"));
 
  QFuture<void> future = QtConcurrent::run(
      [=]() { controller_.ExecuteProtocolList(vec_protocol); });

  // Enable buttons when finished
  auto watcher = new QFutureWatcher<void>(this);
  connect(watcher, &QFutureWatcher<void>::finished, [=]() {
    ui->scrollArea_protocols->setEnabled(true);
    ui->pushButton_runAll->setText(tr("Run All"));
  });
  watcher->setFuture(future);
}

void LabJackU3ControlUI::on_pushButton_storePath_clicked() {
  if (isCollecting_) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::warning(
        this, tr("Attention"),
        tr("Note: Any changes made to the saving path during "
           "data collection will take effect in the next "
           "collection and will not affect this collection."),
        QMessageBox::Ok | QMessageBox::Cancel);

    // Check the user's decision
    if (reply == QMessageBox::Cancel) {
      return;
    }
  }
  QString new_saving_path = QFileDialog::getSaveFileName(
      this, tr("Browse Saving File Name"), ui->lineEdit_storePath->text(),
      tr("HDF5 (*.h5)"));
  if (!new_saving_path.isEmpty()) {
    // Add timestamp placeholder
    QFileInfo fileInfo(new_saving_path);
    QString fileName = fileInfo.fileName();
    QString fileBaseName = fileInfo.baseName();
    QString fileSuffix = fileInfo.completeSuffix();
    QString dirPath = fileInfo.absolutePath();
    fileName.prepend("<timestamp>");
    QDir dir(dirPath);
    new_saving_path = dir.filePath(fileName);
    controller_.set_store_path(dirPath.toStdString() + "/",
                               fileBaseName.toStdString(),
                               "." + fileSuffix.toStdString());
    ui->lineEdit_storePath->setText(new_saving_path);
  }
}