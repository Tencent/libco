/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#pragma once
#include <pthread.h>
#include <stdlib.h>

/*
invoke only once in the whole program
CoRoutineSetSpecificCallBack(CoRoutineGetSpecificFunc_t pfnGet,CoRoutineSetSpecificFunc_t pfnSet)

struct MyData_t
{
	int iValue;
	char szValue[100];
};
CO_ROUTINE_SPECIFIC( MyData_t,__routine );

int main()
{
	CoRoutineSetSpecificCallBack( co_getspecific,co_setspecific );

	__routine->iValue = 10;
	strcpy( __routine->szValue,"hello world" );

	return 0;
}
*/
extern int 	co_setspecific( pthread_key_t key, const void *value );
extern void *	co_getspecific( pthread_key_t key );

template <class T>
class clsCoSpecificData {
 public:
  T* operator->() {
    T* p = static_cast<T*>(co_getspecific(key_));
    if (p != NULL) {
      return p;
    }

    p = static_cast<T*>(calloc(1, sizeof(T)));
    int err = co_setspecific(key_, p);
    if (err == 0) {
      return p;
    }
    
    free(p);
    return NULL;
  }

  static clsCoSpecificData& GetInstance() {
    static clsCoSpecificData instance;
    return instance;
  }

 private:
  clsCoSpecificData() { pthread_key_create(&key_, NULL); }

  pthread_key_t key_;
};

#define CO_ROUTINE_SPECIFIC(Cls, var)  static clsCoSpecificData<Cls>& var = clsCoSpecificData<Cls>::GetInstance();
