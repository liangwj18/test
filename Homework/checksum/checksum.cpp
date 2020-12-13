#include <stdint.h>
#include <stdlib.h>
//my change
#include <iostream>
using namespace std;
/**
 * @brief 进行 IP 头的校验和的验证
 * @param packet 完整的 IP 头和载荷
 * @param len 即 packet 的长度，单位是字节，保证包含完整的 IP 头
 * @return 校验和无误则返回 true ，有误则返回 false
 */
bool validateIPChecksum(uint8_t *packet, size_t len) {
  // TODO:
  int check = (packet[10] << 8) + packet[11];
  packet[10] = 0x0;
  packet[11] = 0x0;
  int length = packet[0];
  length &= 0xF;
  length *= 4;
  int result = 0;
  for(int i = 0;i < length;i += 2){
    result += ((packet[i] << 8) + packet[i+1]);
  }
  while(result > 0xFFFF){
    int tmp = result / 0x10000;
    result %= 0x10000;
    result += tmp;
  }
  result ^= 0xFFFF;
  if(result == check)
    return true;
  else
    return false;
}

uint32_t getIPChecksum(uint8_t *packet){
  int check = (packet[10] << 8) + packet[11];
  packet[10] = 0x0;
  packet[11] = 0x0;
  int length = packet[0];
  length &= 0xF;
  length *= 4;
  int result = 0;
  for(int i = 0;i < length;i += 2){
    result += ((packet[i] << 8) + packet[i+1]);
  }
  while(result > 0xFFFF){
    int tmp = result / 0x10000;
    result %= 0x10000;
    result += tmp;
  }
  result ^= 0xFFFF;
  return result;
}

bool validateICMPChecksum(uint8_t *packet, size_t len) {
    // TODO:
    int check = (packet[2] << 8) + packet[3];
    packet[2] = 0x0;
    packet[3] = 0x0;
    int length = 64;
    int result = 0;
    for(int i = 0;i < length;i += 2){
        result += ((packet[i] << 8) + packet[i+1]);
    }
    while(result > 0xFFFF){
        int tmp = result / 0x10000;
        result %= 0x10000;
        result += tmp;
    }
    result ^= 0xFFFF;
    if(result == check)
        return true;
    else
        return false;
}

uint32_t getICMPChecksum(uint8_t *packet){
  int check = (packet[2] << 8) + packet[3];
    packet[2] = 0x0;
    packet[3] = 0x0;
    int length = 64;
    int result = 0;
    for(int i = 0;i < length;i += 2){
        result += ((packet[i] << 8) + packet[i+1]);
    }
    while(result > 0xFFFF){
        int tmp = result / 0x10000;
        result %= 0x10000;
        result += tmp;
    }
    result ^= 0xFFFF;
    return result;
}
