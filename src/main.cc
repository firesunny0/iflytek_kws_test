#include <iostream>
#include <string>
#include <vector>

#include "../include/TcpServer.h"
#include "../include/cmd_asr_thread.h"
#include "glog/logging.h"
int main() {
  using int16 = int16_t;
  using namespace std;
  // google::InitGoogleLogging("KWS_Test");
  // google::SetLogDestination(google::GLOG_INFO, "../../log/");
  // FLAGS_log_dir = "../../log/";

  int read_timeout = 3;
  int chunk_len = 16000 * 0.02;
  int port_num = 7779;

  signal(SIGPIPE, SIG_IGN);  // ignore SIGPIPE to avoid crashing when socket
                             // forcefully disconnected
  TcpServer server(read_timeout);
  auto cmd_asr_handler = CmdASRThread::getInstance();
  server.Listen(port_num);
  LOG(INFO) << "Wait accept at port " << port_num;
  // cmd_asr_handler->stopSession();
  cmd_asr_handler->startSession();
  LOG(INFO) << "cmd asr startSession" << port_num;
  int16 *buf = new int16[chunk_len];
  while (true) {
    LOG(INFO) << "start listen at " << port_num;
    server.Accept();
    int total_len = 0;
    LOG(INFO) << "Accept a client at " << port_num;
    bool eos = false;
    while (true) {
      eos = !server.ReadChunk(chunk_len);
      if (eos) {
        server.Disconnect();
        LOG(INFO) << "EOS and server disconnected";
        LOG(INFO) << "Duration is " << total_len / 16000 << " seconds";
        break;
      }
      // vector<double> wave_part = server.GetChunk();
      int tmpLen = server.GetChunkBuf(buf, chunk_len);
      cmd_asr_handler->writeAudioData((char *)buf, tmpLen * sizeof(int16));
      total_len += tmpLen;
    }
  }
}
