#include "labjack_u3_controller.h"

#include <CppToolkit\date_time.h>

namespace signal_flow_master {
void LabJackU3Controller::SignalDataStorer::StoreSignalDataAsync(
    const SignalData& data) {
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

void LabJackU3Controller::SignalDataStorer::Start() {
  // Create a new HDF5 file
  file_ = H5::H5File(kStorePath_, H5F_ACC_TRUNC);

  hsize_t maxDims[1] = {H5S_UNLIMITED};
  H5::DataSpace dataspace(1, dims_, maxDims);

  H5::DSetCreatPropList property_1dim;
  hsize_t chunkDims[1] = {1};
  property_1dim.setChunk(1, chunkDims);

  dataset_time_ = file_.createDataSet("timepoint", H5::PredType::NATIVE_INT64,
                                      dataspace, property_1dim);
  dataset_count_ = file_.createDataSet("count", H5::PredType::NATIVE_DOUBLE,
                                       dataspace, property_1dim);

  // For ain_votage and eio_states, we specify the array dimensions explicitly
  hsize_t voltageDims[2] = {1, kNumAIn};
  hsize_t voltageMaxDims[2] = {H5S_UNLIMITED, kNumAIn};
  H5::DataSpace voltageSpace(2, voltageDims, voltageMaxDims);
  H5::DSetCreatPropList property_ain;
  property_ain.setChunk(2, voltageDims);
  dataset_voltage_ = file_.createDataSet(
      "ain_voltage", H5::PredType::NATIVE_DOUBLE, voltageSpace, property_ain);

  hsize_t stateDims[2] = {1, kNumDOut};
  hsize_t stateMaxDims[2] = {H5S_UNLIMITED, kNumDOut};
  H5::DataSpace stateSpace(2, stateDims, stateMaxDims);
  H5::DSetCreatPropList property_dout;
  property_dout.setChunk(2, stateDims);
  dataset_states_ = file_.createDataSet(
      "eio_states", H5::PredType::NATIVE_HBOOL, stateSpace, property_dout);

  cpptoolkit::AsyncConsumer::Start();
}

void LabJackU3Controller::SignalDataStorer::Close() {
  cpptoolkit::AsyncConsumer::Close();
  file_.close();
}

void LabJackU3Controller::SignalDataStorer::LoadDataForProcess() {
  loaded_data_ = std::move(queue_data_buffer_.front());
  queue_data_buffer_.pop();
}

void LabJackU3Controller::SignalDataStorer::ProcessData() {
  // LOG_DEBUG("[Consumer Process]First elem: {}", loaded_data_->ain_votage[0]);
  const auto& data = *loaded_data_;
  // Increase dataset size
  dims_[0]++;
  dataset_time_.extend(dims_);
  dataset_count_.extend(dims_);

  hsize_t extendedDims[2] = {dims_[0], kNumAIn};
  dataset_voltage_.extend(extendedDims);

  hsize_t extendedStateDims[2] = {dims_[0], kNumDOut};
  dataset_states_.extend(extendedStateDims);

  // Write new data
  H5::DataSpace spaceTime = dataset_time_.getSpace();
  hsize_t start[1] = {dims_[0] - 1};
  hsize_t count[1] = {1};
  spaceTime.selectHyperslab(H5S_SELECT_SET, count, start);
  H5::DataSpace memspaceTime(1, count);
  dataset_time_.write(&data.timepoint, H5::PredType::NATIVE_INT64, memspaceTime,
                      spaceTime);

  H5::DataSpace spaceCount = dataset_count_.getSpace();
  spaceCount.selectHyperslab(H5S_SELECT_SET, count, start);
  H5::DataSpace memspaceCount(1, count);
  dataset_count_.write(&data.count, H5::PredType::NATIVE_DOUBLE, memspaceCount,
                       spaceCount);

  hsize_t startVoltage[2] = {dims_[0] - 1, 0};
  hsize_t countVoltage[2] = {1, kNumAIn};
  H5::DataSpace spaceVoltage = dataset_voltage_.getSpace();
  spaceVoltage.selectHyperslab(H5S_SELECT_SET, countVoltage, startVoltage);
  H5::DataSpace memspaceVoltage(2, countVoltage);
  dataset_voltage_.write(data.ain_votage.data(), H5::PredType::NATIVE_DOUBLE,
                         memspaceVoltage, spaceVoltage);

  hsize_t startState[2] = {dims_[0] - 1, 0};
  hsize_t countState[2] = {1, kNumDOut};
  H5::DataSpace spaceState = dataset_states_.getSpace();
  spaceState.selectHyperslab(H5S_SELECT_SET, countState, startState);
  H5::DataSpace memspaceState(2, countState);
  dataset_states_.write(data.eio_states.data(), H5::PredType::NATIVE_HBOOL,
                        memspaceState, spaceState);
}

void LabJackU3Controller::SignalDataStorer::ClearDataBuffer() {
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
  CHECK_LABJACK_API_ERROR(errorCode, "OpenLabJcak " + kAddress_, kDefaultLevel);
  LOG_INFO("Open LabJack-U3 {}", kAddress_);
  // if (errorCode != LJE_NOERROR) {
  //   LOG_ERROR(
  //       "Can't open LabJack U3 {}! OpenLabJack function returned an error:
  //       {}", kAddress_, errorCode);
  //   CPPTOOLKIT_THROW_EXCEPTION(
  //       std::runtime_error("OpenLabJack function returned an error: " +
  //                          std::to_string(errorCode)),
  //       cpptoolkit::ErrorLevel::E_ERROR);
  // }

  // Execute the pin_configuration_reset IOType so that all
  // pin assignments are in the factory default condition.
  // The ePut function is used, which combines the add/go/get.
  errorCode = ePut(device_handle_, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "LJ_ioPIN_CONFIGURATION_RESET ",
                          kDefaultLevel);
  // Set the pin offset to 4.
  AddRequest(device_handle_, LJ_ioPUT_CONFIG, LJ_chTIMER_COUNTER_PIN_OFFSET, 4,
             0, 0);
  CHECK_LABJACK_API_ERROR(errorCode,
                          "LJ_ioPUT_CONFIG->LJ_chTIMER_COUNTER_PIN_OFFSET ",
                          kDefaultLevel);
  // Enable Counter1. It will use FIO4.
  errorCode = AddRequest(device_handle_, LJ_ioPUT_COUNTER_ENABLE, 0, 1, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "LJ_ioPUT_COUNTER_ENABLE ", kDefaultLevel);

  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne for LabJack Init ", kDefaultLevel);
  ResetCounter0();

  flag_open_ = true;

  set_store_data(true);  // TODO
  // CollectSignalDataAsync();
  //
  // StopCollectData();
}

void LabJackU3Controller::CloseDevice() {
  if (!flag_open_) {
    return;
  }
  flag_open_ = false;
  InterruptProtocol();
  StopCollectData();
  device_handle_ = -1;
  LOG_INFO("Close LabJack-U3 {}", kAddress_);
}

void LabJackU3Controller::ExecuteOperation(const Operation& operation) {
  LJ_ERROR errorCode;
  for (int i = 0; i < 8; i++) {
    if (!flag_execute_protocol_) {
      return;
    }
    errorCode = eDO(device_handle_, i + 8, operation.eioStates[i]);
    // errorCode = AddRequest(device_handle_, LJ_ioPUT_DIGITAL_BIT,i + 8,
    // operation.eioStates[i],0,0);
    CHECK_LABJACK_API_ERROR(errorCode, "eDo for ExecuteOperation ",
                            kDefaultLevel);
    LOG_TRACE("Set EIO{} to {}", i, operation.eioStates[i]);
    // eio_states_[i] = operation.eioStates[i];
  }
  std::this_thread::sleep_for(
      std::chrono::milliseconds(operation.duration_in_ms));
}

void LabJackU3Controller::ExecuteProtocol(const Protocol& protocol) {
  for (int r = 0; r < protocol.repetitions || protocol.infinite_repetition;
       r++) {
    for (const auto& op : protocol.operations) {
      if (!flag_execute_protocol_) {
        return;
      }
      ExecuteOperation(op);
    }
  }
}

void LabJackU3Controller::ExecuteProtocolList(
    const std::vector<Protocol>& vec_protocol) {
  flag_execute_protocol_ = true;
  for (const auto& protocol : vec_protocol) {
    if (!flag_execute_protocol_) {
      break;
    }
    ExecuteProtocol(protocol);
  }
  flag_execute_protocol_ = false;
}

// void LabJackU3Controller::ExecuteProtocolAsync(const Protocol& protocol) {
//   if (flag_execute_protocol_) {
//     LOG_WARN("A protocol is already in execution.");
//     return;
//   }
//   flag_execute_protocol_ = true;
//   th_protocol_ =
//       std::thread(&LabJackU3Controller::ExecuteProtocol, this, protocol);
// }

void LabJackU3Controller::ExecuteProtocolListAsync(
    const std::vector<Protocol>& vec_protocol) {
  if (th_protocol_.joinable()) {
    LOG_WARN("A protocol is already in execution.");
    return;
  }
  th_protocol_ = std::thread(&LabJackU3Controller::ExecuteProtocolList, this,
                             vec_protocol);
}

LabJackU3Controller::SignalData LabJackU3Controller::CollectOneSignalData() {
  SignalData data;
  LJ_ERROR errorCode = -1;
  // Add request to get AIN0-3 votage
  for (int i = 0; i < kNumAIn; i++) {
    errorCode = AddRequest(device_handle_, LJ_ioGET_AIN, i, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(errorCode, "Add request to get AIN0-3 votage ",
                            kDefaultLevel);
  }
  // Add request to get counter0
  errorCode = AddRequest(device_handle_, LJ_ioGET_COUNTER, 0, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "Add request to get counter0 ",
                          kDefaultLevel);
  // Add request to get EIO0-8 votage
  for (int i = 0; i < kNumDOut; i++) {
    errorCode =
        AddRequest(device_handle_, LJ_ioGET_DIGITAL_BIT_STATE, i + 8, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(errorCode, "Add request to get EIO0-8 states ",
                            kDefaultLevel);
  }
  // Get data from LabJack
  auto now = std::chrono::high_resolution_clock::now();
  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne to Get data from LabJack ",
                          kDefaultLevel);
  data.timepoint = now;
  // data.eio_states = eio_states_;
  double eio_state_temp;

  // Fill data into SignalData "data"
  for (int i = 0; i < kNumAIn; i++) {
    errorCode = GetResult(device_handle_, LJ_ioGET_AIN, i, &data.ain_votage[i]);
    CHECK_LABJACK_API_ERROR(errorCode, "GetResult to get AIN0-3 votage ",
                            kDefaultLevel);
  }
  errorCode = GetResult(device_handle_, LJ_ioGET_COUNTER, 0, &data.count);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to get counter0 ",
                          kDefaultLevel);
  for (int i = 0; i < kNumDOut; i++) {
    errorCode = GetResult(device_handle_, LJ_ioGET_DIGITAL_BIT_STATE, i + 8,
                          &eio_state_temp);
    CHECK_LABJACK_API_ERROR(errorCode, "GetResult to get EIO0-8 states ",
                            kDefaultLevel);
    data.eio_states[i] = static_cast<bool>(eio_state_temp);
  }
  // LOG_TRACE("Counter:{}", data.count);
  return data;
}

void LabJackU3Controller::CollectSignalData() {
  SignalData current_data;
  SignalDataStorer storer(store_dir_ + cpptoolkit::DateTime().date_time() +
                          "_" + store_name_ + store_format_);
  storer.Init();
  // LOG_INFO(full_path.string());
  flag_collect_data_ = true;
  try {
    while (flag_collect_data_) {
      current_data = CollectOneSignalData();
      LOG_TRACE("AIN0 {}", current_data.ain_votage[0]);
      if (flag_display_data_) {
        // DisplayAsync
      }
      if (flag_store_data_) {
        storer.StoreSignalDataAsync(current_data);
      }
    }
  } catch (...) {
    LOG_ERROR("Unknown exception caught. Signal Collection End.");
  }

  ptr_ui_->CollectSignalDataEnded();
}

void LabJackU3Controller::CollectSignalDataAsync() {
  if (th_data_.joinable()) {
    LOG_WARN("The device is currently collecting signal data.");
    return;
  }
  th_data_ = std::thread(&LabJackU3Controller::CollectSignalData, this);
}

void LabJackU3Controller::ResetCounter0() {
  // Read & reset Counter0. Note that with the U3 reset is just
  // setting a driver flag to reset on the next read, so reset
  // is generally combined with a read in an add/go/get block.
  // The order of the read & reset within the block does not
  // matter ... the read will always happen right before the reset.
  LJ_ERROR errorCode = -1;
  // Add request to reset counter0
  errorCode = AddRequest(device_handle_, LJ_ioPUT_COUNTER_RESET, 0, 1, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "Add request to reset counter0 ",
                          kDefaultLevel);
  // Add request to get counter0
  errorCode = AddRequest(device_handle_, LJ_ioGET_COUNTER, 0, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "Add request to get counter0 for reset ",
                          kDefaultLevel);

  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne to reset counter0 ", kDefaultLevel);
  errorCode = GetResult(device_handle_, LJ_ioGET_COUNTER, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to get counter0 for reset ",
                          kDefaultLevel);
  errorCode = GetResult(device_handle_, LJ_ioPUT_COUNTER_RESET, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to reset counter0 ",
                          kDefaultLevel);
}
}  // namespace signal_flow_master