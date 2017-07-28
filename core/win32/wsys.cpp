// ------------------------------------------------
// File : wsys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      WSys derives from Sys to provide basic win32 functions such as starting threads.
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include <process.h>
#include <windows.h>
#include <time.h>
#include "win32/wsys.h"
#include "win32/wsocket.h"
#include "stats.h"
#include "peercast.h"
#include <sys/timeb.h>
#include <time.h>

// ---------------------------------
WSys::WSys(HWND w)
{
    stats.clear();

    rndGen.setSeed(getTime());

    mainWindow = w;
    WSAClientSocket::init();

    rndSeed = rnd();
}

// ---------------------------------
double WSys::getDTime()
{
   struct _timeb timebuffer;

   _ftime_s( &timebuffer );

   return (double)timebuffer.time+(((double)timebuffer.millitm)/1000);
}

// ---------------------------------
unsigned int WSys::getTime()
{
    time_t ltime;
    time( &ltime );
    return static_cast<unsigned>(ltime);
}

// ---------------------------------
ClientSocket *WSys::createSocket()
{
    return new WSAClientSocket();
}

// ---------------------------------
void WSys::waitThread(ThreadInfo *info, int timeout)
{
    switch(WaitForSingleObject((HANDLE)info->handle, timeout))
    {
      case WAIT_TIMEOUT:
          throw TimeoutException();
          break;
    }
}

// ---------------------------------
bool    WSys::startThread(ThreadInfo *info)
{
    info->active = true;

    typedef unsigned ( __stdcall *start_address )( void * );

    unsigned int threadID;
    info->handle = (unsigned int)_beginthreadex( NULL, 0, (start_address)info->func, info, 0, &threadID );

    if(info->handle == 0)
        return false;

    return true;
}

// ---------------------------------
void    WSys::sleep(int ms)
{
    Sleep(ms);
}

// ---------------------------------
void WSys::appMsg(long msg, long arg)
{
    //SendMessage(mainWindow,WM_USER,(WPARAM)msg,(LPARAM)arg);
}

// --------------------------------------------------
void WSys::callLocalURL(const char *str,int port)
{
    char cmd[512];
    snprintf(cmd,_countof(cmd),"http://localhost:%d/%s",port,str);
    ShellExecute(mainWindow, NULL, cmd, NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------
void WSys::getURL(const char *url)
{
    if (mainWindow)
        if (Sys::strnicmp(url,"http://",7) || Sys::strnicmp(url,"mailto:",7)) // XXX: ==0 �������Ă�H
            ShellExecute(mainWindow, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------
void WSys::exit()
{
    if (mainWindow)
        PostMessage(mainWindow,WM_CLOSE,0,0);
    else
        ::exit(0);
}

// --------------------------------------------------
void WSys::executeFile(const char *file)
{
    ShellExecute(NULL,"open",file,NULL,NULL,SW_SHOWNORMAL);
}

// --------------------------------------------------
//UPnP
unsigned int WSys::SetUPnP()
{
    CoInitialize(NULL);

    //ControlURL���擾�̏ꍇ�͎擾������
    if(strcmp(mControlURL.c_str(),"") == 0){
        if(!YMSSDPDiscover::Initialize() || !mSSDP.Open())
        {
            LOG_ERROR("UPnP Initialize Failed.");
            CoUninitialize();
            return 0;
        }

        LOG_NETWORK("UPnP Initialize OK.");

        mSSDP.Send("urn:schemas-upnp-org:service:WANPPPConnection:1");
        mSSDP.Send("urn:schemas-upnp-org:service:WANIPConnection:1");

        //��M�����݂�
        YMSSDPDiscover::LocationInfo info;

        int cnt = 0;

        while(1) {
            if (cnt > 30)
            {
                LOG_ERROR("UPnP Time Out.");
                CoUninitialize();
                return 0;
            }

            int ret = mSSDP.Recv(info,0);

            if(ret < 0)
            {
                //error
                LOG_ERROR("UPnP Error.");
                CoUninitialize();
                return 0;
            }
            else if(ret != YMSSDPDiscover::OK)
            {
                mSSDP.Send("urn:schemas-upnp-org:service:WANPPPConnection:1");
                mSSDP.Send("urn:schemas-upnp-org:service:WANIPConnection:1");
            }
            else
            {
                //����
                LOG_DEBUG("IGD Location:%s",info.mLocation.c_str());

                //controlURL�擾
                mControlURL = YMSSDPDiscover::GetControlURL(info.mLocation.c_str(),info.mST.c_str());
                if(mControlURL.empty())
                {
                    //���s
                    LOG_ERROR("UPnP ControlURL Scan Failed.");
                    CoUninitialize();
                    return 0;
                }
                mST = info.mST;

                //�擾OK
                LOG_NETWORK("UPnP ControlURL:%s",mControlURL.c_str());
                break;
            }

            Sleep(300);
            cnt++;
        }
    }

    //�|�[�g���
    char sPort[10] = "",sLanIP[64] = "";

    //���[�J��IP�擾
    Host lh(ClientSocket::getIP(NULL),0);
    lh.IPtoStr(sLanIP);

    YMSoapAction soap(mST.c_str(),"AddPortMapping");

    _itoa_s(servMgr->serverHost.port, sPort, _countof(sPort), 10);

    soap.SetParameter("NewRemoteHost","");
    soap.SetParameter("NewExternalPort",sPort);
    soap.SetParameter("NewProtocol","TCP");
    soap.SetParameter("NewInternalPort",sPort);
    soap.SetParameter("NewInternalClient",sLanIP);
    soap.SetParameter("NewEnabled","1");
    soap.SetParameter("NewPortMappingDescription","PeerCast");
    soap.SetParameter("NewLeaseDuration","0");

    if(soap.Invoke(mControlURL.c_str()) != 200)
    {
        //���s
        LOG_ERROR("UPnP AddPortMapping Error.");
        CoUninitialize();
        return 0;
    }

    //OK
    LOG_NETWORK("UPnP AddPortMapping OK.(%d Port)",servMgr->serverHost.port,sPort);
    CoUninitialize();

    return servMgr->serverHost.port;
}

// --------------------------------------------------
//UPnP
bool WSys::UnSetUPnP()
{
    //�|�[�g����폜
    char sPort[10] = "";

    YMSoapAction soap(mST.c_str(),"DeletePortMapping");

    _itoa_s(servMgr->EnableUPnPPort, sPort, _countof(sPort), 10);

    soap.SetParameter("NewRemoteHost","");
    soap.SetParameter("NewExternalPort",sPort);
    soap.SetParameter("NewProtocol","TCP");

    if(soap.Invoke(mControlURL.c_str()) != 200)
    {
        LOG_ERROR("UPnP DeletePortMapping Error.");
        return false;
    }

    //OK
    LOG_NETWORK("UPnP DeletePortMapping OK.(%d Port)",servMgr->EnableUPnPPort);

    return true;
}
