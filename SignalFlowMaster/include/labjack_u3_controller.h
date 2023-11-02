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
#include <CppToolkit\handle_exception.h>
#include <LabJackUD.h>

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <H5Cpp.h>

#include "labjack_u3_ctrl_ui_interface.h"

#define CHECK_LABJACK_API_ERROR(errorCode, operationName, level) \
  CPPTOOLKIT_CHECK_API_ERRORCODE(errorCode, LJE_NOERROR, operationName, level)

namespace signal_flow_master {
struct DeviceInfo {
  std::string type;
  std::map<std::string, std::string> infos;
};

class LabJackU3Controller {
 public:  // Define Structs
  static const int kNumAIn = 4;
  static const int kNumDOut = 8;
  struct Operation {
    int duration_in_ms;                    // duration time in ms
    std::array<bool, kNumDOut> eioStates;  // 8 EIO Digital output states

    Operation() {
      duration_in_ms = 0;
      eioStates = {0, 0, 0, 0, 0, 0, 0, 0};
    };
    Operation(int duration_in_ms, std::array<bool, kNumDOut> eioStates)
        : duration_in_ms(duration_in_ms), eioStates(eioStates) {}
  };

  struct Protocol {
    int repetitions = 0;                    // repetition time
    std::vector<Operation> operations;  // operations sequence
    bool infinite_repetition = false;
    Protocol() = default;
    Protocol(int repetitions, std::vector<Operation> operations,
             bool infinite_repetition = false)
        : repetitions(repetitions),
          operations(operations),
          infinite_repetition(infinite_repetition) {}
  };

  struct SignalData {
    std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
    std::array<double, kNumAIn> ain_votage;
    std::array<bool, kNumDOut> eio_states;
    double count;
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

    //virtual void ConsumerLoop() { LOG_DEBUG("test"); }
    virtual void Start();
    virtual void Close();
    virtual void LoadDataForProcess();
    virtual void ProcessData();
    virtual void ClearDataBuffer();
    virtual bool is_need_wait_for_data() { return queue_data_buffer_.empty(); }
    virtual bool is_data_buffer_empty() { return queue_data_buffer_.empty(); }

   private:
    H5::H5File file_;
    H5::DataSet dataset_time_;
    H5::DataSet dataset_voltage_;
    H5::DataSet dataset_states_;
    H5::DataSet dataset_count_;
    hsize_t dims_[1] = {0};
   // void GetData(std::unique_ptr<int[]> data_ptr);
  };

 public:
  explicit LabJackU3Controller(const std::string& address, LabJackU3CtrlUIInterface* ptr_ui)
      : kAddress_(address), ptr_ui_(ptr_ui) {}
  ~LabJackU3Controller() { CloseDevice(); }
  // Disallow copy and move
  LabJackU3Controller(const LabJackU3Controller&) = delete;
  LabJackU3Controller& operator=(const LabJackU3Controller&) = delete;
  LabJackU3Controller(LabJackU3Controller&&) = delete;
  LabJackU3Controller& operator=(LabJackU3Controller&&) = delete;

  void OpenDevice();
  void CloseDevice();

  void ExecuteProtocolList(const std::vector<Protocol>& vec_protocol);
  //void ExecuteProtocolAsync(const Protocol& protocol);
  void ExecuteProtocolListAsync(const std::vector<Protocol>& vec_protocol);
  void InterruptProtocol() {
    if (flag_execute_protocol_ == true) {
      flag_execute_protocol_ = false;
    }
    if (th_protocol_.joinable()) {
      th_protocol_.join();
    }
  }

  void CollectSignalDataAsync();
  void ResetCounter0();
  void set_store_data(bool store) { flag_store_data_ = store; }
  void set_display_data(bool display) { flag_display_data_ = display; }
  void StopCollectData() {
    if (flag_collect_data_ == true) {
      flag_collect_data_ = false;
      if (th_data_.joinable()) {
        th_data_.join();
      }
    }
  }
  void set_store_path(std::string store_dir, std::string store_name,
                      std::string store_format) {
    store_dir_ = store_dir;
    store_name_ = store_name;
    store_format_ = store_format;
  }
  std::string get_store_path() {
    return store_dir_ + store_name_ + store_format_;
  }

  static std::vector<DeviceInfo> FindAllDevices();

 private:
  LabJackU3CtrlUIInterface* ptr_ui_;
  const long kDeviceType_ = LJ_dtU3;
  const long kConnectionType_ = LJ_ctUSB;
  const ::cpptoolkit::ErrorLevel kDefaultLevel =
      ::cpptoolkit::ErrorLevel::E_ERROR;
  const std::string kAddress_;
  // Open Device
  bool flag_open_ = false;
  LJ_HANDLE device_handle_ = -1;
  // Protocol
  bool flag_execute_protocol_ = false;
  std::thread th_protocol_;
  //std::array<bool, kNumDOut> eio_states_ = {0, 0, 0, 0, 0, 0, 0, 0};

  void ExecuteOperation(const Operation& operation);
  void ExecuteProtocol(const Protocol& protocol);

  // Collect Signal Data
  bool flag_collect_data_ = false;
  bool flag_store_data_ = false;
  bool flag_display_data_ = false;
  std::thread th_data_;
  std::string store_dir_ = "C:/Experiment/";
  std::string store_name_ = "exp";
  std::string store_format_ = ".h5";

  SignalData CollectOneSignalData();
  void CollectSignalData();
};
}  // namespace signal_flow_master

#endif  // SIGNAL_FLOW_MASTER_LABJACK_U3_CONTROLLER_H_