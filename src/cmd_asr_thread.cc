/*
 * @Author: W.jy
 * @Date: 2021-05-26 23:24:36
 * @FilePath: /CPR_KWS_IFlytek_Test/src/cmd_asr_thread.cc
 * @Description: desciption
 */
#include "../include/cmd_asr_thread.h"

#include <QDebug>
#include <QFile>
#include <QThread>
#include <QTime>
#include <mutex>

#include "../include/iflytek_user.h"
#include "../include/msp_cmn.h"
#include "../include/msp_errors.h"
#include "../include/qisr.h"
// Global
static QString getInfoByTag(const QString &str, const QString &tag);

static int buildGrmCb(int ecode, const char *info, void *tmp);

/******************  Definition of Some members ***********************/
// some params for IFlyTek
std::string CmdASRThread::ASR_RES_PATH = ("fo|res/asr/common.jet");
// grammar files correspond to ASRMODE [Maybe update]
std::vector<QString> CmdASRThread::grmFileVec = {"cmd.bnf", "idLast.bnf"};

#ifdef _WIN64
std::string CmdASRThread::GRM_BUILD_PATH = ("res/asr/GrmBuild_x64");
#else
std::string CmdASRThread::GRM_BUILD_PATH = ("res/asr/GrmBuild");
#endif

const int CmdASRThread::MAX_PARAMS_LEN;
const int CmdASRThread::MAX_GRAMMARID_LEN;

int CmdASRThread::sampleRate = 16000;

std::mutex CmdASRThread::m_statMutex;
CmdASRThread::ASRSTATUS CmdASRThread::m_status;
/**********************************************************************/

CmdASRThread::CmdASRThread() : m_mode(ASRMODE::RETURN_RSLT) {
  resetParams();
  m_status = ASRSTATUS::NO_INIT;
  m_iFlytekUser = IFlytekUser::getInstance();
}

CmdASRThread::~CmdASRThread() {
  std::lock_guard<std::mutex> locker(m_statMutex);
  if (m_status == ASRSTATUS::WORKING) {
    stopSession();
  }
  deInit();
  do {
    requestInterruption();
    msleep(20);
  } while ((isRunning()));
}

// concurrency safe
CmdASRThread *CmdASRThread::getInstance() {
  static CmdASRThread asrInstance;
  if (ASRSTATUS::NO_INIT == asrInstance.m_status) {
    asrInstance.init();
    m_status = ASRSTATUS::FREE;
  }

  std::lock_guard<std::mutex> locker(m_statMutex);
  return ASRSTATUS::FREE == asrInstance.m_status ? &asrInstance : NULL;
}

bool CmdASRThread::updateGrmFile() {
  if (MSP_SUCCESS != buildGrammar()) {
    qDebug() << "set Mode error";
    return false;
  }
  qDebug() << "ASR build grammar ok";
  return true;
}

int CmdASRThread::startSession() {
  std::lock_guard<std::mutex> locker(m_statMutex);
  if (ASRSTATUS::WORKING == m_status) {
    qDebug() << "ASR is working, please stopSession firstly";
    return -1;
  }

  generateParams();
  resetParams();

  session_id = QISRSessionBegin(NULL, paramsBuf, &errcode);
  if (NULL == session_id) {
    QISRSessionEnd(session_id, NULL);
    return errcode;
  }
  m_status = ASRSTATUS::WORKING;
  start();
  qDebug() << "Start recognize";
  return 0;
}

bool CmdASRThread::stopSession() {
  // stop last session
  qDebug() << "ASR : stop session";
  int ret = 0;
  ret = QISRSessionEnd(session_id, NULL);
  m_statMutex.lock();
  m_status = ASRSTATUS::FREE;
  m_statMutex.unlock();
  if (MSP_SUCCESS != ret) {
    qDebug() << "func setMode: session end error" << ret;
    return false;
  }
  return true;
}

int CmdASRThread::writeAudioData(const char *dataBuf, int len) {
  if (ASRSTATUS::WORKING == m_status) {
    // status
    if (MSP_EP_LOOKING_FOR_SPEECH == ep_status ||
        MSP_EP_IN_SPEECH == ep_status) {
      errcode = QISRAudioWrite(session_id, (const void *)dataBuf, len, aud_stat,
                               &ep_status, &rec_status);
      qDebug() << QTime::currentTime();
      if (MSP_SUCCESS != errcode) {
        QISRSessionEnd(session_id, NULL);
        qDebug() << "Write error " << errcode;
        return errcode;
      }
      aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
      qDebug() << "Continue";

      if (MSP_EP_AFTER_SPEECH == ep_status) {
        qDebug() << "ASR EP AFTER";
        m_sem.release();
      }
      if (MSP_EP_TIMEOUT == ep_status || MSP_EP_ERROR == ep_status ||
          MSP_EP_MAX_SPEECH == ep_status) {
        qDebug() << "ASR EP ERROR";
        stopSession();
        // emit asrFinished(0);
      }
    }
  }

  return 0;
}

const CmdASRThread::resultInfo CmdASRThread::resultDetail() const {
  resultInfo tmpInfo;
  if (rsltQStr.isEmpty()) {
    qDebug() << "getDetailRsltFromXml fail : str is empty";
    return tmpInfo;
  }

  // get rawtext and confidence
  const QString tag1("rawtext"), tag2("confidence");
  // 可以考虑此处停止线程
  bool okFlag;
  QString rsltXml = getInfoByTag(rsltQStr, tag1);
  int confidenceVal = getInfoByTag(rsltQStr, tag2).toInt(&okFlag, 10);

  // if(m_mode == ASRMODE::IDLAST6)
  // {
  //     rsltXml.clear();
  // }

  // get slots id
  QRegExp tmpRegExp("([1-9][0-9]{4})");
  int pos = 0;
  while ((pos = tmpRegExp.indexIn(rsltQStr, pos)) != -1) {
    pos += tmpRegExp.matchedLength();
    tmpInfo.idVec.push_back(tmpRegExp.cap(0).toInt());

    // if(m_mode == ASRMODE::IDLAST6)
    // {
    //     rsltXml += tmpRegExp.cap(0).at(4);
    // }

    qDebug() << tmpRegExp.cap(0);
  }

  tmpInfo.result = rsltXml;
  tmpInfo.confidence = confidenceVal;

  qDebug() << rsltXml << " " << confidenceVal;

  return tmpInfo;
}

void CmdASRThread::run() {
  while (!isInterruptionRequested()) {
    while (m_sem.tryAcquire(1)) {
      qDebug() << "Thread : ASR";
      getResult();
    }
    msleep(20);
  }
}

int CmdASRThread::init() {
  int ret = 0;
  // Login
  ret =
      m_iFlytekUser
          ->login();  // 第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
  if (MSP_SUCCESS != ret) {
    qDebug() << "IFlyTek Login Error " << ret;
    return ret;
  }
  if (grmFileVec.empty()) {
    qDebug() << Q_FUNC_INFO << "grmFileVec is empty" << ret;
    return MSP_ERROR_FAIL;
  }
  m_grmFile = grmFileVec[0];
  // Build grammar
  qDebug() << "Start build grammar";
  ret = buildGrammar();

  if (MSP_SUCCESS != ret)
    deInit();
  else {
    qDebug() << "ASR Init OK";
  }
  return ret;
}

void CmdASRThread::getResult() {
  while (MSP_REC_STATUS_COMPLETE != rss_status && MSP_SUCCESS == errcode) {
    rstBuf = QISRGetResult(session_id, &rss_status, 0, &errcode);
    msleep(150);
  }

  rsltQStr = QString::fromUtf8(rstBuf);
  qDebug() << "识别结束!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

  qDebug() << (NULL == rstBuf ? "没有识别结果" : rsltQStr);
  stopSession();
  startSession();
  // emit asrFinished(NULL != rstBuf);
  return;
}

void CmdASRThread::generateParams() {
  snprintf(paramsBuf, MAX_PARAMS_LEN - 1,
           "engine_type = local, \
        asr_res_path = %s, sample_rate = %d, \
        grm_build_path = %s, local_grammar = %s, \
        result_type = xml, result_encoding = UTF-8, ",
           ASR_RES_PATH.c_str(), sampleRate, GRM_BUILD_PATH.c_str(),
           usrData.grammar_id);
  qDebug() << "c_str() : " << paramsBuf;
  return;
}

int CmdASRThread::buildGrammar() {
  char grmParamsBuf[500];
  QFile tmpFile(m_grmFile);

  qDebug() << m_grmFile;

  tmpFile.open(QIODevice::ReadOnly);
  int len = tmpFile.size();
  QByteArray tmpByteArr = tmpFile.readAll();

  memset(&usrData, 0, sizeof(usrData));
  snprintf(grmParamsBuf, 500 - 1,
           "engine_type = local, \
        asr_res_path = %s, sample_rate = %d, \
        grm_build_path = %s, ",
           ASR_RES_PATH.c_str(), sampleRate, GRM_BUILD_PATH.c_str());

  qDebug() << tmpFile.size() << " " << tmpByteArr.size();
  qDebug() << grmParamsBuf;

  int ret = QISRBuildGrammar("bnf", tmpByteArr.data(), len, grmParamsBuf,
                             buildGrmCb, &usrData);

  if (MSP_SUCCESS != ret) {
    qDebug() << "ASR build grammar: API Error " << ret;
    return ret;
  }
  while (1 != usrData.build_fini) msleep(300);
  if (MSP_SUCCESS != usrData.errcode) {
    qDebug() << "ASR build grammar : usrData Error" << usrData.errcode;
    return ret;
  }

  qDebug() << "ASR build grammar ok";
  return ret;
}

static int buildGrmCb(int ecode, const char *info, void *uData) {
  CmdASRThread::UserData *grm_data =
      reinterpret_cast<CmdASRThread::UserData *>(uData);

  if (NULL != grm_data) {
    grm_data->build_fini = 1;
    grm_data->errcode = ecode;
  }

  if (MSP_SUCCESS == ecode && NULL != info) {
    qDebug() << "Build Grammar OK ! Grammar ID: " << info;
    if (NULL != grm_data)
      snprintf(grm_data->grammar_id, CmdASRThread::MAX_GRAMMARID_LEN - 1, info);
  } else
    qDebug() << "Build Grammar Error ! errcode: " << ecode;

  return 0;
}

static QString getInfoByTag(const QString &str, const QString &tag) {
  QString rsltStr;
  // 可以考虑此处停止线程
  int lPos = str.indexOf(tag);
  int rPos = str.indexOf(tag, lPos + 1);

  lPos += tag.size() + 1;  // <tag>result</tag>
  rPos -= 2;

  if (lPos >= rPos) {
    return rsltStr;
  }

  rsltStr = str.mid(lPos, rPos - lPos);
  return rsltStr;
}
