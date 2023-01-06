#ifndef CMDASRTHREAD_H
#define CMDASRTHREAD_H

// #include <QMutex>
// #include <QObject>
#include <QSemaphore>
#include <QThread>
#include <mutex>
// #include <semaphore>
// #include <thread>
#include <vector>

#include "iflytek_user.h"
/*
讯飞离线命令词不支持并发(Singleton)
getInstance() 获得对象指针,自动完成登录等初始化工作
status() 返回对象状态(FREE, WORKING)

setMode() 设置工作模式[需要在FREE状态下调用]
startSession()
stopForResult()
stopSession()
writeAudioData()
run() 开启新的线程不断获取ASR状态，如果有结果，获取结果并发出信号
*/
class CmdASRThread final : public QThread {
  Q_OBJECT
 public:
  CmdASRThread(const CmdASRThread &) = delete;
  CmdASRThread(CmdASRThread &&) = delete;
  CmdASRThread &operator=(CmdASRThread &&) = delete;

  static const int MAX_GRAMMARID_LEN = 32;
  typedef struct _UserData {
    int build_fini;   // 标识语法构建是否完成
    int update_fini;  // 标识更新词典是否完成
    int errcode;      // 记录语法构建或更新词典回调错误码
    char grammar_id[MAX_GRAMMARID_LEN];  // 保存语法构建返回的语法ID
  } UserData;

  typedef struct ASR_Reslut_Info {
    QString result;
    int confidence;
    std::vector<int> idVec;
  } resultInfo;

  enum ASRSTATUS { NO_INIT = 0, FREE, WORKING };

  enum ASRMODE {
    RETURN_RSLT = 0,  // asr mode return string
    RETURN_BOOL,      // !!![ Unimplemented ] asr mode return bool (need to set
                      // desired result)
  };

  static CmdASRThread *getInstance();
  ~CmdASRThread();

  const ASRSTATUS &status() const { return m_status; }

  // update grammar file
  bool updateGrmFile();

  // mode of ASR
  ASRMODE mode() const { return m_mode; }

  void setMode(ASRMODE mode) { m_mode = mode; }

  // start or stop a ASR session
  int startSession();
  bool stopSession();

  // result
  const resultInfo resultDetail() const;

  const QString &resultAsXml() const { return rsltQStr; }

  // write audio data to be recongnized
  int writeAudioData(const char *dataBuf, int len);

  // thread function
  void run();

  /*
      Some functions to set params (Unimplemented)
  */

  //  signals:
  //   // return the result is available or not
  //   void asrFinished(bool rsltFlag);

 protected:
  // ctor
  CmdASRThread();

 private:
  // login for SDK API
  int init();  // !!!
  // Logout
  void deInit() const {
    m_iFlytekUser->logout();
    return;
  }
  // generate params
  void generateParams();
  // get result from asr when session is finished
  void getResult();

  // reset params for a new session
  void resetParams() {
    aud_stat = MSP_AUDIO_SAMPLE_FIRST;
    ep_status = MSP_EP_LOOKING_FOR_SPEECH;
    rec_status = MSP_REC_STATUS_INCOMPLETE;
    rss_status = MSP_REC_STATUS_INCOMPLETE;
    errcode = -1;
  }

  // build grammar
  int buildGrammar();

  // Use but own
  IFlytekUser *m_iFlytekUser;

  ASRMODE m_mode;

  static std::mutex m_statMutex;
  static ASRSTATUS m_status;

  // params for ASR session
  static const int MAX_PARAMS_LEN = 1024;
  char paramsBuf[MAX_PARAMS_LEN];
  // id for a ASR session
  const char *session_id = NULL;
  // ptr to get result from ASR
  const char *rstBuf;
  // semaphore to get result
  QSemaphore m_sem;

  int aud_stat;
  int ep_status;
  int rec_status;
  int rss_status;
  int errcode;

  UserData usrData;
  // grammar file
  static std::vector<QString> grmFileVec;
  QString m_grmFile;
  // offline asr resource
  static std::string ASR_RES_PATH;
  static std::string GRM_BUILD_PATH;
  // params
  static int sampleRate;
  // Result
  QString rsltQStr;
};

#endif  // CMDASRTHREAD_H
