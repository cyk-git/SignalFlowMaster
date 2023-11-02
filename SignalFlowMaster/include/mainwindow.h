#pragma once

#include <QtWidgets/QMainWindow>
#include <vector>

#include "labjack_u3_controller.h"
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindowClass;
};
QT_END_NAMESPACE

class BrowseDeviceUI : public QMainWindow {
  Q_OBJECT

 public:
  BrowseDeviceUI(QWidget *parent = nullptr);
  ~BrowseDeviceUI();

  void FindDevice();
  void OpenDevice(const std::string &address);
  void DeviceClosed(const std::string &address);

 private slots:
  void on_pushButton_findDevices_clicked();
  void on_treeView_devices_clicked(const QModelIndex &index);
  void on_pushButton_openDevice_clicked();

 private:
  Ui::MainWindowClass *ui;
  std::vector<signal_flow_master::DeviceInfo> vec_device_info_;
  std::string current_device_address_;
  static std::vector<std::string> opened_addresses_;
};
