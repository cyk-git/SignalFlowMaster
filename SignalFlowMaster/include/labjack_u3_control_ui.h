#pragma once

#include <QtWidgets/QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

#include "labjack_u3_controller.h"
//#include "labjack_u3_ctrl_ui_interface.h"
#include "ui_labjack_u3_control_ui.h"
#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class LabJackU3ControlUIClass;
};
QT_END_NAMESPACE

class LabJackU3ControlUI : public QMainWindow
                           /*,signal_flow_master::LabJackU3CtrlUIInterface*/ {
  Q_OBJECT

 public:
 signals:
  void closeBrowseDeviceUI();

 public:
  using Protocol = signal_flow_master::LabJackU3Controller::Protocol;
  LabJackU3ControlUI(const std::string &address, BrowseDeviceUI *parent = nullptr);
  ~LabJackU3ControlUI();

  void OpenDevice();
  void CloseDevice();

  //virtual void CollectSignalDataEnded() {
  //  isCollecting_ = false;
  //  ui->pushButton_collect->setText(tr("Start Signal Collecting"));
  //  ui->label_collect->setText(tr("Not Collecting Signal"));
  //}

  void StartCollectSignal();
  void StopCollectSignal();


  Protocol GetProtocol(int row);
  std::vector<Protocol> GetAllProtocol();
  void RunProtocolListAsync(const std::vector<Protocol>& vec_protocol);
  bool MoveBottomProtocolUp(int row);
  bool MoveTopProtocolDown(int row);
  void DeleteAllProtocol();
  void AddProtocol(const Protocol &protocol);
  void AddProtocolsFromFile(bool clear_before_add);

 protected:
  void closeEvent(QCloseEvent *event) override;
 
private slots:
  void BrowseSaveRootPath();
  void ClickCollect();
  //void on_actionReset_Frame_Counter_clicked();
  //void on_pushButton_add_clicked();
  void ResetCounter();
  void AddEmptyProtocol();
  void ClickRunAll();
  void DeleteProtocol(int row);
  void Run(int row);
  void MoveProtocolUp(int row);
  void MoveProtocolDown(int row);
  void ShowErrors(QStringList errors);
  void SaveProtocolList();
  void LoadProtocolList();
  void AddProtocolList();
  void UpdateRunProgress();
  void SetOpUnit(double unit);

 private:
  BrowseDeviceUI *ptr_mainwindow;
  Ui::LabJackU3ControlUIClass *ui;
  std::string kAddress_;
  signal_flow_master::LabJackU3Controller controller_;
  bool isCollecting_ = false;

  QString default_preset_path_ = "./preset/";

  QTimer run_protocol_timer_;
  int64_t run_protocol_time_cost_est_;
  QElapsedTimer elapsedTimer;
  QString formatTime(int64_t ms);
  void FinishProgressBarRun();

  void setupConnections();
};
