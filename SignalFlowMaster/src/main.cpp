#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <iomanip>
#include "LabJackUD.h" 
#include "labjack_u3_controller.h"

#ifdef ENABLE_CONSOLE_
#include <Windows.h>
#endif

//namespace signal_flow_master {
//
//
//int labjack_test() {
//  LJ_ERROR errorCode;
//  LJ_HANDLE handle;
//
//  // 打开第一个可用的U3设备
//  errorCode = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &handle);
//  if (errorCode != LJE_NOERROR) {
//    std::cout << "无法打开LabJack U3: " << errorCode << std::endl;
//    return 1;
//  }
//
//  for (int i = 0; i < 20; i++) {  // 持续读取
//    double voltage = 0;
//
//    // 读取模拟输入AI0的电压
//    errorCode = eGet(handle, LJ_ioGET_AIN, 0, &voltage, 0);
//    if (errorCode != LJE_NOERROR) {
//      std::cout << "读取失败: " << errorCode << std::endl;
//      break;
//    }
//
//    std::cout << "\rAI0电压: " << voltage << "V" << std::flush;
//    //Sleep(1);  // 休眠1秒（或者其他适当的时间）
//  }
//
//  
//
//
//  for (int i = 0; i < 20||true; i++) {  // 持续读取
//    double voltage = 0;
//
//    // 读取模拟输入AI0的电压
//    errorCode = eGet(handle, LJ_ioGET_AIN, 0, &voltage, 0);
//    if (errorCode != LJE_NOERROR) {
//      std::cout << "读取失败: " << errorCode << std::endl;
//      break;
//    }
//
//    std::cout << "\rAI0电压: " << voltage << "V" << std::flush;
//    // Sleep(1);  // 休眠1秒（或者其他适当的时间）
//  }
//
//  // 关闭设备
//  //CloseLabJack(handle);
//
//  return 0;
//}
//
//
//
//Protocol getUserInput() {
//  int numOps, rep, duration;
//  std::array<bool, 8> states;
//
//  std::cout << "Enter the number of operations: ";
//  std::cin >> numOps;
//
//  std::cout << "Enter the number of repetitions: ";
//  std::cin >> rep;
//
//  std::vector<Operation> ops;
//
//  for (int i = 0; i < numOps; i++) {
//    std::cout << "Enter the duration for operation " << i + 1 << ": ";
//    std::cin >> duration;
//
//    std::cout << "Enter the 8 EIO states (0 or 1) separated by spaces: ";
//    for (int j = 0; j < 8; j++) {
//      std::cin >> states[j];
//    }
//
//    ops.push_back(Operation(duration, states));
//  }
//
//  return Protocol(rep, ops);
//}
//
//
//void executeProtocol(const Protocol& protocol) {
//  LJ_ERROR ljError;    // For storing LabJack error codes
//  LJ_HANDLE ljHandle;  // LabJack device handle
//
//  // 打开第一个找到的U3设备
//  ljError = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &ljHandle);
//  if (ljError != LJE_NOERROR) {
//    std::cout << "Error opening LabJack: " << ljError << std::endl;
//    return;
//  }
//
//  for (int r = 0; r < protocol.repetitions; r++) {
//    for (const auto& op : protocol.operations) {
//      // 设置EIO状态
//      for (int i = 0; i < 8; i++) {
//        ljError = eDO(ljHandle, i+8, op.eioStates[i]);
//        if (ljError != LJE_NOERROR) {
//          std::cout << "Error setting EIO" << i << " state: " << ljError
//                    << std::endl;
//        }
//      }
//
//      // 等待指定的持续时间
//      std::this_thread::sleep_for(std::chrono::milliseconds(op.duration));
//
//      // 重置EIO状态（如果需要）可以在这里添加代码
//    }
//  }
//}
//
//int ShowLabJackU3List() {
//  const int kListMaxSize = 128;                    // Maximum size for list
//  long num_found;                                  // Number of devices found
//  std::vector<long> serial_numbers(kListMaxSize);  // Serial numbers of devices
//  std::vector<long> ids(kListMaxSize);             // IDs of devices
//  std::vector<double> addresses(kListMaxSize);     // Addresses of devices
//
//  // Calling the ListAll function
//  LJ_ERROR error = ListAll(LJ_dtU3,                // DeviceType
//                           LJ_ctUSB,               // ConnectionType
//                           &num_found,             // Number of devices found
//                           serial_numbers.data(),  // Serial numbers
//                           ids.data(),             // IDs
//                           addresses.data()        // Addresses
//  );
//
//  // Handle errors based on the returned LJ_ERROR value (optional but good
//  // practice)
//  if (error != 0) {
//    std::cerr << "ListAll function returned an error: " << error << std::endl;
//    return -1;
//  }
//
//  // Resize the vectors to discard unused elements
//  serial_numbers.resize(num_found);
//  ids.resize(num_found);
//  addresses.resize(num_found);
//
//// Output the number of devices found and all their details in a table format
//  std::cout << "Number of devices found: " << num_found << std::endl;
//  std::cout << "---------------------------------------------------------"
//            << std::endl;
//  std::cout << std::setw(12) << "Device No." << std::setw(16) << "Serial Number"
//            << std::setw(14) << "ID" << std::setw(14) << "Address" << std::endl;
//  std::cout << "---------------------------------------------------------"
//            << std::endl;
//
//  for (long i = 0; i < num_found; ++i) {
//    std::cout << std::setw(12) << i + 1 << std::setw(16) << serial_numbers[i]
//              << std::setw(14) << ids[i] << std::setw(14) << addresses[i]
//              << std::endl;
//  }
//
//  std::cout << "---------------------------------------------------------"
//            << std::endl;
//  return 0;
//}
//
//}


int main(int argc, char *argv[]) {
  using namespace signal_flow_master;
  QApplication a(argc, argv);
  // Enable console to debug
#ifdef ENABLE_CONSOLE_
  if (AllocConsole()) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
    std::cout.sync_with_stdio();
    std::wcout.sync_with_stdio();
    std::cerr.sync_with_stdio();
    std::clog.sync_with_stdio();
    std::wcerr.sync_with_stdio();
    std::wclog.sync_with_stdio();
  }
#endif

  cpptoolkit::InitLogger("logs", cpptoolkit::GetLogFileName("_log.txt"),
                   spdlog::level::trace);
  MainWindow w;
  w.show();
  w.FindDevice();
  return a.exec();
}
