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

#include <CppToolkit\async_consumer.h>
#include <CppToolkit\locks.h>
#include <LabJackUD.h>

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "labjack_u3_ctrl_ui_interface.h"

namespace signal_flow_master {
struct DeviceInfo {
  std::string type;
  std::map<std::string, std::string> infos;
};

class LabJackU3Controller {
 public:  // Define Structs
  struct Operation {
    int duration;                   // 持续时间，以毫秒为单位
    std::array<bool, 8> eioStates;  // EIO的8个数字输出电平状态

    Operation() {
      duration = 0;
      eioStates = {0, 0, 0, 0, 0, 0, 0, 0};
    };
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

  class SignalDataStorer : public cpptoolkit::AsyncConsumer {
   public:
    explicit SignalDataStorer(std::string store_path)
        : kStorePath_(store_path) {}
    ~SignalDataStorer() { Close(); }
    // Disallow copy and move
    SignalDataStorer(const SignalDataStorer&) = delete;
    SignalDataStorer& operator=(const SignalDataStorer&) = delete;
    SignalDataStorer(SignalDataStorer&&) = delete;
    SignalDataStorer& operator=(SignalDataStorer&&) = delete;

    void StoreSignalDataAsync(const SignalData& data);

   protected:
    std::queue<std::unique_ptr<SignalData>> queue_data_buffer_;
    std::unique_ptr<SignalData> loaded_data_ = nullptr;
    const std::string kStorePath_;

    virtual void ConsumerLoop() { LOG_DEBUG("test"); }

    virtual void LoadDataForProcess();
    virtual void ProcessData();
    virtual void ClearDataBuffer();
    virtual bool is_need_wait_for_data() { return queue_data_buffer_.empty(); }
    virtual bool is_data_buffer_empty() { return queue_data_buffer_.empty(); }

   private:
    void GetData(std::unique_ptr<int[]> data_ptr);
  };

 public:
  explicit LabJackU3Controller(const std::string& address)
      : kAddress_(address) {}
  ~LabJackU3Controller() { CloseDevice(); }
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
  void InterruptProtocol() { flag_execute_protocol_ = false; }

  SignalData CollectOneSignalData();
  void CollectSignalData();
  void CollectSignalDataAsync();
  void set_store_data(bool store) { flag_store_data_ = store; }
  void set_display_data(bool display) { flag_display_data_ = display; }
  void StopCollectData() { flag_collect_data_ = false; }

  static std::vector<DeviceInfo> FindAllDevices();

 private:
  const long kDeviceType_ = LJ_dtU3;
  const long kConnectionType_ = LJ_ctUSB;
  const std::string kAddress_;
  // Open Device
  bool flag_open_ = false;
  LJ_HANDLE device_handle_ = -1;
  // Protocol
  bool flag_execute_protocol_ = false;
  std::thread* ptr_th_protocol_ = nullptr;
  std::array<bool, 8> eio_states_ = {0, 0, 0, 0, 0, 0, 0, 0};
  // Collect Signal Data
  bool flag_collect_data_ = false;
  bool flag_store_data_ = false;
  bool flag_display_data_ = false;
  std::thread* ptr_th_data_ = nullptr;
};
}  // namespace signal_flow_master

#endif  // SIGNAL_FLOW_MASTER_LABJACK_U3_CONTROLLER_H_