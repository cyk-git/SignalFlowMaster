# SignalFlowMaster

 SignalFlowMaster is a control platform for managing real-time signal input and output via data acquisition cards.

## 编译环境配置(待完善)

现在此库基于Visual Studio内置的vcpkg进行环境配置，编译器使用VS2022+Qt5.15.2对应的编译器。需注意：

* Visual Studio请不要安装在其默认路径下，它的默认路径中含有空格，导致boost库无法正常使用vcpkg安装。请将Visual Studio安装到一个编程友好的目录下（不含空格，仅由字母、连字符“-”、下划线“_”组成）。
* 在系统环境变量中, 把labjack驱动程序的根目录添加到系统环境变量中, 命名为LABJACK_PATH，这个vcpkg管不了。

~~作者只在自己的电脑上部署过一次这个软件的编译环境, 经验严重不足, 这里只给出一些要点~~

- ~~将spdlog库拷贝进入3rdPartyLibs文件夹中~~
- ~~在系统环境变量中, 把labjack驱动程序的根目录添加到系统环境变量中, 命名为LABJACK_PATH~~
- ~~使用vcpkg添加其他必要的库~~
