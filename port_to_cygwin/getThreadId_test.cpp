//
// Created by dell-pc on 2018/4/6.
//
#include <iostream>
#include <dlfcn.h>

using namespace std;

static int (*getThreadId)(void) = (int (*)(void)) dlsym(RTLD_DEFAULT, "GetCurrentThreadId");

void *thread(void *args)
{
    char *s = (char *) args;
    cout << s << " tid=" << getThreadId() << " pthreadId="<< pthread_self() << endl;
}

int main() {
    int tid = getThreadId();

    cout << "tid=" << tid << endl;

    pthread_t ptht1, ptht2;
    pthread_create(&ptht1, NULL, thread, (void *) "thread1");
    pthread_create(&ptht2, NULL, thread, (void *) "tgread2");
}
