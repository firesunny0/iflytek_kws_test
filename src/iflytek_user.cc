/*
 * @Author: W.jy
 * @Date: 2021-08-18 11:42:57
 * @LastEditTime: 2023-01-05 04:08:58
 * @FilePath: /CPR_KWS_IFlytek_Test/src/iflytek_user.cc
 * @Description:
 */
#include "../include/iflytek_user.h"

#include <assert.h>

#include <QDebug>
#include <QFile>
#include <string>

#include "../include/msp_cmn.h"
#include "../include/msp_errors.h"
#include "../include/msp_types.h"
std::string IFlytekUser::usrId = "appid = abe83e12";

IFlytekUser::IFlytekUser() : m_appCnt(0) {
  QFile idFile("iflytek.cfg");
  assert(idFile.open(QIODevice::ReadOnly | QIODevice::Text));
  QByteArray tmpLine = idFile.readLine();
  usrId = "appid = " + tmpLine.toStdString();
  qDebug() << usrId.c_str();
}

int IFlytekUser::login() {
  std::lock_guard<std::mutex> lock(m_mutex);
  int ret = 0;
  if (0 == m_appCnt) {
    ret = MSPLogin(
        NULL, NULL,
        usrId
            .c_str());  // 第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
    if (MSP_SUCCESS != ret) {
      qDebug() << "IFlyTek Login Error " << ret;
      MSPLogout();
      return ret;
    }
    ++m_appCnt;
  }
  return MSP_SUCCESS;
}
