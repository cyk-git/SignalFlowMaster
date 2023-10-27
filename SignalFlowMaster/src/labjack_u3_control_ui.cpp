#include "labjack_u3_control_ui.h"
#include <QFileDialog>
#include <QMessageBox>

LabJackU3ControlUI::LabJackU3ControlUI(const std::string& address,
                                       MainWindow* parent)
    : kAddress_(address),
      QMainWindow(parent),
      ui(new Ui::LabJackU3ControlUIClass()),
      ptr_mainwindow(parent),
      controller_(address, this) {
  ui->setupUi(this);
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
  delete ui;
}

void LabJackU3ControlUI::OpenDevice() { 
  controller_.OpenDevice(); 
}

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
  controller_.ResetCounter0();
  ui->statusBar->showMessage(tr("Frame counter Reset to 0"));
}

void LabJackU3ControlUI::StartCollectSignal() {
  controller_.CollectSignalDataAsync();
  isCollecting_ = true;
  ui->pushButton_collect->setText(tr("Stop Signal Collecting"));
  ui->label_collect->setText(tr("Collecting Signal"));
}

void LabJackU3ControlUI::StopCollectSignal() { 
  controller_.StopCollectData(); 
  CollectSignalDataEnded();
}

void LabJackU3ControlUI::on_pushButton_storePath_clicked() {
  if (isCollecting_) {
    QMessageBox::StandardButton reply;
    reply =
        QMessageBox::warning(this, tr("Attention"),
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
