#include "labjack_u3_controller.h"

#include <CppToolkit\date_time.h>
#include <CppToolkit\qt_file_operations.h>

#include <algorithm>
#include <bitset>
#include <memory>

namespace signal_flow_master {
void LabJackU3Controller::SignalDataStorer::StoreSignalDataAsync(
    const StreamDataPack& data) {
  if (vec_errors_.size() > 0) {
    LOG_ERROR("SignalDataStorer has errors, can't store data.");
    std::string error_message;
    for (const auto& error : vec_errors_) {
      error_message += error + "\n";
    }
    throw std::runtime_error("SignalDataStorer has errors, can't store data.\nDetial:\n"+error_message);
  }

  PreGetData();
  try {
    if (!flag_handling_error_) {
      cpptoolkit::SafeLockUp lock(lock_data_transfer_, 0);
      queue_data_buffer_.push(
          std::make_unique<StreamDataPack>(data));  // Get Data
      LOG_TRACE("notify_and_unlock");
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

//H5::DataSet LabJackU3Controller::SignalDataStorer::create2DDataSet(
//    H5::H5File& file, const std::string& name, int columns, H5::DataType type) {
//  hsize_t dims[2] = {1, columns};
//  hsize_t maxDims[2] = {H5S_UNLIMITED, columns};
//  H5::DataSpace dataspace(2, dims, maxDims);
//
//  H5::DSetCreatPropList prop_list;
//  hsize_t chunkDims[2] = {1, columns};
//  prop_list.setChunk(2, chunkDims);
//
//  return file.createDataSet(name, type, dataspace, prop_list);
//}

void LabJackU3Controller::SignalDataStorer::Start() {
  // Create a new HDF5 file
  PrepareH5File();

  cpptoolkit::AsyncConsumer::Start();
}
void LabJackU3Controller::SignalDataStorer::PrepareH5File() {
  // Create a new HDF5 file
  try {
    LOG_INFO("Data store: {}", kStorePath_.toStdString());
    // QString store_root_path = "F:/cyk/debug/labjack";
    cpptoolkit::create_directory_if_needed(kStorePath_);
    //cpptoolkit::SafeHighFiveFile file_safe(kStorePath_, HighFive::File::Create);
    /*writeMetadataToH5File(file_safe.get(), actual_scan_rate_);*/
  } catch (std::exception& e) {
    try {
      LOG_ERROR("Error when PrepareH5File: {}", e.what());
      vec_errors_.push_back("Error when PrepareH5File");
    } catch (...) {
      vec_errors_.push_back("Error when PrepareH5File");
    }
  } catch (...) {
    LOG_ERROR("Unknown Error when PrepareH5File");
    vec_errors_.push_back("Unknown Error when PrepareH5File");
    // try {
    //   file_.close();
    // } catch (...) {
    // }
  }
  // file_.close();
}
  //void LabJackU3Controller::SignalDataStorer::PrepareH5File() {
//  // Create a new HDF5 file
//  try {
//    kStorePath_ = "F:/test.h5";
//    LOG_DEBUG("{}", kStorePath_);
//    //throw(std::exception("Test"));
//    file_ = H5::H5File(kStorePath_, H5F_ACC_CREAT | H5F_ACC_RDWR);
//    writeMetadataToH5File(file_, actual_scan_rate_);
//
//    // Create datasets for StreamDataPack vectors
//    dataset_ain_voltage_ = create2DDataSet(file_, "ain_voltage", kNumAIn,
//                                           H5::PredType::NATIVE_DOUBLE);
//    dataset_dout_states_ = create2DDataSet(file_, "dout_states", kNumDOut,
//                                           H5::PredType::NATIVE_HBOOL);
//    dataset_din_states_ = create2DDataSet(file_, "din_states", kNumDIn,
//                                          H5::PredType::NATIVE_HBOOL);
//    dataset_counter_ = create2DDataSet(file_, "counter", kNumCounter,
//                                       H5::PredType::NATIVE_UINT32);
//  } catch (H5::Exception& e) {
//    try {
//      e.printErrorStack();
//      vec_errors_.push_back("Error when PrepareH5File");
//      //LOG_ERROR("Error when PrepareH5File: {}", e.getDetailMsg());
//      //vec_errors_.push_back(e.getDetailMsg());
//      file_.close();
//    } catch (...) {
//      vec_errors_.push_back("Error when PrepareH5File");
//    }
//  } catch (...) {
//    LOG_ERROR("Unknown Error when PrepareH5File: {}");
//    vec_errors_.push_back("Unknown Error when PrepareH5File");
//    try {
//      file_.close();
//    } catch (...) {
//    }
//  }
//  // file_.close();
//}


void LabJackU3Controller::SignalDataStorer::Close() {
  cpptoolkit::AsyncConsumer::Close();
  //try {
  //  file_.close();
  //} catch (std::exception& e) {
  //  LOG_ERROR("Error when close file: {}", e.what());
  //}
}

void LabJackU3Controller::SignalDataStorer::LoadDataForProcess() {
  loaded_data_ = std::move(queue_data_buffer_.front());
  queue_data_buffer_.pop();
}
//void LabJackU3Controller::SignalDataStorer::extendDataSet(H5::DataSet& dataset,
//                                                          int rank,
//                                                          hsize_t newSize) {
//  std::vector<hsize_t> currentDims(rank);
//  dataset.getSpace().getSimpleExtentDims(
//      currentDims.data());   // Get current dim size
//  currentDims[0] = newSize;  // Change first dim size
//
//  dataset.extend(currentDims.data());  // Extend dataset
//}

//void LabJackU3Controller::SignalDataStorer::writeMetadataToH5File(
//    H5::H5File& file, double actual_scan_rate) {
//  // Get the current time including milliseconds
//  auto now = std::chrono::system_clock::now();
//  auto now_as_time_t = std::chrono::system_clock::to_time_t(now);
//  std::stringstream ss;
//  ss << std::put_time(std::localtime(&now_as_time_t), "%Y-%m-%d %H:%M:%S");
//
//  // add milliseconds to string
//  // auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//  //                  now.time_since_epoch()) %
//  //              1000;
//  //ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
//
//  std::string timeStr = ss.str();
//
//  // Create a dataspace for the attribute (scalar, since it's a single value)
//  H5::DataSpace attr_dataspace = H5::DataSpace(H5S_SCALAR);
//
//  // Write the actual scan rate
//  H5::Attribute scan_rate_attr = file.createAttribute(
//      "actual_scan_rate", H5::PredType::NATIVE_DOUBLE, attr_dataspace);
//  scan_rate_attr.write(H5::PredType::NATIVE_DOUBLE, &actual_scan_rate);
//
//  // Write the current time
//  H5::StrType str_type(H5::PredType::C_S1,
//                       H5T_VARIABLE);  // Variable length string type
//  H5::Attribute time_attr =
//      file.createAttribute("current_time", str_type, attr_dataspace);
//  time_attr.write(str_type, timeStr);
//}
void LabJackU3Controller::SignalDataStorer::writeMetadataToH5File(
    HighFive::File& file, double actual_scan_rate) {
  std::string timeStr = fmt::format(
      "{:%Y-%m-%d %H:%M:%S}", fmt::localtime(std::chrono::system_clock::now()));
  HighFive::Group root = file.getGroup("/");
  root.createAttribute("actual_scan_rate", actual_scan_rate);
  root.createAttribute("current_time", timeStr);
}

//template <typename T>
//void LabJackU3Controller::SignalDataStorer::writeDataToDataSet(
//    H5::DataSet& dataset, int columns, int rows, const H5::DataType& mem_type,
//    const T* data) {
//  // Get the new dimensions of the dataset
//  hsize_t dims[2];
//  dataset.getSpace().getSimpleExtentDims(dims, nullptr);
//  hsize_t offset[2] = {dims[0] - rows, 0};
//  hsize_t count[2] = {rows, columns};
//
//  // Create file and memory dataspace
//  H5::DataSpace fspace = dataset.getSpace();
//  fspace.selectHyperslab(H5S_SELECT_SET, count, offset);
//  H5::DataSpace mspace(2, count);
//
//  // Write the data to the dataset
//  dataset.write(data, mem_type, mspace, fspace);
//}

void LabJackU3Controller::SignalDataStorer::ProcessData() {
  SaveDataToH5File();
  return;
}

void LabJackU3Controller::SignalDataStorer::SaveDataToH5File() {
  // LOG_DEBUG("[Consumer Process]First elem: {}",loaded_data_->ain_votage[0]);
  const auto& data = *loaded_data_;
  try {
    QString filename =
        QString::fromStdString(fmt::format("{:08d}.h5", batch_num_++));
    QString temp_path =
        cpptoolkit::generate_temp_file_path(kStorePath_, ".labjacktemp");
    QString file_path = cpptoolkit::generate_file_path(kStorePath_, filename);

    {
      cpptoolkit::SafeHighFiveFile file_safe(temp_path.toStdString(),
                                             HighFive::File::Create);
      HighFive::File& file = file_safe.get();
      writeMetadataToH5File(file, actual_scan_rate_);
      xt::dump(file, "ain_voltage", data.vec_ain_data);
      xt::dump(file, "dout_states", data.vec_dout_data);
      xt::dump(file, "din_states", data.vec_din_data);
      xt::dump(file, "counter", data.vec_counter_data);
    }

    cpptoolkit::rename_temp_file(temp_path, file_path);
  } catch (std::runtime_error& e) {
    LOG_ERROR("Error when SaveDataToH5File: {}", e.what());
    vec_errors_.push_back("Error when SaveDataToH5File");
  }
}

//void LabJackU3Controller::SignalDataStorer::SaveDataToH5File() {
//  // LOG_DEBUG("[Consumer Process]First elem: {}", loaded_data_->ain_votage[0]);
//  const auto& data = *loaded_data_;
//  // Increase dataset size for each data type
//  dims_[0] += data.pack_size;
//  //file_.reOpen();
//  extendDataSet(dataset_ain_voltage_, kNumAIn, dims_[0]);
//  extendDataSet(dataset_dout_states_, kNumDOut, dims_[0]);
//  extendDataSet(dataset_din_states_, kNumDIn, dims_[0]);
//  extendDataSet(dataset_counter_, kNumCounter, dims_[0]);
//  LOG_DEBUG("data.pack_size:{}", data.pack_size);
//  LOG_DEBUG("dims_[0]:{}", dims_[0]);
//  // Write new data for each data type
//  writeDataToDataSet(dataset_ain_voltage_, kNumAIn, data.pack_size,
//                     H5::PredType::NATIVE_DOUBLE, data.vec_ain_data.data());
//  writeDataToDataSet(dataset_dout_states_, kNumDOut, data.pack_size,
//                     H5::PredType::NATIVE_HBOOL, data.vec_dout_data.data());
//  writeDataToDataSet(dataset_din_states_, kNumDIn, data.pack_size,
//                     H5::PredType::NATIVE_HBOOL, data.vec_din_data.data());
//  writeDataToDataSet(dataset_counter_, kNumCounter, data.pack_size,
//                     H5::PredType::NATIVE_UINT32, data.vec_counter_data.data());
//  //file_.close();
//}


void LabJackU3Controller::SignalDataStorer::ClearDataBuffer() {
  queue_data_buffer_ = std::queue<std::unique_ptr<StreamDataPack>>();
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
  errorCode = ePut(device_handle_, LJ_ioPIN_CONFIGURATION_RESET, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "LJ_ioPIN_CONFIGURATION_RESET ",
                          kDefaultLevel);

  // In case the program exits abnormally, the Stream might not have been properly closed.  
  // Here, we attempt to close the stream to reset its state.  
  errorCode = ePut(device_handle_, LJ_ioSTOP_STREAM, 0, 0, 0);  
  if (errorCode != 5 && errorCode != 0) { // errorCode == 5 indicates the stream was already closed.
   CHECK_LABJACK_API_ERROR(errorCode, "eGet for LJ_ioSTOP_STREAM ",  
                         kDefaultLevel);  
  }

  SetUpAIns();
  SetUpDINs();
  SetUpDOUTs();
  SetUpCounters();
  SetUpStreamMode();

  flag_open_ = true;

  set_store_data(true);  // TODO
  // flag_execute_protocol_ = true;
  // ExecuteOperation(Operation(0, {0, 1, 0, 1, 1, 0, 1, 0}));
  // flag_execute_protocol_ = false;
  // StartGetStreamData();
  // GetStreamDataPack();
  // StopGetStreamData();
  //  CollectSignalDataAsync();
  //
  //  StopCollectData();
}

void LabJackU3Controller::CloseDevice() {
  if (!flag_open_) {
    return;
  }
  flag_open_ = false;
  InterruptProtocol();
  StopCollectData();
  StopGetStreamData();
  device_handle_ = -1;
  LOG_INFO("Close LabJack-U3 {}", kAddress_);
}

void LabJackU3Controller::ExecuteOperation(const Operation& operation) {
  LJ_ERROR errorCode;

  //// Method one: executive eDO in turn
  // for (int i = 0; i < kNumDOut; i++) {
  //   if (!flag_execute_protocol_) {
  //     return;
  //   }
  //   errorCode = eDO(device_handle_, i + 8, operation.eioStates[i]);
  //   // errorCode = AddRequest(device_handle_, LJ_ioPUT_DIGITAL_BIT,i + 8,
  //   // operation.eioStates[i],0,0);
  //   CHECK_LABJACK_API_ERROR(errorCode, "eDo for ExecuteOperation ",
  //                           kDefaultLevel);
  //   LOG_TRACE("Set EIO{} to {}", i, operation.eioStates[i]);
  //   // eio_states_[i] = operation.eioStates[i];
  // }

  //// Method 2: use PORT method to write all ports
  if (!flag_execute_protocol_) {
    return;
  }
  errorCode = AddRequest(device_handle_, LJ_ioPUT_DIGITAL_PORT, 8,
                         operation.eioStates.to_ulong(), kNumDOut, 0);
  CHECK_LABJACK_API_ERROR(
      errorCode,
      "Add request to write EIO0-8 states " + operation.eioStates.to_string(),
      kDefaultLevel);
  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne to write EIO0-8 states ",
                          kDefaultLevel);
  errorCode = GetResult(device_handle_, LJ_ioPUT_DIGITAL_PORT, 8, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to write EIO0-8 states ",
                          kDefaultLevel);
  LOG_TRACE("Set EIO0-7 to {}", operation.eioStates.to_string());

  // std::this_thread::sleep_for(
  //     std::chrono::milliseconds(operation.duration_in_ms));
  // std::mutex temp_mutex;
  // std::unique_lock<std::mutex> temp_lk(temp_mutex);
  // sleep_waiter.wait_for(temp_lk,
  //                       std::chrono::milliseconds(operation.duration_in_ms),
  //                       [this] { return !flag_execute_protocol_; });
  sleep_waiter.sleep_for(operation.duration_in_ms);
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
  // SignalData current_data;
  StreamDataPack current_data_pack;
  SignalDataStorer storer(
      QString::fromStdString(store_dir_ + cpptoolkit::DateTime().date_time() +
                             "_" + store_name_ + store_format_));
  // LOG_INFO(full_path.string());
  flag_collect_data_ = true;
  // Stream Mode
  StartGetStreamData();
  storer.Init(actual_scan_rate_);
  try {
    while (flag_collect_data_) {
      // No Stream Mode
      // current_data = CollectOneSignalData();
      // LOG_TRACE("AIN0 {}", current_data.ain_votage[0]);
      // if (flag_display_data_) {
      //  // DisplayAsync
      //}
      // if (flag_store_data_) {
      //  storer.StoreSignalDataAsync(current_data);
      //}

      // Stream Mode
      current_data_pack = GetStreamDataPack();
      if (flag_display_data_) {
        // DisplayAsync
      }
      if (flag_store_data_) {
        storer.StoreSignalDataAsync(current_data_pack);
      }
    }
  } catch (std::exception& e) {
    LOG_ERROR("Error occured when collecting signal.\n> {}", e.what());
  }
  catch (...) {
    LOG_ERROR("Unknown exception caught. Signal Collection End.");
  }
  StopGetStreamData();
  //ptr_ui_->CollectSignalDataEnded();
}

void LabJackU3Controller::SetUpAIns() {
  double dblVoltage;
  for (int i = 0; i < kNumAIn; i++) {
    auto errorCode = eAIN(device_handle_, i, 31, &dblVoltage, 0, 0, 0, 0, 0, 0);
    // If the negative channel is set to 31/199, the U3 does a single-ended
    // conversion and returns a unipolar value.
    CHECK_LABJACK_API_ERROR(errorCode,
                            "eAIN to set up AIN-" + std::to_string(i) + " ",
                            kDefaultLevel);
  }
}

void LabJackU3Controller::SetUpDINs() {
  long lngState;
  for (int i = 0; i < kNumDIn; i++) {
    auto errorCode = eDI(device_handle_, i + 16, &lngState);
    CHECK_LABJACK_API_ERROR(errorCode,
                            "eDI to set up DIN-" + std::to_string(i) + " ",
                            kDefaultLevel);
  }
}

void LabJackU3Controller::SetUpDOUTs() {
  long lngState = false;
  for (int i = 0; i < kNumDIn; i++) {
    auto errorCode = eDO(device_handle_, i + 8, lngState);
    CHECK_LABJACK_API_ERROR(errorCode,
                            "eDO to set up DOUT-" + std::to_string(i) + " ",
                            kDefaultLevel);
  }
}

void LabJackU3Controller::SetUpStreamMode() {
  // Set the scan rate.
  auto errorCode = AddRequest(device_handle_, LJ_ioPUT_CONFIG,
                              LJ_chSTREAM_SCAN_FREQUENCY, scan_rate_, 0, 0);
  CHECK_LABJACK_API_ERROR(
      errorCode, "AddRequest for LJ_chSTREAM_SCAN_FREQUENCY ", kDefaultLevel);

  // Configure reads to wait and retrieve the desired amount of data.
  errorCode = AddRequest(device_handle_, LJ_ioPUT_CONFIG, LJ_chSTREAM_WAIT_MODE,
                         LJ_swSLEEP, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "AddRequest for LJ_chSTREAM_WAIT_MODE ",
                          kDefaultLevel);

  // Define AIN stream
  errorCode =
      AddRequest(device_handle_, LJ_ioCLEAR_STREAM_CHANNELS, 0, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(
      errorCode, "AddRequest for LJ_ioCLEAR_STREAM_CHANNELS ", kDefaultLevel);
  for (int i = 0; i < kNumAIn; i++) {
    errorCode = AddRequest(device_handle_, LJ_ioADD_STREAM_CHANNEL, i, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(
        errorCode,
        "AddRequest for LJ_ioADD_STREAM_CHANNEL-" + std::to_string(i) + " ",
        kDefaultLevel);
    vec_aio_channel_id.at(i) = number_of_channels_;
    number_of_channels_++;
  }

  // Define DOUT stream
  errorCode = AddRequest(device_handle_, LJ_ioADD_STREAM_CHANNEL, 193, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode,
                          "AddRequest for LJ_ioADD_STREAM_CHANNEL-EIO_FIO ",
                          kDefaultLevel);
  eio_channel_id = number_of_channels_;
  number_of_channels_++;

  // Define DIN stream
  errorCode = AddRequest(device_handle_, LJ_ioADD_STREAM_CHANNEL, 194, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode,
                          "AddRequest for LJ_ioADD_STREAM_CHANNEL-MIO_CIO ",
                          kDefaultLevel);
  cio_channel_id = number_of_channels_;
  number_of_channels_++;

  // Define Counter stream
  for (int i = 0; i < kNumCounter; i++) {
    errorCode =
        AddRequest(device_handle_, LJ_ioADD_STREAM_CHANNEL, 210 + i, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(errorCode,
                            "AddRequest for LJ_ioADD_STREAM_CHANNEL-Counter-" +
                                std::to_string(i) + "(LSW) ",
                            kDefaultLevel);
    vec_counter_channel_id.at(i) = number_of_channels_;
    number_of_channels_++;

    errorCode =
        AddRequest(device_handle_, LJ_ioADD_STREAM_CHANNEL, 224, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(
        errorCode,
        "AddRequest for LJ_ioADD_STREAM_CHANNEL-TC_Capture(Counter-" +
            std::to_string(i) + "(MSW)) ",
        kDefaultLevel);
    number_of_channels_++;
  }

  // Give the UD driver a buffer_seconds_ second buffer
  errorCode =
      AddRequest(device_handle_, LJ_ioPUT_CONFIG, LJ_chSTREAM_BUFFER_SIZE,
                 scan_rate_ * number_of_channels_ * buffer_seconds_, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "AddRequest for LJ_chSTREAM_BUFFER_SIZE ",
                          kDefaultLevel);

  // Execute the requests.
  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne for Stream Init ", kDefaultLevel);
}

void LabJackU3Controller::StartGetStreamData() {
  if (!flag_stream_started_) {
    auto errorCode =
        eGet(device_handle_, LJ_ioSTART_STREAM, 0, &actual_scan_rate_, 0);
    CHECK_LABJACK_API_ERROR(errorCode, "eGet for LJ_ioSTART_STREAM ",
                            kDefaultLevel);
    flag_stream_started_ = true;
    LOG_INFO("LabJack data stream started. Actual scan rate: {}Hz",
             actual_scan_rate_);
  }
}

void LabJackU3Controller::StopGetStreamData() {
  if (flag_stream_started_) {
    auto errorCode = ePut(device_handle_, LJ_ioSTOP_STREAM, 0, 0, 0);
    CHECK_LABJACK_API_ERROR(errorCode, "eGet for LJ_ioSTOP_STREAM ",
                            kDefaultLevel);
    flag_stream_started_ = false;
    LOG_INFO("LabJack data stream stopped.");
  }
}

LabJackU3Controller::StreamDataPack LabJackU3Controller::GetStreamDataPack() {
  if (!flag_stream_started_) {
    LOG_WARN("Stream haven't been started!");
    return StreamDataPack();
  }
  // Read data until done.
  // Must set the number of scans to read each iteration, as the read
  // returns the actual number read.
  double actual_number_read = 5000;
  double dblCommBacklog = -1;
  double dblUDBacklog = -1;
  std::vector<double> raw_data(actual_number_read * number_of_channels_, -1);

  // Read the data. Note that the array passed must be sized to hold
  // enough SAMPLES, and the Value passed specifies the number of SCANS
  // to read.
  auto errorCode =
      eGetPtr(device_handle_, LJ_ioGET_STREAM_DATA, LJ_chALL_CHANNELS,
              &actual_number_read, raw_data.data());
  CHECK_LABJACK_API_ERROR(errorCode, "eGetPtr for LJ_ioGET_STREAM_DATA ",
                          kDefaultLevel);

  // Unpack data
  // std::vector<double> vec_ain_data(kNumAIn * actual_number_read);
  // std::vector<bool> vec_dout_data(kNumDOut * actual_number_read);
  // std::vector<bool> vec_din_data(kNumDIn * actual_number_read);
  // std::vector<uint32_t> vec_counter_data(kNumCounter * actual_number_read);
  // std::vector<uint16_t> uint16_dout_data(actual_number_read, -1);
  // std::vector<uint16_t> uint16_din_data(actual_number_read, -1);
  //StreamDataPack data_pack(actual_number_read);
  //for (int i = 0; i < actual_number_read; i++) {
  //  int index_base_in_raw = i * number_of_channels_;
  //  // Get AIn
  //  for (int j = 0; j < kNumAIn; j++) {
  //    data_pack.vec_ain_data.at(j + i * kNumAIn) =
  //        raw_data.at(vec_aio_channel_id.at(j) + index_base_in_raw);
  //  }
  //  // std::copy(data.begin() + index_base_in_raw, data.begin() +
  //  // index_base_in_raw + kNumAIn,
  //  //           data_pack.vec_ain_data.begin());
  //  // Get DOut
  //  uint16_t uint16_dout_data =
  //      static_cast<uint16_t>(raw_data.at(eio_channel_id + index_base_in_raw));
  //  for (int j = 0; j < kNumDOut; j++) {
  //    data_pack.vec_dout_data.at(j + i * kNumDOut) =
  //        (uint16_dout_data >> (8 + j)) & 0x1;
  //  }
  //  // Get DIn
  //  uint16_t uint16_din_data =
  //      static_cast<uint16_t>(raw_data.at(cio_channel_id + index_base_in_raw));
  //  for (int j = 0; j < kNumDIn; j++) {
  //    data_pack.vec_din_data.at(j + i * kNumDIn) = (uint16_din_data >> j) & 0x1;
  //  }
  //  // Get Counters
  //  for (int j = 0; j < kNumCounter; j++) {
  //    uint16_t counter_lsw = static_cast<uint16_t>(
  //        raw_data.at(vec_counter_channel_id.at(j) + index_base_in_raw));
  //    uint16_t counter_msw = static_cast<uint16_t>(
  //        raw_data.at(vec_counter_channel_id.at(j) + 1 + index_base_in_raw));
  //    data_pack.vec_counter_data.at(j + i * kNumCounter) =
  //        (static_cast<uint32_t>(counter_msw) << 16) | counter_lsw;
  //  }
  //}

  StreamDataPack data_pack(actual_number_read);
  for (int i = 0; i < actual_number_read; i++) {
    int index_base_in_raw = i * number_of_channels_;
    // Get AIn
    for (int j = 0; j < kNumAIn; j++) {
      data_pack.vec_ain_data.at(i,j) =
          raw_data.at(vec_aio_channel_id.at(j) + index_base_in_raw);
    }
    // std::copy(data.begin() + index_base_in_raw, data.begin() +
    // index_base_in_raw + kNumAIn,
    //           data_pack.vec_ain_data.begin());
    // Get DOut
    uint16_t uint16_dout_data =
        static_cast<uint16_t>(raw_data.at(eio_channel_id + index_base_in_raw));
    for (int j = 0; j < kNumDOut; j++) {
      data_pack.vec_dout_data.at(i, j) =
          (uint16_dout_data >> (8 + j)) & 0x1;
    }
    // Get DIn
    uint16_t uint16_din_data =
        static_cast<uint16_t>(raw_data.at(cio_channel_id + index_base_in_raw));
    for (int j = 0; j < kNumDIn; j++) {
      data_pack.vec_din_data.at(i, j) = (uint16_din_data >> j) & 0x1;
    }
    // Get Counters
    for (int j = 0; j < kNumCounter; j++) {
      uint16_t counter_lsw = static_cast<uint16_t>(
          raw_data.at(vec_counter_channel_id.at(j) + index_base_in_raw));
      uint16_t counter_msw = static_cast<uint16_t>(
          raw_data.at(vec_counter_channel_id.at(j) + 1 + index_base_in_raw));
      data_pack.vec_counter_data.at(i, j) =
          (static_cast<uint32_t>(counter_msw) << 16) | counter_lsw;
    }
  }



  // Retrieve the current U3 backlog. The UD driver retrieves
  // stream data from the U3 in the background, but if the computer
  // is too slow for some reason the driver might not be able to read
  // the data as fast as the U3 is acquiring it, and thus there will
  // be data left over in the U3 buffer.
  errorCode = eGet(device_handle_, LJ_ioGET_CONFIG, LJ_chSTREAM_BACKLOG_COMM,
                   &dblCommBacklog, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "eGet for LJ_chSTREAM_BACKLOG_COMM ",
                          kDefaultLevel);
  // Retrieve the current UD driver backlog. If this is growing, then
  // the application software is not pulling data from the UD driver
  // fast enough.
  errorCode = eGet(device_handle_, LJ_ioGET_CONFIG, LJ_chSTREAM_BACKLOG_UD,
                   &dblUDBacklog, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "eGet for LJ_chSTREAM_BACKLOG_UD ",
                          kDefaultLevel);

  {
    // std::function<std::string(const int_bool&)> int_bool_to_bool_string =
    //    [](const int_bool& c) -> std::string {
    //  return std::to_string(static_cast<bool>(c));
    //};
    // std::cout << data_pack.vec_ain_data << std::endl;
    using cpptoolkit::ToStringStream;
    LOG_TRACE("AIN: \n{}\nShape:{}", data_pack.vec_ain_data,
              xt::adapt(data_pack.vec_ain_data.shape()));
    LOG_TRACE("DOUT: \n{}\nShape:{}", data_pack.vec_dout_data,
              xt::adapt(data_pack.vec_dout_data.shape()));
    LOG_TRACE("DIN: \n{}\nShape:{}", data_pack.vec_din_data,
              xt::adapt(data_pack.vec_din_data.shape()));
    LOG_TRACE("COUNTER: \n{}\nShape:{}", data_pack.vec_counter_data,
              xt::adapt(data_pack.vec_counter_data.shape()));
  }

  return data_pack;
}

void LabJackU3Controller::CollectSignalDataAsync() {
  if (th_data_.joinable()) {
    LOG_WARN("The device is currently collecting signal data.");
    return;
  }
  th_data_ = std::thread(&LabJackU3Controller::CollectSignalData, this);
}

void LabJackU3Controller::SetUpCounters() {  // Set Up Counters
  // Set the pin offset to 4.
  auto errorCode = AddRequest(device_handle_, LJ_ioPUT_CONFIG,
                              LJ_chTIMER_COUNTER_PIN_OFFSET, 4, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode,
                          "LJ_ioPUT_CONFIG->LJ_chTIMER_COUNTER_PIN_OFFSET ",
                          kDefaultLevel);
  for (int i = 0; i < kNumCounter; i++) {
    // Enable Counter_i. It will use FIO4+i.
    errorCode = AddRequest(device_handle_, LJ_ioPUT_COUNTER_ENABLE, i, 1, 0, 0);
    CHECK_LABJACK_API_ERROR(errorCode, "LJ_ioPUT_COUNTER_ENABLE ",
                            kDefaultLevel);
    errorCode = GoOne(device_handle_);
    CHECK_LABJACK_API_ERROR(errorCode, "GoOne for LabJack Init ",
                            kDefaultLevel);
    ResetCounter(i);
  }
}

void LabJackU3Controller::ResetCounter(int channel) {
  // Read & reset Counter0. Note that with the U3 reset is just
  // setting a driver flag to reset on the next read, so reset
  // is generally combined with a read in an add/go/get block.
  // The order of the read & reset within the block does not
  // matter ... the read will always happen right before the reset.
  LJ_ERROR errorCode = -1;
  // Add request to reset counter0
  errorCode =
      AddRequest(device_handle_, LJ_ioPUT_COUNTER_RESET, channel, 1, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "Add request to reset counter0 ",
                          kDefaultLevel);
  // Add request to get counter0
  errorCode = AddRequest(device_handle_, LJ_ioGET_COUNTER, channel, 0, 0, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "Add request to get counter0 for reset ",
                          kDefaultLevel);

  errorCode = GoOne(device_handle_);
  CHECK_LABJACK_API_ERROR(errorCode, "GoOne to reset counter0 ", kDefaultLevel);
  errorCode = GetResult(device_handle_, LJ_ioGET_COUNTER, channel, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to get counter0 for reset ",
                          kDefaultLevel);
  errorCode = GetResult(device_handle_, LJ_ioPUT_COUNTER_RESET, channel, 0);
  CHECK_LABJACK_API_ERROR(errorCode, "GetResult to reset counter0 ",
                          kDefaultLevel);
}
void LabJackU3Controller::ResetAllCounters() {
  for (int i = 0; i < kNumCounter; i++) {
    ResetCounter(i);
  }
}
}  // namespace signal_flow_master