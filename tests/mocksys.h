﻿#ifndef MOCKSYS_H
#define MOCKSYS_H

#include "sys.h"

class MockSys : public Sys
{
public:
    MockSys()
    {
    }

    class ClientSocket* createSocket() override
    {
        return NULL;
    }

    bool startThread(class ThreadInfo*) override
    {
        return false;
    }

    bool startWaitableThread(class ThreadInfo*) override
    {
        return false;
    }

    void sleep(int) override
    {
    }

    unsigned int getTime() override
    {
        return 0;
    }

    double getDTime() override
    {
        return 0;
    }

    unsigned int rnd() override
    {
        return 123456789;
    }

    // URLをブラウザやメーラで開く。
    void getURL(const char*) override
    {
    }

    void exit() override
    {
    }

    bool hasGUI() override
    {
        return false;
    }

    // ローカルサーバーのURLを開く。
    void callLocalURL(const char*, int) override
    {
    }

    // ファイルを開く。
    void executeFile(const char*) override
    {
    }

    void waitThread(ThreadInfo*) override
    {
    }

    unsigned int SetUPnP() override
    {
        return 0;
    }
};

#endif
