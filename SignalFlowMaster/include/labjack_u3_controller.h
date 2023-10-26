/*
 * labjack_u3_controller.h
 *
 * Created on 20231026
 *   by Yukun Cheng
 *   cyk_phy@mail.ustc.edu.cn
 *
 */

#ifndef SIGNAL_FLOW_MASTER_LABJACK_U3_CONTROLLER_H_
#define SIGNAL_FLOW_MASTER_LABJACK_U3_CONTROLLER_H_

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <iomanip>
#include <thread>
#include "LabJackUD.h" 

namespace signal_flow_master {

struct DeviceInfo {
  std::string type;
  std::map<std::string, std::string> infos;
};


class LabJackU3Controller {
 public: // Define Structs
  struct Operation {
    int duration;                   // 持续时间，以毫秒为单位
    std::array<bool, 8> eioStates;  // EIO的8个数字输出电平状态

    Operation(int duration, std::array<bool, 8> eioStates)
        : duration(duration), eioStates(eioStates) {}
  };

  struct Protocol {
    int repetitions;                    // 重复次数
    std::vector<Operation> operations;  // 包含的Operation序列
    bool infinite_repetition;

    Protocol(int repetitions, std::vector<Operation> operations,
             bool infinite_repetition = false)
        : repetitions(repetitions),
          operations(repetitions),
          infinite_repetition(infinite_repetition) {}
  };

  struct SignalData {
    std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
    std::array<double, 4> ain_votage;
    std::array<bool, 8> eio_states;
    uint64_t count;
  };

 public:
  explicit LabJackU3Controller(std::string address);
  ~LabJackU3Controller();
  // Disallow copy and move
  LabJackU3Controller(const LabJackU3Controller&) = delete;
  LabJackU3Controller& operator=(const LabJackU3Controller&) = delete;
  LabJackU3Controller(LabJackU3Controller&&) = delete;
  LabJackU3Controller& operator=(LabJackU3Controller&&) = delete;

  void OpenDevice();
  void CloseDevice();

  void ExecuteOperation(const Operation& operation);
  void ExecuteProtocol(const Protocol& protocol);
  void ExecuteProtocolList(const std::vector<Protocol>& vec_protocol);
  void ExecuteProtocolAsync(const Protocol& protocol);
  void ExecuteProtocolListAsync(const std::vector<Protocol>& vec_protocol);
  void InterruptProtocol();

  SignalData CollectOneSignalData();
  void CollectSignalData();
  void CollectSignalDataAsync();

  static std::vector<DeviceInfo> FindAllDevices();

 private:
  const long kDeviceType_=LJ_dtU3;
  const long kConnectionType_ = LJ_ctUSB;
  const std::string kAddress_;

  bool flag_open_ = false;
  LJ_HANDLE device_handle_;

  bool flag_execute_protocol_ = false;
  std::thread* ptr_th_protocol_ = nullptr;
  std::array<bool, 8> eio_states;

};



}  // namespace signal_flow_master

#endif  // SIGNAL_FLOW_MASTER_LABJACK_U3_CONTROLLER_H_