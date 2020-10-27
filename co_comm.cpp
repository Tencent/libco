#include "co_comm.h"

clsCoMutex::clsCoMutex() {
  m_ptCondSignal = co_cond_alloc();
  m_iWaitItemCnt = 0;
}

clsCoMutex::~clsCoMutex() { co_cond_free(m_ptCondSignal); }

void clsCoMutex::CoLock() {
  if (m_iWaitItemCnt > 0) {
    m_iWaitItemCnt++;
    co_cond_timedwait(m_ptCondSignal, -1);
  } else {
    m_iWaitItemCnt++;
  }
}

void clsCoMutex::CoUnLock() {
  m_iWaitItemCnt--;
  co_cond_signal(m_ptCondSignal);
}

