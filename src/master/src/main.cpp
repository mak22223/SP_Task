#include <chrono>
#include <iostream>

#include "Master.hpp"

using namespace std::chrono_literals;

/* 
 * Задачи для реализации:
 * - работа с сокетами
 * - обнаружение и установка соединения
 * - keepalive для обоих сторон
 * - обработка аргументов
 * - считывание исходных данных
 * - распределение задачи и получение результатов
 * - распараллеливание задачи
 * - реализация обрабатываемой функции 
 */


int main(int argc, char **argv)
{
  // вынести в параметры программы
  (void)argc;
  (void)argv;
  int workersNumber = 5;
  std::chrono::milliseconds discoveryTimeout = 60000ms;

  Master master(workersNumber);

  if (master.discoverWorkers() != 0) {
    std::cerr << "Failed to discover workers." << std::endl;
    return 1;
  }
  std::cout << "Workers discovered successfully." << std::endl;

  return 0;
}