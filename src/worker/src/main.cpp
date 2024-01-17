#include <chrono>
#include <iostream>

#include "Worker.hpp"

using namespace std::chrono_literals;

/* Задачи для реализации:
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
  (void)argc;
  (void)argv;

  Worker worker;
  if (worker.discoverMaster(60000ms) == 0) {
    std::cout << "Master discovered successfully." << std::endl;
  } 


  return 0;
}