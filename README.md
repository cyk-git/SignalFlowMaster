# SignalFlowMaster

 SignalFlowMaster is a control platform for managing real-time signal input and output via data acquisition cards.

## 编译环境配置(待完善)

作者只在自己的电脑上部署过一次这个软件的编译环境, 经验严重不足, 这里只给出一些要点

- 将spdlog库拷贝进入3rdPartyLibs文件夹中
- 在系统环境变量中, 把labjack驱动程序的根目录添加到系统环境变量中, 命名为LABJACK_PATH
- 使用vcpkg添加其他必要的库
