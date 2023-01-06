/*
 * @Author: W.jy
 * @Date: 2021-08-18 11:42:57
 * @LastEditTime: 2023-01-05 03:20:50
 * @FilePath: /CPR_KWS_IFlytek_Test/include/iflytek_user.h
 * @Description:
 */
#ifndef IFLYTEKUSER_H
#define IFLYTEKUSER_H

#include <mutex>
#include <string>

#include "msp_cmn.h"

class IFlytekUser {
 public:
  IFlytekUser(const IFlytekUser &) = delete;
  IFlytekUser(IFlytekUser &&) = delete;
  IFlytekUser &operator=(IFlytekUser &&) = delete;

  static IFlytekUser *getInstance() {
    static IFlytekUser iFlytekUsrInstance;
    return &iFlytekUsrInstance;
  }

  int login();

  void logout() {
    std::lock_guard<std::mutex> locker(m_mutex);
    if (m_appCnt > 1)
      --m_appCnt;
    else {
      m_appCnt = 0;
      MSPLogout();
    }
  }

 private:
  explicit IFlytekUser();
  static std::string usrId;
  int m_appCnt;
  std::mutex m_mutex;
};

#endif  // IFLYTEKUSER_H
