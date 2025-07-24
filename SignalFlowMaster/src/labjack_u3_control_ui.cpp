#include "labjack_u3_control_ui.h"

#include <CppToolkit\q_status_bar_sink.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

#include <CppToolkit\qjson_save_and_load.h>

#include "protocol_ui.h"

LabJackU3ControlUI::LabJackU3ControlUI(const std::string& address,
                                       BrowseDeviceUI* parent)
    : kAddress_(address),
      QMainWindow(parent, Qt::Window),
      ui(new Ui::LabJackU3ControlUIClass()),
      ptr_mainwindow(parent),
      controller_(address/*, this*/),
      run_protocol_timer_(this) {
  ui->setupUi(this);
  ::cpptoolkit::AddQStatusBarSink(statusBar());
  setupConnections();
  ui->doubleSpinBox_opUnit->setValue(controller_.get_operation_unit_in_s());
  // Set default store path
  //{
  //  std::string path = controller_.get_store_path();
  //  QFileInfo fileInfo(QString::fromStdString(path));
  //  QString fileName = fileInfo.fileName();
  //  QString dirPath = fileInfo.absolutePath();
  //  fileName.prepend("<timestamp>");
  //  QDir dir(dirPath);
  //  ui->lineEdit_storeRootDir->setText(dir.filePath(fileName));
  //}
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

void LabJackU3ControlUI::ClickCollect() {
  if (!isCollecting_) {
    if (ui->checkBox_resetCounter->isChecked()) {
      ResetCounter();
    }
    StartCollectSignal();
  } else {
    StopCollectSignal();
  }
}

//void LabJackU3ControlUI::on_actionReset_Frame_Counter_clicked() {
//  ResetCounter();
//}

//void LabJackU3ControlUI::on_pushButton_add_clicked() { AddEmptyProtocol(); }

void LabJackU3ControlUI::ClickRunAll() {
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

void LabJackU3ControlUI::MoveProtocolUp(int row) {
  if (MoveBottomProtocolUp(row)) {
    MoveTopProtocolDown(row - 1);
  }
}

void LabJackU3ControlUI::MoveProtocolDown(int row) {
  if (MoveTopProtocolDown(row)) {
    MoveBottomProtocolUp(row + 1);
  }
}

void LabJackU3ControlUI::ShowErrors(QStringList errors) {
  this->activateWindow();
  QMessageBox::critical(this, tr("Error"), errors.join("\n"));
}

void LabJackU3ControlUI::SaveProtocolList() {
  QDir dir(default_preset_path_);
  if (!dir.exists()) {
    dir.mkpath(".");  // Create the directory if it does not exist
  }
  cpptoolkit::BrowseToSaveJsonFile(
      controller_.ProtocolListToQJson(GetAllProtocol()), ".json", this,
      tr("Save Protocol List"), default_preset_path_);
}

void LabJackU3ControlUI::LoadProtocolList() { AddProtocolsFromFile(true); }

void LabJackU3ControlUI::AddProtocolList() { AddProtocolsFromFile(false); }

void LabJackU3ControlUI::UpdateRunProgress() {
  qint64 elapsed = elapsedTimer.elapsed();

  ui->progressBar_run->setValue(qMin(elapsed, run_protocol_time_cost_est_));

  ui->label_run->setText(formatTime(elapsed) + "/" +
                         formatTime(run_protocol_time_cost_est_));
}

void LabJackU3ControlUI::SetOpUnit(double unit) {
  int unit_int = unit * 1000;
  unit = unit_int / 1000.0;
  controller_.set_operation_unit_in_s(unit);
  LOG_INFO("Operation unit set to {} seconds",unit);
}

void LabJackU3ControlUI::setupConnections() {
  connect(ui->pushButton_storePath, &QPushButton::clicked, this,
          &LabJackU3ControlUI::BrowseSaveRootPath);
  connect(ui->pushButton_collect, &QPushButton::clicked, this,
          &LabJackU3ControlUI::ClickCollect);
  connect(ui->actionReset_Frame_Counter, &QAction::triggered, this,
          &LabJackU3ControlUI::ResetCounter);
  connect(ui->pushButton_add, &QPushButton::clicked, this,
          &LabJackU3ControlUI::AddEmptyProtocol);
  connect(ui->pushButton_runAll, &QPushButton::clicked, this,
          &LabJackU3ControlUI::ClickRunAll);
  connect(&controller_,
          &signal_flow_master::LabJackU3Controller::errorCollect,
          this, &LabJackU3ControlUI::ShowErrors);
  connect(ui->actionSave_Protocols, &QAction::triggered, this,
          &LabJackU3ControlUI::SaveProtocolList);
  connect(ui->actionLoad_Protocols, &QAction::triggered, this,
          &LabJackU3ControlUI::LoadProtocolList);
  connect(ui->actionAdd_Protocols, &QAction::triggered, this,
          &LabJackU3ControlUI::AddProtocolList);
  connect(&run_protocol_timer_, &QTimer::timeout, this,
          &LabJackU3ControlUI::UpdateRunProgress);
  connect(ui->doubleSpinBox_opUnit,
          QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &LabJackU3ControlUI::SetOpUnit);
}

QString LabJackU3ControlUI::formatTime(int64_t ms) {
  int seconds = ms / 1000;
  int minutes = seconds / 60;
  return QString("%1:%2")
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds % 60, 2, 10, QChar('0'));
}

void LabJackU3ControlUI::FinishProgressBarRun() { 
  ui->progressBar_run->setMaximum(1);
  ui->progressBar_run->setValue(1);
}

bool LabJackU3ControlUI::MoveBottomProtocolUp(int row) {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout || row <= 0) {
    return false;
  }

  // Find widgets to swap
  QLayoutItem* protocolItem = layout->itemAtPosition(row, 0);
  //QLayoutItem* buttonItem = layout->itemAtPosition(row, 1);
  if (!protocolItem/* || !buttonItem*/) {
    return false;  // or log an error
  }

  QWidget* protocolWidget = protocolItem->widget();
  //QWidget* buttonWidget = buttonItem->widget();

  // Remove widgets from current position
  layout->removeWidget(protocolWidget);
  //layout->removeWidget(buttonWidget);

  // Insert widgets at the new position (row - 1)
  layout->addWidget(protocolWidget, row - 1, 0);
  //layout->addWidget(buttonWidget, row - 1, 1);
  return true;
}

bool LabJackU3ControlUI::MoveTopProtocolDown(int row) {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout || row >= layout->rowCount() - 1) {
    return false;  // or log an error
  }

  // Find widgets to swap
  QLayoutItem* protocolItem = layout->itemAtPosition(row, 0);
  //QLayoutItem* buttonItem = layout->itemAtPosition(row, 1);
  if (!protocolItem /*|| !buttonItem*/) {
    return false;  // or log an error
  }

  QWidget* protocolWidget = protocolItem->widget();
  //QWidget* buttonWidget = buttonItem->widget();

  // Remove widgets from current position
  layout->removeWidget(protocolWidget);
  //layout->removeWidget(buttonWidget);

  // Insert widgets at the new position (row + 1)
  layout->addWidget(protocolWidget, row + 1, 0);
  //layout->addWidget(buttonWidget, row + 1, 1);
  return true;
}

void LabJackU3ControlUI::DeleteAllProtocol() {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    return;  // or log an error
  }

  // Clear all items in the layout
  while (QLayoutItem* item = layout->takeAt(0)) {
    if (item->widget()) {
      delete item->widget();
    }
    delete item;
  }
}

void LabJackU3ControlUI::AddProtocol(const Protocol& protocol) {
  QScrollArea* scrollArea = ui->scrollArea_protocols;
  QWidget* container = scrollArea->widget();
  QGridLayout* layout = qobject_cast<QGridLayout*>(container->layout());
  if (!layout) {
    layout = new QGridLayout();
    container->setLayout(layout);
  }

  ProtocolUI* protocol_ui = new ProtocolUI();
  protocol_ui->PutProtocol(protocol);
  QPushButton* deleteBtn = new QPushButton("Delete");
  QPushButton* runBtn = new QPushButton("Run");
  QPushButton* upBtn = new QPushButton("^");
  QPushButton* downBtn = new QPushButton("v");

  QGridLayout* btnLayout = new QGridLayout();
  btnLayout->addWidget(deleteBtn, 0, 0);
  btnLayout->addWidget(runBtn, 1, 0);
  btnLayout->addWidget(upBtn, 0, 1);
  btnLayout->addWidget(downBtn, 1, 1);

  deleteBtn->setMinimumWidth(50);
  runBtn->setMinimumWidth(50);
  upBtn->setMinimumWidth(20);
  downBtn->setMinimumWidth(20);
  upBtn->setToolTip(tr("Move Protocol Up"));
  downBtn->setToolTip(tr("Move Protocol Down"));
  // btnLayout->setColumnStretch(0, 1);  // Make delete and run buttons stretch

  QWidget* btnContainer = new QWidget();
  btnContainer->setLayout(btnLayout);

  int row = layout->rowCount();

  layout->addWidget(protocol_ui, row, 0);
  layout->addWidget(btnContainer, row, 1);
  layout->setColumnStretch(0, 1);
  // Connect the deleteBtn to DeleteProtocol
  connect(deleteBtn, &QPushButton::clicked, [=]() { DeleteProtocol(row); });

  // Connect the runBtn to Run
  connect(runBtn, &QPushButton::clicked, [=]() { Run(row); });

  connect(upBtn, &QPushButton::clicked, [=]() { MoveProtocolUp(row); });

  connect(downBtn, &QPushButton::clicked, [=]() { MoveProtocolDown(row); });
}

void LabJackU3ControlUI::AddProtocolsFromFile(bool clear_before_add) {
  QString file_path = QFileDialog::getOpenFileName(
      this, tr("Load Protocol List"), default_preset_path_);
  if (!file_path.isEmpty()) {
    QJsonObject json = cpptoolkit::LoadQJsonObject(file_path);
    if (json.isEmpty()) {
      LOG_WARN("Load protocol list result is empty!");
      return;
    }
    auto vec_protocols = controller_.QJsonToProtocolList(json);
    if (clear_before_add) {
      DeleteAllProtocol();
    }
    for (const Protocol& protocol : vec_protocols) {
      AddProtocol(protocol);
    }
  }
}

void LabJackU3ControlUI::closeEvent(QCloseEvent* event) {
  int result = QMessageBox::question(
      this, tr("Close Window"),
      tr("Do you wish to close the browse devices window as well?"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (result == QMessageBox::Yes) {
    emit closeBrowseDeviceUI();
  }
}

void LabJackU3ControlUI::ResetCounter() {
  controller_.ResetAllCounters();
  ui->statusBar->showMessage(tr("Frame counter Reset to 0"));
}

void LabJackU3ControlUI::StartCollectSignal() {
  ui->pushButton_collect->setEnabled(false);
  //controller_.clear_errors();
  QString root_dir = ui->lineEdit_storeRootDir->text();
  QString prefix = ui->lineEdit_prefix->text();
  QString timestamp =
      QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
  QString surfix = ui->lineEdit_surfix->text();
  QString folder_name = prefix + "_" + timestamp + "_" +
                        surfix;
  // use a safe way to contact the path
  QDir dir(root_dir);
  QString folder_path = dir.filePath(folder_name);
  ui->label_timestemp->setText(timestamp);

  QFuture<void> future =
      QtConcurrent::run([=]() { controller_.CollectSignalData(folder_path); });

  // Reset button when finished
  auto watcher = new QFutureWatcher<void>(this);
  connect(watcher, &QFutureWatcher<void>::finished, [=]() {
    isCollecting_ = false;
    ui->pushButton_collect->setText(tr("Start Signal Collecting"));
    ui->label_collect->setText(tr("Not Collecting Signal"));
    ui->label_collect->setStyleSheet(
        "color: rgb(255, 255, 255);\nbackground-color: rgb(170, 0, 0);");
    ui->label_timestemp->setText("<timestamp>");
    //std::vector<std::string> vec_errors = controller_.get_errors_collect_();
    //if (!vec_errors.empty()) {
    //  //this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    //  this->activateWindow();
    //  QMessageBox::critical(this, tr("Error When Collecting Signal Data"),
    //                        QString::fromStdString(fmt::format(
    //                            "{}", fmt::join(vec_errors, "\n"))));
    //  //LOG_WARN("{}", fmt::join(vec_errors, "\n"));
    //}
    watcher->deleteLater();  // Avoid memory leaky
  });
  watcher->setFuture(future);

  isCollecting_ = true;
  ui->pushButton_collect->setText(tr("Stop Signal Collecting"));
  ui->label_collect->setText(tr("Collecting Signal"));
  ui->label_collect->setStyleSheet(
      "color: rgb(255, 255, 255);\nbackground-color: rgb(0, 170, 0);");
  ui->pushButton_collect->setEnabled(true);

  // controller_.CollectSignalDataAsync();
}

void LabJackU3ControlUI::StopCollectSignal() {
  //controller_.InterruptProtocol();
  controller_.StopCollectData();
  //CollectSignalDataEnded();
}

void LabJackU3ControlUI::AddEmptyProtocol() { AddProtocol(Protocol()); }

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
  run_protocol_time_cost_est_ = controller_.EstimateTimeCost(vec_protocol);
  ui->progressBar_run->setValue(0);
  ui->progressBar_run->setMaximum(run_protocol_time_cost_est_ + 1);
  elapsedTimer.start();
  run_protocol_timer_.start(100);

  QFuture<void> future = QtConcurrent::run(
      [=]() { controller_.ExecuteProtocolList(vec_protocol); });

  // Enable buttons when finished
  auto watcher = new QFutureWatcher<void>(this);
  connect(watcher, &QFutureWatcher<void>::finished, [=]() {
    run_protocol_timer_.stop();
    FinishProgressBarRun();
    ui->scrollArea_protocols->setEnabled(true);
    ui->pushButton_runAll->setText(tr("Run All"));
  });
  watcher->setFuture(future);
}

void LabJackU3ControlUI::BrowseSaveRootPath() {
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
  //QString new_saving_path = QFileDialog::getSaveFileName(
  //    this, tr("Browse Saving File Name"), ui->lineEdit_storeRootDir->text(),
  //    tr("HDF5 (*.h5)"));
  QString new_saving_root_dir =
      QFileDialog::getExistingDirectory(
      this, tr("Browse Saving Root Directory"), ui->lineEdit_storeRootDir->text());
  if (!new_saving_root_dir.isEmpty()) {
    //// Add timestamp placeholder
    //QFileInfo fileInfo(new_saving_path);
    //QString fileName = fileInfo.fileName();
    //QString fileBaseName = fileInfo.baseName();
    //QString fileSuffix = fileInfo.completeSuffix();
    //QString dirPath = fileInfo.absolutePath();
    //fileName.prepend("<timestamp>");
    //QDir dir(dirPath);
    //new_saving_path = dir.filePath(fileName);
    //controller_.set_store_path(dirPath.toStdString() + "/",
    //                           fileBaseName.toStdString(),
    //                           "." + fileSuffix.toStdString());
    ui->lineEdit_storeRootDir->setText(new_saving_root_dir);
  }
}