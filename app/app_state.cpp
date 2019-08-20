#include "app_state.h"


namespace tag12 {

AppState::AppState(STM32Sleep * const sleep) : sleep_(sleep) {
  // Nothing
}

AppStateData AppState::GetAppStateData() {
  uint32_t data_res = sleep_->rtc_read_backup_reg(STATE_REG);
  AppStateData appstate_data;
  appstate_data.intValue = data_res;

  return appstate_data;
}

void AppState::SetState(int state) {
  AppStateData data = GetAppStateData();
  data.bits.state = state;
  StoreAppStateData(data);
}

void AppState::SetLogErrSent(bool err) {
  AppStateData data = GetAppStateData();
  data.bits.log_err_sent = static_cast<int>(err);
  StoreAppStateData(data);
}

void AppState::SetLastWokeupAcc(bool wokenup) {
  AppStateData data = GetAppStateData();
  data.bits.last_wokeup_acc = static_cast<int>(wokenup);
  StoreAppStateData(data);
}

void AppState::StoreAppStateData(const AppStateData & data) {
  sleep_->rtc_write_backup_reg(STATE_REG, data.intValue);
}

}  // namespace tag12