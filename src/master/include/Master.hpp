#pragma once

#include <netinet/in.h>
#include <string>
#include <vector>

/*    Worker struct     */
struct Worker
{
  sockaddr_in addr;
};

/*    Master class      */
class Master
{
public:
  Master(size_t workerNum);
  
  int discoverWorkers(unsigned timeoutMsec = 60000);

protected:
  int sendMsg(int socket, const std::string &msg);

protected:
  size_t d_workerNumber;
  std::vector<Worker> d_workers;

  int d_receiveSocket;
  sockaddr_in d_receiveSocketAddr;

  int d_sendSocket;
  sockaddr_in d_sendSocketAddr;

};