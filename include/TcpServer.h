/*
 * @Author: firesunny
 * @Date: 2023-01-04 22:06:29
 * @LastEditTime: 2023-01-04 22:14:36
 * @FilePath: /CPR_KWS_IFlytek_Test/include/TcpServer.h
 * @Description: base on kaldi
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

class TcpServer {
 public:
  using int32 = int32_t;
  using int16 = int16_t;

  explicit TcpServer(int read_timeout);
  ~TcpServer();

  bool Listen(int32 port);  // start listening on a given port
  int32 Accept();           // accept a client and return its descriptor

  bool ReadChunk(
      size_t len);  // get more data and return false if end-of-stream

  int GetChunkBuf(int16 *buf, int len);
  std::vector<double> GetChunk();  // get the data read by above method

  bool Write(const std::string &msg);  // write to accepted client
  bool WriteLn(const std::string &msg,
               const std::string &eol = "\n");  // write line to accepted client

  void Disconnect();

 private:
  struct ::sockaddr_in h_addr_;
  int32 server_desc_, client_desc_;
  int16 *samp_buf_;
  size_t buf_len_, has_read_;
  pollfd client_set_[1];
  int read_timeout_;
};