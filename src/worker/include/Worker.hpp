#pragma once

#include <netinet/in.h>
#include <chrono>
#include <string>

class Worker
{
public:
  Worker();

  int discoverMaster(std::chrono::milliseconds timeout);

protected:
  int sendMsg(int socket, const std::string &msg);

protected:
  int d_receiveSocket;
  sockaddr_in d_receiveSocketAddr;

  int d_sendSocket;
  sockaddr_in d_sendSocketAddr;

};