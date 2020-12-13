#include "rip.h"
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;
/*
  在头文件 rip.h 中定义了结构体 `RipEntry` 和 `RipPacket` 。
  你需要从 IPv4 包中解析出 RipPacket 结构体，也要从 RipPacket 结构体构造出对应的
  IP 包。 由于 RIP 包结构本身不记录表项的个数，需要从 IP 头的长度中推断，所以在
  RipPacket 中额外记录了个数。 需要注意这里的地址都是用 **网络字节序（大端序）**
  存储的，1.2.3.4 在小端序的机器上被解释为整数 0x04030201 。
*/

/**
 * @brief 从接受到的 IP 包解析出 RIP 协议的数据
 * @param packet 接受到的 IP 包
 * @param len 即 packet 的长度
 * @param output 把解析结果写入 *output
 * @return 如果输入是一个合法的 RIP 包，把它的内容写入 RipPacket 并且返回
 * true；否则返回 false
 *
 * IP 包的 Total Length 长度可能和 len 不同，当 Total Length 大于 len
 * 时，把传入的 IP 包视为不合法。 你不需要校验 IP 头和 UDP 的校验和是否合法。
 * 你需要检查 Command 是否为 1 或 2，Version 是否为 2， Zero 是否为 0，
 * Family 和 Command 是否有正确的对应关系（见上面结构体注释），Tag 是否为 0，
 * Metric 是否在 [1,16] 的区间内，
 * Mask 的二进制是不是连续的 1 与连续的 0 组成等等。
 */
bool disassemble(const uint8_t *packet, uint32_t len, RipPacket *output) {
  // TODO:
  // 38 - start of UDP
  // 43-44 length of UDP (include RIP)
  // 46 - RIP
  uint8_t ihl = 4 * (packet[0] & 0xF);
  // cout << hex << "ihl = " << ihl << endl;
  uint16_t total_len = (packet[2] << 8) + packet[3];
  // cout << hex << "total_len = " << total_len << endl;
  uint16_t udp_len = (packet[ihl+4] << 8) + packet[ihl+5];
  // cout << hex << "udp_len = " << udp_len << endl;
  uint8_t command = packet[ihl+8];
  // cout << "command = "<< hex << (int)command << endl;
  uint8_t version = packet[ihl+9];
  uint16_t num = (udp_len - 8) / 20;  // number of RIP package
  if(total_len != len){
    // cout << "fuck you 1" << endl;
    return false;
  }
  if(version != 2){
    // cout << "fuck you 2" << endl;
    return false;
  }
  if(packet[ihl+10] != 0 || packet[ihl+11] != 0){    //zero
    // cout << "fuck you 3" << endl;
    return false;
  }
  uint8_t padding = ihl+8+4;
  // cout << "num = " << num << endl;
  for(uint16_t i = 0;i < num;i++){
    uint16_t family = (packet[padding+20*i+0]<<8) + packet[padding+20*i+1];
    if((command == 1 && family != 0x00) || (command == 2 && family != 0x0002)){
      // cout << padding+20*i+0 << padding+20*i+1 << endl;
      // cout << "command = " << command << " family = " << family << endl;
      // cout << "fuck you 4" << endl;
      return false;
    }
    if(command != 1 && command != 2)
      return false;
    uint16_t tag = (packet[padding+20*i+2]<<8) + packet[padding+20*i+3];
    if(tag != 0){
      // cout << "fuck you 5" << endl;
      return false;
    }
    uint32_t ipaddr = (packet[padding+20*i+4]<<24)+(packet[padding+20*i+5]<<16)+(packet[padding+20*i+6]<<8)+packet[padding+20*i+7];
    uint32_t mask = (packet[padding+20*i+8]<<24)+(packet[padding+20*i+9]<<16)+(packet[padding+20*i+10]<<8)+packet[padding+20*i+11];
    uint32_t nexthop = (packet[padding+20*i+12]<<24)+(packet[padding+20*i+13]<<16)+(packet[padding+20*i+14]<<8)+packet[padding+20*i+15];
    uint32_t metric = (packet[padding+20*i+16]<<24)+(packet[padding+20*i+17]<<16)+(packet[padding+20*i+18]<<8)+packet[padding+20*i+19];
    if((~mask+1)!=((~mask+1)&mask))
      return false;
    if(!((1 <= metric) && (metric <= 16))){
      // cout << "fuck you 6" << endl;
      return false;
    }
    //todo add mask check
    output->entries[i].addr = ntohl(ipaddr);
    output->entries[i].mask = ntohl(mask);
    output->entries[i].metric = ntohl(metric);
    output->entries[i].nexthop = ntohl(nexthop);
  }
  output->numEntries = num;
  output->command = command;
  return true;
  // packet[ihl+11+(num-1)*20+j]

}

/**
 * @brief 从 RipPacket 的数据结构构造出 RIP 协议的二进制格式
 * @param rip 一个 RipPacket 结构体
 * @param buffer 一个足够大的缓冲区，你要把 RIP 协议的数据写进去
 * @return 写入 buffer 的数据长度
 *
 * 在构造二进制格式的时候，你需要把 RipPacket 中没有保存的一些固定值补充上，包括
 * Version、Zero、Address Family 和 Route Tag 这四个字段 你写入 buffer
 * 的数据长度和返回值都应该是四个字节的 RIP 头，加上每项 20 字节。
 * 需要注意一些没有保存在 RipPacket 结构体内的数据的填写。
 */
uint32_t assemble(const RipPacket *rip, uint8_t *buffer) {
  // TODO:
  buffer[0] = rip->command;  //command
  // cout << "command = " << rip->command << endl;
  // cout << hex << buffer[0] << endl;
  buffer[1] = 2;  //version
  // cout << hex << buffer[1] << endl;
  // cout << "num = " << rip->numEntries << endl;
  for(int i = 0;i < rip->numEntries;i++){
    buffer[i*20 + 4] = 0;
    if(rip->command == 2) //family
      buffer[i*20 + 5] = 2;
    else if(rip->command == 1)
      buffer[i*20 + 5] = 0;
    buffer[i*20 + 6] = 0; //tag
    buffer[i*20 + 7] = 0;
    uint32_t addr = ntohl(rip->entries[i].addr);
    buffer[i*20 + 8] = addr >> 24; // ip address
    buffer[i*20 + 9] = (addr >> 16) & 0xffff;
    buffer[i*20 + 10] = (addr >> 8)  & 0xffff;
    buffer[i*20 + 11] = addr & 0xff;
    // cout << hex << buffer[i*20 + 8] << buffer[i*20 + 9] << buffer[i*20 + 10] << buffer[i*20 + 11] << endl;
    
    // cout << hex << (buffer[i*20 + 8] >> 4) << (buffer[i*20 + 8] & 0xf) << endl;
    // cout << hex << (buffer[i*20 + 9] >> 4) << (buffer[i*20 + 9] & 0xf) << endl;
    // cout << hex << (buffer[i*20 + 10] >> 4) << (buffer[i*20 + 10] & 0xf) << endl;
    // cout << hex << (buffer[i*20 + 11] >> 4) << (buffer[i*20 + 11] & 0xf) << endl;
    
    uint32_t mask = ntohl(rip->entries[i].mask);
    buffer[i*20 + 12] = mask >> 24; // mask
    buffer[i*20 + 13] = mask >> 16 & 0xff;
    buffer[i*20 + 14] = mask >> 8  & 0xff;
    buffer[i*20 + 15] = mask & 0xff;
    
    uint32_t nexthop = ntohl(rip->entries[i].nexthop);
    buffer[i*20 + 16] = nexthop >> 24; // next hop
    buffer[i*20 + 17] = nexthop >> 16 & 0xff;
    buffer[i*20 + 18] = nexthop >> 8  & 0xff;
    buffer[i*20 + 19] = nexthop & 0xff;

    uint32_t metric = ntohl(rip->entries[i].metric);
    buffer[i*20 + 20] = metric >> 24; // next hop
    buffer[i*20 + 21] = metric >> 16 & 0xff;
    buffer[i*20 + 22] = metric >> 8  & 0xff;
    buffer[i*20 + 23] = metric & 0xff;
    
  }
  return 4+rip->numEntries*20;
}
