#include "labjack_u3_controller.h"

namespace signal_flow_master {
void LabJackU3Controller::SignalDataStorer::
    StoreSignalDataAsync(const SignalData& data) {
  PreGetData();
  try {
    if (!flag_handling_error_) {
      cpptoolkit::SafeLockUp lock(lock_data_transfer_, 0);
      queue_data_buffer_.push(std::make_unique<SignalData>(data));  // Get Data
      lock.notify_and_unlock();
    }
  } catch (...) {
    auto level = HandleException(boost::current_exception());
    if (level == cpptoolkit::ErrorLevel::E_CRITICAL) {
      boost::rethrow_exception(boost::current_exception());
    }
  }
  PostGetData();
}

void LabJackU3Controller::SignalDataStorer::
    LoadDataForProcess() {
  loaded_data_ = std::move(queue_data_buffer_.front());
  queue_data_buffer_.pop();
}

void LabJackU3Controller::SignalDataStorer::ProcessData() {
  LOG_INFO("[Consumer Process]First elem: {}", loaded_data_->ain_votage[0]);
}

void LabJackU3Controller::SignalDataStorer::
    ClearDataBuffer() {
  queue_data_buffer_ = std::queue<std::unique_ptr<SignalData>>();
}

std::vector<DeviceInfo> LabJackU3Controller::FindAllDevices() {
  const int kListMaxSize = 128;                    // Maximum size for list
  long num_found;                                  // Number of devices found
  std::vector<long> serial_numbers(kListMaxSize);  // Serial numbers of devices
  std::vector<long> ids(kListMaxSize);             // IDs of devices
  std::vector<double> addresses(kListMaxSize);     // Addresses of devices

  // Calling the ListAll function
  LJ_ERROR error = ListAll(LJ_dtU3,                // DeviceType
                           LJ_ctUSB,               // ConnectionType
                           &num_found,             // Number of devices found
                           serial_numbers.data(),  // Serial numbers
                           ids.data(),             // IDs
                           addresses.data()        // Addresses
  );

  // Handle errors based on the returned LJ_ERROR value (optional but good
  // practice)
  if (error != 0) {
    LOG_ERROR("ListAll function returned an error: {}", error);
    CPPTOOLKIT_THROW_EXCEPTION(
        std::runtime_error("ListAll function returned an error: " +
                           std::to_string(error)),
        cpptoolkit::ErrorLevel::E_ERROR);
  }

  std::vector<DeviceInfo> result;
  for (int i = 0; i < num_found; i++) {
    DeviceInfo device_info;
    device_info.type = "U3-USB";
    device_info.infos["Serial Number"] = std::to_string(serial_numbers.at(i));
    device_info.infos["Id"] = std::to_string(ids.at(i));
    device_info.infos["Addresse"] = std::to_string(addresses.at(i));
    result.emplace_back(device_info);
  }
  return result;
}


void LabJackU3Controller::OpenDevice() {
  if (flag_open_) {
    LOG_WARN("LabJack U3 {} have already been opened!", kAddress_);
    return;
  }
  LJ_ERROR errorCode = OpenLabJack(kDeviceType_, kConnectionType_,
                                   kAddress_.c_str(), false, &device_handle_);
  LOG_INFO("Open LabJack-U3 {}", kAddress_);
  if (errorCode != LJE_NOERROR) {
    LOG_ERROR(
        "Can't open LabJack U3 {}! OpenLabJack function returned an error: {}",
        kAddress_, errorCode);
    CPPTOOLKIT_THROW_EXCEPTION(
        std::runtime_error("OpenLabJack function returned an error: " +
                           std::to_string(errorCode)),
        cpptoolkit::ErrorLevel::E_ERROR);
  }
  flag_open_ = true;
}


void LabJackU3Controller::CloseDevice() {
  if (!flag_open_) {
    return;
  }
  flag_open_ = false;
  InterruptProtocol();
  StopCollectData();
  device_handle_ = -1;
  LOG_INFO("Close LabJack-U3 {}",kAddress_);
}

void LabJackU3Controller::ExecuteProtocolListAsync(
    const std::vector<Protocol>& vec_protocol) {}

}  // namespace signal_flow_master


