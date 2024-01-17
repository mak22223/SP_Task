#include <Master.hpp>

#include <arpa/inet.h>
#include <chrono>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <iostream>
#include <string.h>

Master::Master(size_t workerNum)
: d_workerNumber(workerNum)
{
  // TODO: создать возможность проверки успешно ли создался объект
  d_receiveSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (d_receiveSocket == -1) {
    std::cerr << "Failed to create receive socket." << std::endl;
  }

  d_receiveSocketAddr = {
    .sin_family = AF_INET,
    .sin_port = htons(25001),
    .sin_addr = htonl(INADDR_LOOPBACK),
    // .sin_addr = htonl(INADDR_ANY),
    .sin_zero = {}
  };

  if (bind(d_receiveSocket, (sockaddr*)&d_receiveSocketAddr, sizeof(d_receiveSocketAddr)) != 0) {
    std::cerr << "Failed to bind receive socket." << std::endl;
  }
  
  listen(d_receiveSocket, workerNum);
};

int Master::discoverWorkers(unsigned timeoutMsec)
{
  std::chrono::milliseconds discoveryTimeout(timeoutMsec);

  // инициализация сокета обнаружения
  int discovery_sock;
  if ((discovery_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    std::cerr << "Failed to create a socket." << std::endl;
    return 1;
  }

  sockaddr_in discovery_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(25001),
    // .sin_addr = htonl(INADDR_ANY),
    .sin_addr = htonl(INADDR_LOOPBACK),
    .sin_zero = {}
  };

  if (bind(discovery_sock, (sockaddr*)&discovery_addr, sizeof(discovery_addr)) != 0) {
    std::cerr << "Failed to bind the socket." << std::endl;
    return 1;
  }
  fcntl(discovery_sock, F_SETFL, O_NONBLOCK);

  listen(discovery_sock, d_workerNumber);

  pollfd polledSockets[] = { { .fd = discovery_sock, .events = POLLIN, .revents = 0 } };
  
  auto discTimeoutTimer = std::chrono::system_clock::now();
  while (d_workers.size() < d_workerNumber && std::chrono::system_clock::now() - discTimeoutTimer < discoveryTimeout) {
    // обнаружение указанного количества воркеров
    std::cout << "Polling for " << d_workerNumber << " workers." << std::endl;
    int eventNum = poll(polledSockets, sizeof(polledSockets) / sizeof(pollfd), discoveryTimeout.count() / 10 + 1); /// !

    if (eventNum > 0) {
      for (size_t i = 0; i < sizeof(polledSockets) / sizeof(pollfd); ++i) {
        if (polledSockets[i].revents & POLLIN) {
          sockaddr_in datagrammAddr;
          socklen_t sockLen = sizeof(sockaddr_in);
          char buf[50] = "";
          ssize_t bytesRead = recvfrom(polledSockets[i].fd, buf, sizeof(buf) - 1, 0, (sockaddr*)&datagrammAddr, &sockLen);

          if (d_workers.size() < d_workerNumber && bytesRead > 0) {
            // TODO: проверить случай неполного получения сообщения
            buf[sizeof(buf) - 1] = '\0';

            // TODO: прочитать данные и вернуть обратно воркеру

            int port = 0;
            if (bytesRead >= 11) {
              std::string str = buf;
              if (str.substr(0, 11) == "magic word:") {
                try {
                  port = std::stoi(str.substr(11, 5));
                }
                catch (std::invalid_argument const &ex) {
                  std::cerr << "Could not convert worker port number." << std::endl;
                }
              }
            }

            if (port == 0) {
              std::cerr << "Not a worker tried to connect." << std::endl;
            } else {
              std::cout << "Found new worker. IP: " << inet_ntoa(datagrammAddr.sin_addr) <<
                ", port: " << port << std::endl;
              datagrammAddr.sin_port = port;

              // отправляем ответ воркеру
              sockaddr_in workerAddr = {
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr = datagrammAddr.sin_addr,
                .sin_zero = {}
              };

              d_sendSocket = socket(AF_INET, SOCK_STREAM, 0);
              if (d_sendSocket == -1) {
                std::cerr << "Failed to create send socket." << std::endl;
              }
              if (connect(d_sendSocket, (sockaddr*)&workerAddr, sizeof(workerAddr)) != 0) {
                std::cerr << "Failed to open send socket to send discovery data to worker. Errno: " << errno << std::endl;
                return 3;
              }
              if (sendMsg(d_sendSocket, std::to_string(port)) != 0) {
                std::cerr << "Failed to send discovery data to worker." << std::endl;
              }
              if (close(d_sendSocket) != 0) {
                std::cerr << "Failed to close send socket." << std::endl;
              }

              d_workers.push_back({ datagrammAddr });
              // TODO: отправить ответ воркеру с данными мастера
            }
          }
        }
      }
    }
  }

  close(discovery_sock);

  if (d_workers.size() != d_workerNumber) {
    std::cerr << "Couldn't discover number of workers set." << std::endl;
    return 2;
  }
  return 0;
}

int Master::sendMsg(int socket, const std::string &msg)
{
  // TODO: Добавить таймаут
  size_t sentBytes = 0;
  do {
    int sent = send(socket, &msg.c_str()[sentBytes], msg.length() - sentBytes, 0);
    if (sent != -1) {
      sentBytes += sent;
    } else {
      return -1;
    }
  }
  while (sentBytes != msg.length());
  return 0;
}