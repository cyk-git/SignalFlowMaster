/*
 * labjack_u3_ctrl_ui_interface.h
 *
 * Created on 20231026
 *   by Yukun Cheng
 *   cyk_phy@mail.ustc.edu.cn
 *
 */

#ifndef SIGNAL_FLOW_MASTER_LABJACK_U3_CTRL_UI_INTERFACE_H_
#define SIGNAL_FLOW_MASTER_LABJACK_U3_CTRL_UI_INTERFACE_H_

namespace signal_flow_master {
class LabJackU3CtrlUIInterface {
 public:
  explicit LabJackU3CtrlUIInterface() = default;
  virtual ~LabJackU3CtrlUIInterface() = default;

  // Disallow copy and move
  LabJackU3CtrlUIInterface(const LabJackU3CtrlUIInterface&) = delete;
  LabJackU3CtrlUIInterface& operator=(const LabJackU3CtrlUIInterface&) = delete;
  LabJackU3CtrlUIInterface(LabJackU3CtrlUIInterface&&) = delete;
  LabJackU3CtrlUIInterface& operator=(LabJackU3CtrlUIInterface&&) = delete;

  // Define interface methods here
  //virtual void ShowUI() = 0;
  //virtual void UpdateStatus() = 0;
};
}  // namespace signal_flow_master

#endif  // SIGNAL_FLOW_MASTER_LABJACK_U3_CTRL_UI_INTERFACE_H_