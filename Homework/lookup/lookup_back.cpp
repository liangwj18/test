//#include "router.h"
//#include <stdint.h>
//#include <stdlib.h>
//#include <iostream>
//using namespace std;
//struct DictTree{
//  int edge;
//  RoutingTableEntry *e;
//  DictTree *sib;
//  DictTree *son;
//  // bool is = false;
//  DictTree(int en){
//    this->edge = en;
//  }
//  DictTree(){};
//};
//
//DictTree root;
//void insertt(RoutingTableEntry entry){
//  uint32_t addr = entry.addr;
//  uint32_t len  = entry.len / 4;
//  // int depth = 10;                           // real depth is depth's zero's number
//  // cout << "len  = " << len << endl;
//  // cout << "addr = " << addr << endl;
//  if(root.son == nullptr){
//    root.son = new DictTree(addr%10);
//    addr /= 10;
//  }
//  else{
//    DictTree *now = &root;
//    int depth = 0;
//    while(depth < len){
//      depth++;
//      DictTree *son = now->son;
//      int pos = addr % 10;
//      addr /= 10;
//      bool has = false;
//      DictTree *save;
//      while(son != nullptr){  //遍历子节点
//        if(son->edge == pos){
//          now = son;
//          has = true;
//          break;
//        }
//        else{
//          son = son->sib;
//          save = son;
//        }
//      }
//      if(son == nullptr)
//        son = save;
//      // if(now->e != nullptr)
//      //   break;
//      if(!has){             //如果这个分支没有开启,则直接往下添加节点
//        son->sib = new DictTree(pos);
//        now = son->sib;
//        son = now->son;
//        for(;depth < len; depth++){
//          pos = addr % 10;
//          addr /= 10;
//          son = new DictTree(pos);
//          now = son;
//        }
//        break;
//      }
//    }
//    if(now->son == nullptr){
//      // now->is = true;
//      now->e = &entry;
//    }else{
//      //如果不是最后一个节点，则不动
//    }
//  }
//}
//
//void deletee(RoutingTableEntry entry){
//
//}
///*
//  RoutingTable Entry 的定义如下：
//  typedef struct {
//    uint32_t addr; // 大端序，IPv4 地址
//    uint32_t len; // 小端序，前缀长度
//    uint32_t if_index; // 小端序，出端口编号
//    uint32_t nexthop; // 大端序，下一跳的 IPv4 地址
//  } RoutingTableEntry;
//
//  约定 addr 和 nexthop 以 **大端序** 存储。
//  这意味着 1.2.3.4 对应 0x04030201 而不是 0x01020304。
//  保证 addr 仅最低 len 位可能出现非零。
//  当 nexthop 为零时这是一条直连路由。
//  你可以在全局变量中把路由表以一定的数据结构格式保存下来。
//*/
//
///**
// * @brief 插入/删除一条路由表表项
// * @param insert 如果要插入则为 true ，要删除则为 false
// * @param entry 要插入/删除的表项
// *
// * 插入时如果已经存在一条 addr 和 len 都相同的表项，则替换掉原有的。
// * 删除时按照 addr 和 len **精确** 匹配。
// */
//void update(bool insert, RoutingTableEntry entry) {
//  // TODO:
//  if(insert)
//    insertt(entry);
//  else
//    deletee(entry); //todo
//}
//
///**
// * @brief 进行一次路由表的查询，按照最长前缀匹配原则
// * @param addr 需要查询的目标地址，大端序
// * @param nexthop 如果查询到目标，把表项的 nexthop 写入
// * @param if_index 如果查询到目标，把表项的 if_index 写入
// * @return 查到则返回 true ，没查到则返回 false
// */
//DictTree** ans;
//bool prefix_query(uint32_t addr, uint32_t *nexthop, uint32_t *if_index) {
//  // TODO:
//  ans = new DictTree *[10];
//  int size = 0;
//
//  DictTree *now = &root;
//  while(1){
//    if(now->e != nullptr){  //存储答案
//      ans[size] = now;
//      size++;
//    }
//    DictTree *son = now->son;
//    if(addr == 0){
//      *nexthop = ans[size-1]->e->nexthop;
//      *if_index = ans[size-1]->e->if_index;
//      delete []ans;
//      return true;
//    }
//    int pos = addr % 10;
//    addr /= 10;
//    while(son != nullptr){  //遍历子节点
//      if(son->edge == pos){
//        now = son;
//        break;
//      }
//      else{
//        son = son->sib;
//      }
//    }
//    if(son == nullptr){
//      *nexthop = 0;
//      *if_index = 0;
//      delete []ans;
//      return false;
//    }
//  }
//
//}