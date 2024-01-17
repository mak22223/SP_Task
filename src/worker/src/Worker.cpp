#include "Worker.hpp"

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

using namespace std::chrono_literals;

Worker::Worker()
{

}

int Worker::discoverMaster(std::chrono::milliseconds timeout)
{
  std::chrono::milliseconds discoveryTimeout = timeout;

  // TODO: очистить данную функцию, так как в ней происходит (неявное) изменение полей класса
  // инициализация сокета приема
  d_receiveSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (d_receiveSocket == -1) {
    std::cerr << "Failed to create task receive socket." << std::endl;
    return 1;
  }

  d_receiveSocketAddr = {
    .sin_family = AF_INET,
    .sin_port = htons(0),
    .sin_addr = htonl(INADDR_LOOPBACK),
    // .sin_addr = htonl(INADDR_ANY),
    .sin_zero = {}
  };

  if (bind(d_receiveSocket, (sockaddr*)&d_receiveSocketAddr, sizeof(d_receiveSocketAddr)) != 0) {
    std::cerr << "Failed to bind receive socket." << std::endl;
    return 1;
  }

  // инициализация сокета обнаружения
  int discovery_sock;
  if ((discovery_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    std::cerr << "Failed to create discovery socket." << std::endl;
    return 1;
  }

  sockaddr_in discovery_addr = {
    .sin_family = AF_INET,
    .sin_port = htons(25001),
    .sin_addr = htonl(INADDR_LOOPBACK),
    // .sin_addr = htonl(INADDR_BROADCAST),
    .sin_zero = {}
  };

  listen(d_receiveSocket, 1);

  connect(discovery_sock, (sockaddr*)&discovery_addr, sizeof(discovery_addr));

  // опрашиваем сокет на наличие ответа
  pollfd polledSockets[] = { { .fd = d_receiveSocket, .events = POLLIN, .revents = 0 } };
  
  // броадкастим себя в сети и проверяем наличие ответа
  bool masterFound = false;
  auto discTimeoutTimer = std::chrono::system_clock::now();
  while (!masterFound && std::chrono::system_clock::now() - discTimeoutTimer < discoveryTimeout) {
    static auto sendTimer = std::chrono::system_clock::now();

    // TODO: сделать таймаут дискавери кастомизируемым
    if (std::chrono::system_clock::now() - sendTimer > 2000ms) {
      sendTimer = std::chrono::system_clock::now();

      std::cout << "Sending discovery packet..." << std::endl;
      // получаем номер выданного системой порта
      sockaddr_in receiveAddr;
      socklen_t addrLen = sizeof(sockaddr_in);
      getsockname(d_receiveSocket, (sockaddr*)&receiveAddr, &addrLen);
      std::string msg = "magic word:" + std::to_string(ntohs(receiveAddr.sin_port)) + "\0";
      sendMsg(discovery_sock, msg);
    }

    int eventNum = poll(polledSockets, sizeof(polledSockets) / sizeof(pollfd), 100);

    if (eventNum > 0) {
      for (size_t i = 0; i < sizeof(polledSockets) / sizeof(pollfd); ++i) {
        if (polledSockets[i].revents & POLLIN) {
          sockaddr_in masterAddr;
          socklen_t sockLen = sizeof(sockaddr_in);
          int workerSock = accept(polledSockets[i].fd, (sockaddr*)&masterAddr, &sockLen);

          if (workerSock >= 0) {
            char buf[50] = "";

            ssize_t bytesRead = recv(workerSock, buf, sizeof(buf) - 1, 0);
            buf[bytesRead] = '\0';

            // TODO: вынести в функцию
            sockaddr_in sendAddr;
            socklen_t addrLen = sizeof(sockaddr_in);
            getsockname(d_receiveSocket, (sockaddr*)&sendAddr, &addrLen);

            if (atoi(buf) == ntohs(sendAddr.sin_port)) {
              // инициализация сокета отправки
              d_sendSocket = socket(AF_INET, SOCK_STREAM, 0);
              if (d_sendSocket == -1) {
                std::cerr << "Failed to create task send socket." << std::endl;
                return 3;
              }

              d_sendSocketAddr = masterAddr;
              // TODO: получать порт из сообщения
              d_sendSocketAddr.sin_port = htons(25001);
              std::cout << "Found master. IP: " << inet_ntoa(d_sendSocketAddr.sin_addr) <<
                ", port: " << ntohs(d_sendSocketAddr.sin_port) << std::endl;

              if (connect(d_sendSocket, (sockaddr*)&d_sendSocketAddr, sizeof(d_sendSocketAddr)) != 0) {
                std::cerr << "Failed to connect send socket. Errno: " << errno << std::endl;
                return 3;
              }
              masterFound = true;
            }

            close(workerSock);
          }
        }
      }
    }
  }

  close(discovery_sock);

  if (!masterFound) {
    std::cerr << "Couldn't discover master." << std::endl;
    return 2;
  }
  return 0;
}

int Worker::sendMsg(int socket, const std::string &msg)
{
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
