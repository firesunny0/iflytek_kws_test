/*
 * @Author: firesunny
 * @Date: 2023-01-04 22:06:16
 * @LastEditTime: 2023-01-05 01:03:38
 * @FilePath: /CPR_KWS_IFlytek_Test/src/TcpServer.cc
 * @Description: base on kaldi
 */
#include "../include/TcpServer.h"

#include "glog/logging.h"

TcpServer::TcpServer(int read_timeout) {
  server_desc_ = -1;
  client_desc_ = -1;
  samp_buf_ = NULL;
  buf_len_ = 0;
  read_timeout_ = 1000 * read_timeout;
}

bool TcpServer::Listen(int32 port) {
  h_addr_.sin_addr.s_addr = INADDR_ANY;
  h_addr_.sin_port = htons(port);
  h_addr_.sin_family = AF_INET;

  server_desc_ = socket(AF_INET, SOCK_STREAM, 0);

  if (server_desc_ == -1) {
    LOG(ERROR) << "Cannot create TCP socket!";
    return false;
  }

  int32 flag = 1;
  int32 len = sizeof(int32);
  if (setsockopt(server_desc_, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) {
    LOG(ERROR) << "Cannot set socket options!";
    return false;
  }

  if (bind(server_desc_, (struct sockaddr *)&h_addr_, sizeof(h_addr_)) == -1) {
    LOG(ERROR) << "Cannot bind to port: " << port << " (is it taken?)";
    return false;
  }

  if (listen(server_desc_, 1) == -1) {
    LOG(ERROR) << "Cannot listen on port!";
    return false;
  }

  LOG(INFO) << "TcpServer: Listening on port: " << port;

  return true;
}

TcpServer::~TcpServer() {
  Disconnect();
  if (server_desc_ != -1) close(server_desc_);
  delete[] samp_buf_;
}

int32_t TcpServer::Accept() {
  LOG(INFO) << "Waiting for client...";

  socklen_t len;

  len = sizeof(struct sockaddr);
  client_desc_ = accept(server_desc_, (struct sockaddr *)&h_addr_, &len);

  struct sockaddr_storage addr;
  char ipstr[20];

  len = sizeof addr;
  getpeername(client_desc_, (struct sockaddr *)&addr, &len);

  struct sockaddr_in *s = (struct sockaddr_in *)&addr;
  inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

  client_set_[0].fd = client_desc_;
  client_set_[0].events = POLLIN;

  LOG(INFO) << "Accepted connection from: " << ipstr;

  return client_desc_;
}

bool TcpServer::ReadChunk(size_t len) {
  // try to reuse buffer
  if (buf_len_ != len) {
    buf_len_ = len;
    delete[] samp_buf_;
    samp_buf_ = new int16[len];
  }

  ssize_t ret;
  int poll_ret;
  char *samp_buf_p = reinterpret_cast<char *>(samp_buf_);
  size_t to_read = len * sizeof(int16);
  has_read_ = 0;
  while (to_read > 0) {
    poll_ret = poll(client_set_, 1, read_timeout_);
    if (poll_ret == 0) {
      LOG(WARNING) << "Socket timeout! Disconnecting..."
                   << "(has_read_ = " << has_read_ << ")";
      break;
    }
    if (poll_ret < 0) {
      LOG(WARNING) << "Socket error! Disconnecting...";
      break;
    }
    ret = read(client_desc_, static_cast<void *>(samp_buf_p + has_read_),
               to_read);
    if (ret <= 0) {
      LOG(WARNING) << "Stream over...";
      break;
    }
    to_read -= ret;
    has_read_ += ret;
  }
  has_read_ /= sizeof(int16);
  // LOG(INFO) << "Read data(int16) count is " << has_read_;

  return has_read_ > 0;
}

int TcpServer::GetChunkBuf(int16 *buf, int len) {
  // buf.resize(static_cast<int32>(has_read_));

  // for (int i = 0; i < has_read_; i++)
  //   buf[i] = static_cast<double>(samp_buf_[i]);
  int to_read = len;
  if (to_read > has_read_) to_read = has_read_;
  // else
  // LOG(WARNING) << "buf smaller than has_read_";

  memcpy((char *)buf, (char *)samp_buf_, to_read * sizeof(int16));
  return to_read;
}

std::vector<double> TcpServer::GetChunk() {
  std::vector<double> buf;

  buf.resize(static_cast<int32>(has_read_));

  for (int i = 0; i < has_read_; i++)
    buf[i] = static_cast<double>(samp_buf_[i]);

  return buf;
}

bool TcpServer::Write(const std::string &msg) {
  const char *p = msg.c_str();
  size_t to_write = msg.size();
  size_t wrote = 0;
  while (to_write > 0) {
    ssize_t ret =
        write(client_desc_, static_cast<const void *>(p + wrote), to_write);
    if (ret <= 0) return false;

    to_write -= ret;
    wrote += ret;
  }

  return true;
}

bool TcpServer::WriteLn(const std::string &msg, const std::string &eol) {
  if (Write(msg))
    return Write(eol);
  else
    return false;
}

void TcpServer::Disconnect() {
  if (client_desc_ != -1) {
    close(client_desc_);
    client_desc_ = -1;
  }
}