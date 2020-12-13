#include "router.h"
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <iostream>
using namespace std;
struct DictTree{ 
  RoutingTableEntry e;
  DictTree *next;
  DictTree *bef;
  DictTree(RoutingTableEntry en){
    this->e = en;
    this->next = nullptr;
    this->bef = nullptr;
  }
  DictTree(){};
};

DictTree *begin_ = new DictTree();
void insertt(RoutingTableEntry entry){
  DictTree *now = begin_;
  // cout << "in lookup.cpp 2.0 \n";
  while(now->next != nullptr){
    // cout << now->e.if_index << " ";
    now = now->next;
  }
  // cout << endl;
  
  // cout << "in lookup.cpp 2.1 \n";
  now->next = new DictTree(entry);
  // cout << "in lookup.cpp 2.2 \n";
  now->next->bef = now;
}

void deletee(uint32_t addr,uint32_t len){
  DictTree *now = begin_;
  while(now->next != nullptr){
    now = now->next;
    if(now->e.addr == addr && now->e.len == len){
      now->bef->next = now->next;
      if(now->next != nullptr)
        now->next->bef = now->bef;
      break;
    }
  }
  if(now->e.addr == addr && now->e.len == len){
    now->bef->next = now->next;
    if(now->next != nullptr)
      now->next->bef = now->bef;
  }
}
/*
  RoutingTable Entry 的定义如下：
  typedef struct {
    uint32_t addr; // 大端序，IPv4 地址
    uint32_t len; // 小端序，前缀长度
    uint32_t if_index; // 小端序，出端口编号
    uint32_t nexthop; // 大端序，下一跳的 IPv4 地址
  } RoutingTableEntry;

  约定 addr 和 nexthop 以 **大端序** 存储。
  这意味着 1.2.3.4 对应 0x04030201 而不是 0x01020304。
  保证 addr 仅最低 len 位可能出现非零。
  当 nexthop 为零时这是一条直连路由。
  你可以在全局变量中把路由表以一定的数据结构格式保存下来。
*/

/**
 * @brief 插入/删除一条路由表表项
 * @param insert 如果要插入则为 true ，要删除则为 false
 * @param entry 要插入/删除的表项
 *
 * 插入时如果已经存在一条 addr 和 len 都相同的表项，则替换掉原有的。
 * 删除时按照 addr 和 len **精确** 匹配。
 */
void update(bool insert, RoutingTableEntry entry) {
  // TODO:
  // cout << "in lookup.cpp 1 \n";
  if(insert){
    // cout << "in lookup.cpp 2 \n";
    insertt(entry);
    // cout << "in lookup.cpp 3 \n";
  }
  else{
    // cout << "in lookup.cpp 4 \n";
    uint32_t addr = entry.addr;
    uint32_t len = entry.len;
    deletee(addr,len); 
    // cout << "in lookup.cpp 5 \n";
  }
}

/**
 * @brief 进行一次路由表的查询，按照最长前缀匹配原则
 * @param addr 需要查询的目标地址，网络字节序
 * @param nexthop 如果查询到目标，把表项的 nexthop 写入
 * @param if_index 如果查询到目标，把表项的 if_index 写入
 * @return 查到则返回 true ，没查到则返回 false
 */
bool match(DictTree *now, uint32_t addr){
  uint32_t len = now->e.len;
  uint32_t save_addr = now->e.addr;
  if(len < 32)
    addr = addr % (1 << len);
  if(addr == save_addr)
    return true;
  else
    return false;
}

bool exact_match(DictTree *now, uint32_t addr, uint32_t mask) {
    uint32_t len = now->e.len;
    uint32_t save_addr = now->e.addr;
    // cout << "mask = " << mask << " " << " mask_len = " << ((1 << len) - 1) << endl;
    // printf("mask = 0x%08x\t((1 << len)-1) = 0x%08x\n", mask, ((1 << len)-1));
    if(addr == save_addr && ((1 << len)-1) == mask)
        return true;
    else
        return false;
}


bool exact_query(uint32_t addr, uint32_t mask, uint32_t &nexthop, uint32_t &if_index, uint32_t &metric) {          // 精确匹配
    int leng = 0;
    DictTree *now = begin_;
    int times = 0;
    while(now->next != nullptr){          // 找到精确匹配符合的表项
        now = now->next;
        // cout << times++ << " nexthop = " << now->e.nexthop << " if_index = " << now->e.if_index << " len = " << now->e.len << " metric = " << now->e.metric << endl;
        if(exact_match(now, addr, mask)){
            nexthop = now->e.nexthop;
            if_index = now->e.if_index;
            metric = now->e.metric;
            return true;
        }
    }
    return false;
}
int size = 10;
DictTree** ans = new DictTree*[size];

vector<RoutingTableEntry> get_all_entries(){
  DictTree *now = begin_;
  vector<RoutingTableEntry> vec;
  while(now->next != nullptr){
    now = now->next;
    vec.push_back(now->e);
  }
  return vec;
}

bool prefix_query(uint32_t addr, uint32_t *nexthop, uint32_t *if_index) {   // 最长前缀匹配
  // TODO:
  int leng = 0;
  DictTree *now = begin_;
  while(now->next != nullptr){          // 找到所有前缀匹配符合的表项
    now = now->next;
    if(match(now,addr)){
      ans[leng] = now;
      leng++;
    }
  }
  int max_id = -1;
  int max = 0;
  for(int i = 0;i < leng;i++){          // 从所有前缀匹配符合的表项中最长前缀匹配的表项
    if(max < ans[i]->e.len){
      max = ans[i]->e.len;
      max_id = i;
    }
  }
  if(leng > 0){
    *nexthop = ans[max_id]->e.nexthop;
    *if_index = ans[max_id]->e.if_index;
    return true;
  }

  *nexthop = 0;
  *if_index = 0;
  // // cout << "false !\n";
  return false;
}
