#ifndef APP_STATE_H
#define APP_STATE_H

#include "../../system/dev/stm32_sleep.h"
#include "config_default.h"


namespace tag12 {

typedef union app_state_data {
  struct {
    unsigned state : 4;  // 4 bits  state machine  
    unsigned log_err_sent : 1;  // 1 previous message lost
    unsigned last_wokeup_acc: 1; // if the last woke up was by acc
    unsigned backup : 25; // just backup, not in use
  } bits;
  uint32_t intValue;
} AppStateData;


/**
* The class is reponsible to keep the application state 
* during the sleeping period
* 
*/
class AppState {
 public:
  AppState(STM32Sleep * const sleep);

  AppStateData GetAppStateData();
  void SetState(int state);
  void SetLogErrSent(bool err);
  void SetLastWokeupAcc(bool wokenup);
 private:
  void StoreAppStateData(const AppStateData & data);
  STM32Sleep * const sleep_;
};

} // namespace tag12

#endif
