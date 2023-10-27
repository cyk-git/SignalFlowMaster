#include "mainwindow.h"

#include <CppToolkit\q_status_bar_sink.h>

#include <QStandardItemModel>
#include <iostream>

#include "labjack_u3_control_ui.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass()) {
  ui->setupUi(this);
  cpptoolkit::AddQStatusBarSink(statusBar());
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::FindDevice() {
  ui->progressBar_findDevices->setValue(0);
  vec_device_info_ = signal_flow_master::LabJackU3Controller::FindAllDevices();
  ui->progressBar_findDevices->setValue(90);

  QStandardItemModel* model = new QStandardItemModel();
  QStandardItem* rootItem = model->invisibleRootItem();
  ui->treeView_devices->setModel(model);
  QStandardItem* header1 = new QStandardItem("Device");
  model->setHorizontalHeaderItem(0, header1);

  // ���model
  for (size_t i = 0; i < vec_device_info_.size(); ++i) {
    const signal_flow_master::DeviceInfo& deviceInfo = vec_device_info_[i];

    // ����������Ϊ��һ��
    QStandardItem* typeItem =
        new QStandardItem(QString::fromStdString(deviceInfo.type));
    rootItem->appendRow(typeItem);
    typeItem->setEditable(false);

    // �����±���Ϊ�ڶ���
    QStandardItem* indexItem = new QStandardItem(
        QString::fromStdString(deviceInfo.infos.at("Serial Number")));
    typeItem->appendRow(indexItem);
    indexItem->setData(i);
    indexItem->setEditable(false);

    //// �������Ҫ���infos�������Ϣ��Ϊ������
    // for (const auto& [key, value] : deviceInfo.infos) {
    //   QStandardItem* keyItem = new
    //   QStandardItem(QString::fromStdString(key)); QStandardItem* valueItem =
    //       new QStandardItem(QString::fromStdString(value));
    //   indexItem->appendRow({keyItem, valueItem});
    // }
  }

  ui->progressBar_findDevices->setValue(100);
}

void MainWindow::OpenDevice(const std::string& address) {
  auto it =
      std::find(opened_addresses_.begin(), opened_addresses_.end(), address);
  if (it != opened_addresses_.end()) {
    LOG_WARN("Device {} have already been opened!", address);
    return;
  }
  LabJackU3ControlUI* labjack_ui = new LabJackU3ControlUI(address, this);
  labjack_ui->show();
  labjack_ui->setAttribute(Qt::WA_DeleteOnClose, true);
  labjack_ui->OpenDevice();
  opened_addresses_.push_back(address);
}

void MainWindow::DeviceClosed(const std::string& address) {
  auto it =
      std::find(opened_addresses_.begin(), opened_addresses_.end(), address);

  if (it != opened_addresses_.end()) {
    // Address exists, remove it
    opened_addresses_.erase(it);
  }
}

void MainWindow::on_treeView_devices_clicked(const QModelIndex& index) {
  QStandardItemModel* model =
      qobject_cast<QStandardItemModel*>(ui->treeView_devices->model());
  QStandardItem* item = model->itemFromIndex(index);

  if (item->data().isValid()) {
    int index = item->data().toInt();
    // std::cout << "Open: " << item->data().toInt();

    QStandardItemModel* model = new QStandardItemModel();
    int row = 0;
    for (const auto& pair : vec_device_info_.at(index).infos) {
      QStandardItem* keyItem =
          new QStandardItem(QString::fromStdString(pair.first));
      QStandardItem* valueItem =
          new QStandardItem(QString::fromStdString(pair.second));

      model->setItem(row, 0, keyItem);
      model->setItem(row, 1, valueItem);
      ++row;
    }
    current_device_address_ =
        vec_device_info_.at(index).infos.at("Serial Number");
    ui->tableView_deviceInfo->setEnabled(true);
    ui->tableView_deviceInfo->setModel(model);
    ui->pushButton_openDevice->setEnabled(true);
  } else {
    if (ui->tableView_deviceInfo->model()) {
      qobject_cast<QStandardItemModel*>(ui->tableView_deviceInfo->model())
          ->clear();
    }
    ui->tableView_deviceInfo->setEnabled(false);
    ui->pushButton_openDevice->setEnabled(false);
  }
}

void MainWindow::on_pushButton_openDevice_clicked() {
  OpenDevice(current_device_address_);
}

void MainWindow::on_pushButton_findDevices_clicked() { FindDevice(); }