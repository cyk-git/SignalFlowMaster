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
#include <CppToolkit\handle_exception.h>
#include <CppToolkit\locks.h>
#include <CppToolkit\hdf5_toolkit_core.h>
//#include <H5Cpp.h>
#include <LabJackUD.h>

#include <array>
#include <bitset>
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "labjack_u3_ctrl_ui_interface.h"

#define CHECK_LABJACK_API_ERROR(errorCode, operationName, level) \
  CPPTOOLKIT_CHECK_API_ERRORCODE(errorCode, LJE_NOERROR, operationName, level)

namespace signal_flow_master {
struct DeviceInfo {
  std::string type;
  std::map<std::string, std::string> infos;
};

class LabJackU3Controller {
 public:                             // Define Structs
  static const size_t kNumAIn = 4;      // AIO 0-3
  static const size_t kNumDOut = 8;  // EIO 0-7
  static const size_t kNumDIn = 4;      // CIO 0-3
  static const size_t kNumCounter = 2;  // FIO 4-5

  // Using int_bool as an alternative to std::vector<bool> which is a
  // specialized template that does not behave like a standard STL container.
  // The std::vector<bool> is optimized for space and stores boolean values in a
  // compact form, bit by bit, which leads to a non-standard behavior. This can
  // cause unexpected issues with APIs expecting standard STL container
  // semantics. For instance, references to elements are not actual references
  // but proxy objects, and iterators do not necessarily return references upon
  // dereferencing. The int_bool type is used here to ensure a consistent and
  // expected behavior with vector elements being treated as regular integers,
  // maintaining the size of a boolean while providing standard container
  // characteristics.
  using int_bool = int8_t;
  static_assert(sizeof(int_bool) == sizeof(bool),
                "int_bool must be the same size as bool");
  //struct StreamDataPack {
  //  int pack_size;
  //  std::vector<double> vec_ain_data = std::vector<double>(kNumAIn * pack_size);
  //  std::vector<int_bool> vec_dout_data =
  //      std::vector<int_bool>(kNumDOut * pack_size);
  //  std::vector<int_bool> vec_din_data =
  //      std::vector<int_bool>(kNumDIn * pack_size);
  //  std::vector<uint32_t> vec_counter_data =
  //      std::vector<uint32_t>(kNumCounter * pack_size);
  //  explicit StreamDataPack(int _pack_size = 0) : pack_size(_pack_size) {}
  //};
  struct StreamDataPack {
    size_t pack_size;
    xt::xtensor<double, 2> vec_ain_data;
    xt::xtensor<int_bool, 2> vec_dout_data;
    xt::xtensor<int_bool, 2> vec_din_data;
    xt::xtensor<uint32_t, 2> vec_counter_data;
    explicit StreamDataPack(size_t _pack_size = 0)
        : pack_size(_pack_size),
          vec_ain_data({_pack_size, kNumAIn}),
          vec_dout_data({_pack_size, kNumDOut}),
          vec_din_data({_pack_size, kNumDIn}),
          vec_counter_data({_pack_size, kNumCounter}) {}
  };

  struct Operation {
    int duration_in_ms;               // duration time in ms
    std::bitset<kNumDOut> eioStates;  // 8 EIO Digital output states

    Operation(int duration_in_ms = 0, uint64_t _eioStates = 0b00000000)
        : duration_in_ms(duration_in_ms), eioStates(_eioStates) {}
    Operation(int duration_in_ms, std::array<bool, kNumDOut> _eioStates)
        : duration_in_ms(duration_in_ms) {
      for (int i = 0; i < kNumDOut; i++) {
        eioStates.set(i, _eioStates.at(i));
      }
    }
  };

  struct Protocol {
    int repetitions = 0;                // repetition time
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
    
    virtual void Init(double actual_scan_rate) {
      actual_scan_rate_ = actual_scan_rate;
      AsyncConsumer::Init();
    }

    void StoreSignalDataAsync(const StreamDataPack& data);

    //static H5::DataSet create2DDataSet(H5::H5File& file,
    //                                   const std::string& name, int columns,
    //                                   H5::DataType type);
    //static void extendDataSet(H5::DataSet& dataset, int rank, hsize_t newSize);
    //template <typename T>
    //static void writeDataToDataSet(H5::DataSet& dataset, int columns, int rows,
    //                               const H5::DataType& mem_type, const T* data);
    //static void writeMetadataToH5File(H5::H5File& file,
    //                                  double actual_scan_rate);
    static void writeMetadataToH5File(HighFive::File& file,
                                      double actual_scan_rate);

   protected:
    std::queue<std::unique_ptr<StreamDataPack>> queue_data_buffer_;
    std::unique_ptr<StreamDataPack> loaded_data_ = nullptr;
    const std::string kStorePath_;

    // virtual void ConsumerLoop() { LOG_DEBUG("test"); }
    virtual void Start() override;
    virtual void Close() override;
    virtual void LoadDataForProcess() override;
    virtual void ProcessData() override;
    virtual void ClearDataBuffer() override;
    virtual bool is_need_wait_for_data() override {
      return queue_data_buffer_.empty();
    }
    virtual bool is_data_buffer_empty() override {
      return queue_data_buffer_.empty();
    }

   private:
    double actual_scan_rate_ = -1;

    // Store all data into one file
    //H5::H5File file_;
    //H5::DataSet dataset_ain_voltage_;
    //H5::DataSet dataset_dout_states_;
    //H5::DataSet dataset_din_states_;
    //H5::DataSet dataset_counter_;

    void PrepareH5File();
    void SaveDataToH5File();

    // H5::DataSet dataset_time_;
    // H5::DataSet dataset_voltage_;
    // H5::DataSet dataset_states_;
    // H5::DataSet dataset_count_;
    hsize_t dims_[1] = {0};
    // void GetData(std::unique_ptr<int[]> data_ptr);

    // Handel Errors
    std::vector<std::string> vec_errors_;
    
    int64_t batch_num_=0;
  };

 public:
  explicit LabJackU3Controller(const std::string& address,
                               LabJackU3CtrlUIInterface* ptr_ui)
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
  // void ExecuteProtocolAsync(const Protocol& protocol);
  void ExecuteProtocolListAsync(const std::vector<Protocol>& vec_protocol);
  void InterruptProtocol() {
    if (flag_execute_protocol_ == true) {
      flag_execute_protocol_ = false;
    }
    // sleep_waiter.notify_all();
    sleep_waiter.wake_up();
    if (th_protocol_.joinable()) {
      th_protocol_.join();
    }
  }

  void CollectSignalDataAsync();
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

  void SetUpAIns();
  void SetUpDINs();
  void SetUpDOUTs();

  void SetUpCounters();
  void ResetCounter(int channel);
  void ResetAllCounters();

  void SetUpStreamMode();
  void StartGetStreamData();
  void StopGetStreamData();
  StreamDataPack GetStreamDataPack();

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
  // Stream Settings
  bool flag_stream_started_ = false;
  double scan_rate_ = 5000;
  double actual_scan_rate_ = -1;
  int number_of_channels_ = 0;
  double buffer_seconds_ = 5;
  std::vector<int> vec_aio_channel_id = std::vector<int>(kNumAIn, -1);  // 0-3
  int eio_channel_id = -1;                                              // 4
  int cio_channel_id = -1;                                              // 5
  std::vector<int> vec_counter_channel_id =
      std::vector<int>(kNumCounter, -1);  // 6,8

  // Protocol
  bool flag_execute_protocol_ = false;
  std::thread th_protocol_;
  // std::condition_variable sleep_waiter;
  cpptoolkit::SleepWaiter sleep_waiter;
  // std::array<bool, kNumDOut> eio_states_ = {0, 0, 0, 0, 0, 0, 0, 0};

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