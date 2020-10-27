#pragma once

#include "co_routine.h"

class clsCoMutex {
 public:
  clsCoMutex();
  ~clsCoMutex();

  void CoLock();
  void CoUnLock();

 private:
  stCoCond_t* m_ptCondSignal;
  int m_iWaitItemCnt;
};

class clsSmartLock {
 public:
  clsSmartLock(clsCoMutex* m) {
    m_ptMutex = m;
    m_ptMutex->CoLock();
  }
  ~clsSmartLock() { m_ptMutex->CoUnLock(); }

 private:
  clsCoMutex* m_ptMutex;
};

