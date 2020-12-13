#include "rip.h"
#include "router.h"
#include "router_hal.h"
#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <vector>
using namespace std;

extern bool validateIPChecksum(uint8_t *packet, size_t len);
extern bool validateICMPChecksum(uint8_t *packet, size_t len);
extern uint32_t getIPChecksum(uint8_t *packet);
extern uint32_t getICMPChecksum(uint8_t *packet);
extern void update(bool insert, RoutingTableEntry entry);
extern bool prefix_query(uint32_t addr, uint32_t *nexthop, uint32_t *if_index);
extern bool exact_query(uint32_t addr, uint32_t mask, uint32_t &nexthop, uint32_t &if_index, uint32_t &metric);
extern bool forward(uint8_t *packet, size_t len);
extern bool disassemble(const uint8_t *packet, uint32_t len, RipPacket *output);
extern uint32_t assemble(const RipPacket *rip, uint8_t *buffer);
extern vector<RoutingTableEntry> get_all_entries();

uint8_t packet[2048];
uint8_t output[2048];

// for online experiment, don't change
#ifdef ROUTER_R1
// 0: 192.168.1.1
// 1: 192.168.3.1
// 2: 192.168.6.1
// 3: 192.168.7.1
const in_addr_t addrs[N_IFACE_ON_BOARD] = {0x0101a8c0, 0x0103a8c0, 0x0106a8c0,
                                           0x0107a8c0};
#elif defined(ROUTER_R2)
// 0: 192.168.3.2
// 1: 192.168.4.1
// 2: 192.168.8.1
// 3: 192.168.9.1
const in_addr_t addrs[N_IFACE_ON_BOARD] = {0x0203a8c0, 0x0104a8c0, 0x0108a8c0,
                                           0x0109a8c0};
#elif defined(ROUTER_R3)
// 0: 192.168.4.2
// 1: 192.168.5.2
// 2: 192.168.10.1
// 3: 192.168.11.1
const in_addr_t addrs[N_IFACE_ON_BOARD] = {0x0204a8c0, 0x0205a8c0, 0x010aa8c0,
                                           0x010ba8c0};
#else

// 自己调试用，你可以按需进行修改，注意字节序
// 0: 10.0.0.1
// 1: 10.0.1.1
// 2: 10.0.2.1
// 3: 10.0.3.1
in_addr_t addrs[N_IFACE_ON_BOARD] = {0x0100000a, 0x0101000a, 0x0102000a,
                                     0x0103000a};
#endif
uint32_t min(uint32_t a, uint32_t b){
    if(a > b)
        return b;
    else
        return a;
}

int main(int argc, char *argv[]) {
  // 0a.
  int res = HAL_Init(1, addrs);
  cout<<"FUCK1\n";
  if (res < 0) {
   // cout << "res = " << res << endl;
    return res;
  }
  cout<<"FUCK 1.1 \n";

  // 0b. Add direct routes
  // For example:
  // 10.0.0.0/24 if 0
  // 10.0.1.0/24 if 1
  // 10.0.2.0/24 if 2
  // 10.0.3.0/24 if 3
  for (uint32_t i = 0; i < N_IFACE_ON_BOARD; i++) {
    RoutingTableEntry entry = {
        .addr = addrs[i] & 0x00FFFFFF, // network byte order
        .len = 24,                     // host byte order
        .if_index = i,                 // host byte order
        .nexthop = 0,                   // network byte order, means direct
        .metric = 0x01000000,
    };
    cout<<"FUCK 1.2 \n";
    update(true, entry);
    cout<<"FUCK 1.3 \n";
  }

  uint64_t last_time = 0;
  cout<<"FUCK2\n";
  while (1) {
    uint64_t time = HAL_GetTicks();
    // the RFC says 30s interval,
    // but for faster convergence, use 5s here
    if (time > last_time + 5 * 1000) {
      // ref. RFC 2453 Section 3.8
      printf("5s Timer\n");
      // HINT: print complete routing table to stdout/stderr for debugging
      // TODO: done ? send complete routing table to every interface
      for (int i = 0; i < N_IFACE_ON_BOARD; i++) {
        // construct rip response
        // do the mostly same thing as step 3a.3
        // except that dst_ip is RIP multicast IP 224.0.0.9
        // and dst_mac is RIP multicast MAC 01:00:5e:00:00:09
          // TODO: fill resp
          // implement split horizon with poisoned reverse
          // ref. RFC 2453 Section 3.4.3
          // fill IP headers
          struct ip *ip_header = (struct ip *)output;
          ip_header->ip_hl = 5;
          ip_header->ip_v = 4;
          // TODO: done; set tos = 0, id = 0, off = 0, ttl = 1, p = 17(udp), dst and src
          ip_header->ip_tos = 0;
          ip_header->ip_id = 0;
          ip_header->ip_off = 0;
          ip_header->ip_ttl = 1;
          ip_header->ip_p = 17;
          ip_header->ip_src.s_addr = addrs[i];
          ip_header->ip_dst.s_addr = (9 << 24) + 224;
          // fill UDP headers
          struct udphdr *udpHeader = (struct udphdr *)&output[20];
          // src port = 520
          udpHeader->uh_sport = htons(520);
          // dst port = 520
          udpHeader->uh_dport = htons(520);
          // assemble RIP
          vector<RoutingTableEntry> vec;
          vec = get_all_entries();
          int routerEntryTableNum = vec.size();
          // cout << "vec size = " << vec.size() << endl;
          for(int j = 0;j < routerEntryTableNum;j += 25){
              RipPacket resp;
              resp.numEntries = 0;
              resp.command = 2; // response
              int size = min(j+25, routerEntryTableNum);
              // cout << "size = " << size << endl;
              for(int k = 0;k < size;k++){
                  resp.entries[k].addr = vec[j+k].addr;
                  resp.entries[k].mask = (1 << vec[j+k].len) - 1;   // todo : may wrong
                  resp.entries[k].metric = vec[j+k].metric;      // infinity
                  resp.entries[k].nexthop = vec[j+k].nexthop;
                  resp.numEntries++;
              }
              udpHeader->len = htons(4+resp.numEntries*20 + 8);     // length of rip + length of udpHeader(8)
              udpHeader->uh_sum=0;
              uint32_t rip_len = assemble(&resp, &output[20 + 8]);
              ip_header->ip_len = htons(rip_len+20+8);
              ip_header->ip_sum = ntohs(getIPChecksum((uint8_t *)ip_header));
              macaddr_t mac_addr;
              mac_addr[5] = 0x09;
              mac_addr[4] = 0x00;
              mac_addr[3] = 0x00;
              mac_addr[2] = 0x5e;
              mac_addr[1] = 0x00;
              mac_addr[0] = 0x01;
              HAL_SendIPPacket(i, output, rip_len + 20 + 8, mac_addr);
          }
          
          // // if you don't want to calculate udp checksum, set it to zero
          // udpHeader->uh_sum = 0;
          // // send it back
          // macaddr_t dst_mac =  //'01:00:5e:00:00:09'
          // HAL_SendIPPacket(if_index, output, rip_len + 20 + 8, dst_mac);
      }
      last_time = time;
    }

    int mask = (1 << N_IFACE_ON_BOARD) - 1;
    macaddr_t src_mac;
    macaddr_t dst_mac;
    int if_index;
    res = HAL_ReceiveIPPacket(mask, packet, sizeof(packet), src_mac, dst_mac,
                              1000, &if_index);
    if (res == HAL_ERR_EOF) {
      break;
    } else if (res < 0) {
      return res;
    } else if (res == 0) {
      // Timeout
      continue;
    } else if (res > sizeof(packet)) {
      // packet is truncated, ignore it
      continue;
    }

    // 1. validate
    if (!validateIPChecksum(packet, res)) {
      printf("Invalid IP Checksum\n");
      // drop if ip checksum invalid
      continue;
    }
    in_addr_t src_addr, dst_addr;
    // TODO: done; extract src_addr and dst_addr from packet (big endian)
      src_addr = packet[12] + (packet[13] << 8) + (packet[14] << 16) + (packet[15] << 24);
      dst_addr = packet[16] + (packet[17] << 8) + (packet[18] << 16) + (packet[19] << 24);
    // 2. check whether dst is me
    bool dst_is_me = false;
    for (int i = 0; i < N_IFACE_ON_BOARD; i++) {
        if (memcmp(&dst_addr, &addrs[i], sizeof(in_addr_t)) == 0) {
            dst_is_me = true;
            break;
        }
    }
    // TODO: handle rip multicast address(224.0.0.9)
    if (dst_addr==(9<<24)+224)
        dst_is_me=true;

    if (dst_is_me) {
      cout << "dst_is_me \n";
      // 3a.1
      RipPacket rip;
      // check and validate
      if (disassemble(packet, res, &rip)) {
        if (rip.command == 1) {                 // rip.command := 1 : request , 2 : response
          cout<<"receive rip request \n";
          // 3a.3 request, ref. RFC 2453 Section 3.9.1
          // only need to respond to whole table requests in the lab

          RipPacket resp;
          // TODO: fill resp
          // implement split horizon with poisoned reverse
          // ref. RFC 2453 Section 3.4.3

          // fill IP headers
          struct ip *ip_header = (struct ip *)output;
          ip_header->ip_hl = 5;
          ip_header->ip_v = 4;
          // TODO: done; set tos = 0, id = 0, off = 0, ttl = 1, p = 17(udp), dst and src
          ip_header->ip_tos = 0;
          ip_header->ip_id = 0;
          ip_header->ip_off = 0;
          ip_header->ip_ttl = 1;
          ip_header->ip_p = 17;
          ip_header->ip_dst.s_addr=src_addr;
          ip_header->ip_src.s_addr=dst_addr;

          // fill UDP headers
          struct udphdr *udpHeader = (struct udphdr *)&output[20];
          // src port = 520
          udpHeader->uh_sport = htons(520);
          // dst port = 520
          udpHeader->uh_dport = htons(520);
          // TODO: done; udp length
          udpHeader->uh_ulen = htons(4+rip.numEntries*20 + 8);     // length of rip + length of udpHeader(8)

          // assemble RIP
          vector<RoutingTableEntry> vec;
          vec = get_all_entries();
          int routerNum = vec.size();
          resp.command = 2; // response
          for(int j = 0;j < resp.numEntries;j += 25){
              int size = min(j+25, resp.numEntries);
              resp.numEntries = 0;
              for(int k = 0;k < size;k++){
                  resp.entries[k].addr = vec[j+k].addr;
                  resp.entries[k].mask = (1 << vec[j+k].len) - 1;
                  resp.entries[k].metric = vec[j+k].metric;      // infinity
                  resp.entries[k].nexthop = rip.entries[j].nexthop;
                  resp.numEntries++;
                  if (vec[j+k].if_index==if_index)
                      resp.entries[k].metric=0x10000000;
                  resp.numEntries++;
              }
              udpHeader->len = htons(4+resp.numEntries*20 + 8);     // length of rip + length of udpHeader(8)
              udpHeader->uh_sum=0;
              uint32_t rip_len = assemble(&resp, &output[20 + 8]);
              ip_header->ip_len = htons(rip_len+20+8);
              ip_header->ip_sum = ntohs(getIPChecksum((uint8_t *)ip_header));
              HAL_SendIPPacket(if_index, output, rip_len + 20 + 8, src_mac);
          }

          uint32_t rip_len = assemble(&resp, &output[20 + 8]);

          // TODO: done; checksum calculation for ip and udp
          // if you don't want to calculate udp checksum, set it to zero
          udpHeader->uh_sum = 0;

          // send it back
          HAL_SendIPPacket(if_index, output, rip_len + 20 + 8, src_mac);
        } else {        // 接收到一个 response 数据包
          // 3a.2 response, ref. RFC 2453 Section 3.9.2
          // TODO: done; update routing table
            // src_port = htons(packet[20] << 8 + packet[21]);
            // dst_port = htons(packet[22] << 8 + packet[23]);
            // if src_port != 520 or dst_port != 520:      // 如果不是 rip 端口，则忽略掉这个响应
            //     continue;
            cout<<"receive rip response !!! \n";
            for(int i = 0;i < rip.numEntries; i++) {
                uint32_t nexthop = 0;
                uint32_t if_index = 0;
                uint32_t len = 32;
                uint32_t metric = 0;
                bool has_entry = exact_query(rip.entries[i].addr, rip.entries[i].mask, nexthop, if_index, metric);  // 精确匹配
                // cout << has_entry << " nexthop = " << nexthop << " if_index = " << if_index << " len = " << len << " metric = " << metric << endl;
                metric = min(htons(htons(metric) + 1), htons(16));
                if (has_entry) {      // 改变这个表项
                    // todo : 更新 nexthop 和 if_index 和 len
                    if (nexthop == src_addr){           // 毒性反转
                        if (metric == htonl(16)) {  // todo : may wrong
                            cout << "poison me\n";
                            RoutingTableEntry entry(src_addr, len, if_index, nexthop, metric);  // mask is zero , but may wrong
                            update(false, entry);
//                            update(true, entry);
                        }
                    } else if (rip.entries[i].metric < metric) {
                        cout << "change entry==============================\n";
                        RoutingTableEntry entry(rip.entries[i].addr, 32-__builtin_clz(rip.entries[i].mask), if_index, rip.entries[i].nexthop, rip.entries[i].metric);
                        update(false, entry);
                        update(true, entry);
                    }
                } else {             // 添加一个新表项
                    cout << "add entry\n";
                    // todo : 更新 nexthop 和 if_index 和 len
                    RoutingTableEntry entry(rip.entries[i].addr, 32-__builtin_clz(rip.entries[i].mask), if_index, rip.entries[i].nexthop, ntohl(htonl(rip.entries[i].metric)+1));
                    update(true, entry);
                }
            }
          // new metric = ?
          // update metric, if_index, nexthop
          // HINT: handle nexthop = 0 case
          // HINT: what is missing from RoutingTableEntry?
          // you might want to use `prefix_query` and `update`, but beware of
          // the difference between exact match and longest prefix match.
          // optional: triggered updates ref. RFC 2453 Section 3.10.1
        }
      } else {
          cout<<"receive icmp resquest \n";
        // not a rip packet
        // handle icmp echo request packet
        // TODO: how to determine?
//        if (false) {
          uint8_t type = packet[20];
          if (type == 8) {
          // construct icmp echo reply
          // reply is mostly the same as request,
          for (int i=0;i<res;++i)
              output[i]=packet[i];
          struct ip *ip_header = (struct ip *)output;
          ip_header->ip_ttl = 64;
          ip_header->ip_src.s_addr = dst_addr;
          ip_header->ip_dst.s_addr = src_addr;
          // you need to:
          // 1. swap src ip addr and dst ip addr
          // 2. change icmp `type` in header
            struct icmphdr *icmp_header = (struct icmphdr *)&output[20];
            // icmp type = Destination Unreachable
            icmp_header->type = ICMP_ECHOREPLY;
          // 3. set ttl to 64
          // 4. re-calculate icmp checksum and ip checksum
            ip_header->ip_sum = ntohs(getIPChecksum((uint8_t *)ip_header));
            icmp_header->checksum = ntohs(getICMPChecksum((uint8_t *)icmp_header));
          // 5. send icmp packet
            HAL_SendIPPacket(if_index, output, res, dst_mac);
        }
      }
    } else {
      cout << "dst_is_not_me \n";
      cout<<"broadcast packet 1\n";
      // 3b.1 dst is not me
      // check ttl
      uint8_t ttl = packet[8];
      if (ttl <= 1) {
        // send icmp time to live exceeded to src addr
        // fill IP header
        struct ip *ip_header = (struct ip *)output;
        ip_header->ip_hl = 5;
        ip_header->ip_v = 4;
        // TODO: done; set tos = 0, id = 0, off = 0, ttl = 64, p = 1(icmp), src and dst
        ip_header->ip_tos = 0;
        ip_header->ip_id = 0;
        ip_header->ip_off = 0;
        ip_header->ip_ttl = 64;
        ip_header->ip_p = 1;
        ip_header->ip_src.s_addr = dst_addr;
        ip_header->ip_dst.s_addr = src_addr;
        ip_header->ip_len = htons(56);
        ip_header->ip_sum = htons(getIPChecksum((uint8_t *)ip_header));

        // fill icmp header
        struct icmphdr *icmp_header = (struct icmphdr *)&output[20];
        // icmp type = Time Exceeded
        icmp_header->type = ICMP_TIME_EXCEEDED;
        // TODO: icmp code = 0
        icmp_header->code = 0;
        // TODO: fill unused fields with zero
        icmp_header->un.echo.id=0;
        icmp_header->un.echo.sequence=0;
        for (int i=28;i<56;++i)
            output[i]=packet[i-28];
        // TODO: append "ip header and first 8 bytes of the original payload"
        // TODO: calculate icmp checksum and ip checksum
        icmp_header->checksum = getICMPChecksum((uint8_t *)icmp_header);
        // TODO: send icmp packet
        HAL_SendIPPacket(if_index, output, 56, dst_mac);
      } else {
        // forward
        // beware of endianness
        uint32_t nexthop, dest_if;
        if (prefix_query(dst_addr, &nexthop, &dest_if)) {
          // found
          macaddr_t dest_mac;
          // direct routing
          if (nexthop == 0) {
            nexthop = dst_addr;
          }
          if (HAL_ArpGetMacAddress(dest_if, nexthop, dest_mac) == 0) {
            // found
            memcpy(output, packet, res);
            // update ttl and checksum
            forward(output, res);
            struct ip *ip_header = (struct ip *)output;
            ip_header->ip_ttl--;
            ip_header->ip_sum=ntohs(getIPChecksum((uint8_t*)ip_header));
            HAL_SendIPPacket(dest_if, output, res, dest_mac);
          } else {
            // not found
            // you can drop it
            printf("ARP not found for nexthop %x\n", nexthop);
          }
        } else {
          cout<<"broadcast packet 2\n";
          // not found
          // send ICMP Destination Network Unreachable
          printf("IP not found in routing table for src %x dst %x\n", src_addr, dst_addr);
          // send icmp destination net unreachable to src addr
          // fill IP header
          struct ip *ip_header = (struct ip *)output;
          ip_header->ip_hl = 5;
          ip_header->ip_v = 4;
          // TODO: done; set tos = 0, id = 0, off = 0, ttl = 64, p = 1(icmp), src and dst
          ip_header->ip_tos = 0;
          ip_header->ip_id = 0;
          ip_header->ip_off = 0;
          ip_header->ip_ttl = 64;
          ip_header->ip_p = 1;
          ip_header->ip_src.s_addr = dst_addr;
          ip_header->ip_dst.s_addr = src_addr;
          ip_header->ip_len=htons(56);

          // fill icmp header
          struct icmphdr *icmp_header = (struct icmphdr *)&output[20];
          // icmp type = Destination Unreachable
          icmp_header->type = ICMP_DEST_UNREACH;
          // TODO: icmp code = Destination Network Unreachable
          icmp_header->code = ICMP_NET_UNREACH;
          // TODO: fill unused fields with zero
          icmp_header->un.echo.id=0;
          icmp_header->un.echo.sequence=0;

          for (int i=28;i<56;++i)
              output[i]=packet[i-28];
          // TODO: append "ip header and first 8 bytes of the original payload"
          // TODO: calculate icmp checksum and ip checksum
          ip_header->ip_sum = getIPChecksum((uint8_t *)ip_header);
          icmp_header->checksum = getICMPChecksum((uint8_t *)icmp_header);
          // TODO: send icmp packet
          HAL_SendIPPacket(dest_if, output, 56, dst_mac);
        }
      }
    }
  }
  return 0;
}
