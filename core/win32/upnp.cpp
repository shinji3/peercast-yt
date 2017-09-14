// ------------------------------------------------------------------------------
// UPnPクラス
//
// 下記ホームページで公開されているソースを利用させていただきました。
// βえんどるふぃん -平穏で不穏な日々-様 URL:http://www.bosuke.mine.nu/blog/ 
//
// ------------------------------------------------------------------------------
#include "upnp.h"
#include "sys.h"
#include "xml.h"
#include "http.h"
#include "socket.h"
#include "uri.h"
#include <malloc.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <strstream>
#include <sstream>
#include <Wininet.h>


namespace
{
    const char* UPNP_GROUP = "239.255.255.250";
    const unsigned short UPNP_PORT = 1900;
    const DWORD BUFFER_SIZE = 2048;
}

//////////////////////////////////////////////////
//winsock初期化
//////////////////////////////////////////////////

YMSSDPDiscover::YMSSDPDiscover()
    :mSocket(INVALID_SOCKET), mLastError(0)
{
}

YMSSDPDiscover::~YMSSDPDiscover()
{
    Close();
}

//////////////////////////////////////////////////
// ソケットオープンとクローズ
//////////////////////////////////////////////////

bool YMSSDPDiscover::Open()
{
    mLastError = 0;
    if (mSocket != INVALID_SOCKET)
    {
        return true;
    }

    //UDPソケットつくりーの
    mSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mSocket == INVALID_SOCKET)
    {
        mLastError = WSAGetLastError();
        return false;
    }

    //空いているポートにbind
    sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    unsigned short port = 20000;
    while (1)
    {
        addr.sin_port = htons(port);
        if (bind(mSocket, (sockaddr*)&addr, sizeof(addr)) == 0)
        {
            //成功
            break;
        }
        int err = WSAGetLastError();
        if (err == WSAEADDRINUSE)
        {
            //使用中
            ++port;
        }
        else
        {
            //エラー
            mLastError = err;
            closesocket(mSocket);
            mSocket = INVALID_SOCKET;
            return false;
        }
    }

    //TTL・ノンブロッキング設定
    const char TTL = 4;
    unsigned long flags = 1;
    setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_TTL, &TTL, sizeof(TTL));
    ioctlsocket(mSocket, FIONBIO, &flags);

    //イベントオブジェクト作成
    mEvent = WSACreateEvent();

    return true;
}

void YMSSDPDiscover::Close()
{
    mLastError = 0;
    if (mSocket != INVALID_SOCKET)
    {
        closesocket(mSocket);
        mSocket = INVALID_SOCKET;
        mLastError = WSAGetLastError();
        WSACloseEvent(mEvent);
    }
    mSTList.clear();
}

//////////////////////////////////////////////////
// M-SEARCH送信
//////////////////////////////////////////////////

bool YMSSDPDiscover::Send(const char* st)
{
    mLastError = 0;

    //IPアドレスリストを取得する
    DWORD bufferSize = 0;
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, NULL, 0, &bufferSize, NULL, NULL);
    if (bufferSize == 0)
    {
        mLastError = WSAGetLastError();
        closesocket(s);
        return false;
    }
    SOCKET_ADDRESS_LIST* list = (SOCKET_ADDRESS_LIST*)_alloca(bufferSize);
    if (WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, list, bufferSize, &bufferSize, NULL, NULL) == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        closesocket(s);
        return false;
    }
    closesocket(s);

    //送信するデータを作成
    std::string data = "M-SEARCH * HTTP/1.1\r\nMX: 3\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nST: ";
    data += st;
    data += "\r\n\r\n";

    //データをぎゃんぎゃんマルチキャストしていく
    sockaddr_in dest_addr;
    ZeroMemory(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UPNP_PORT);
    inet_pton(AF_INET, UPNP_GROUP, &dest_addr.sin_addr);

    in_addr interface_addr;
    ZeroMemory(&interface_addr, sizeof(interface_addr));

    bool ok = false;
    for (int i = 0; i < list->iAddressCount; ++i)
    {
        interface_addr.s_addr = ((sockaddr_in*)list->Address[i].lpSockaddr)->sin_addr.s_addr;
        if (setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&interface_addr, sizeof(interface_addr)) == 0)
        {
            if (sendto(mSocket, data.data(), static_cast<int>(data.length()), 0, (sockaddr*)&dest_addr, sizeof(dest_addr)) > 0)
            {
                ok = true;
            }
            else
            {
                mLastError = WSAGetLastError();
            }
        }
        else
        {
            mLastError = WSAGetLastError();
        }
    }

    //１つでもうまく送ることができたら良しとする
    if (ok)
    {
        mLastError = 0;

        //STのリストにぶち込む
        if (std::find(mSTList.begin(), mSTList.end(), st) == mSTList.end())
        {
            mSTList.push_back(st);
        }
    }
    return ok;
}


//////////////////////////////////////////////////
// データ受信
//////////////////////////////////////////////////
int YMSSDPDiscover::Recv(LocationInfo& info, DWORD timeout)
{

    mLastError = 0;

    //指定ミリセカンドデータが受信できるまで待つ
    WSAResetEvent(mEvent);
    if (WSAEventSelect(mSocket, mEvent, FD_READ) == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        return Error;
    }

    WSANETWORKEVENTS events;
    DWORD ret = WSAWaitForMultipleEvents(1, &mEvent, FALSE, timeout, FALSE);
    if (ret == WSA_WAIT_TIMEOUT)
    {
        return Timeout;
    }
    if (ret == WSA_WAIT_FAILED
        || WSAEnumNetworkEvents(mSocket, mEvent, &events) != 0
        || !(events.lNetworkEvents & FD_READ))
    {
        mLastError = WSAGetLastError();
        return Error;
    }

    //受信
    char buffer[BUFFER_SIZE + 1];
    sockaddr_in addr;
    int addrLen = sizeof(addr);
    int readLen = recvfrom(mSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&addr, &addrLen);
    if (readLen == SOCKET_ERROR)
    {
        mLastError = WSAGetLastError();
        return Error;
    }
    buffer[readLen] = '\0';

    //パース
    LocationInfo tempInfo;
    std::istrstream input(buffer);
    std::string line;
    bool first = true;
    while (std::getline(input, line))
    {

        //ATLTRACE("recv:%s\n",line.c_str());

        if (first)
        {
            //200以外はダメポ
            std::istringstream responseLine(line);
            std::string dummy;
            int code = 0;
            responseLine >> dummy >> code;
            if (code != 200)
            {
                return HttpError;
            }
            first = false;
        }
        else
        {
            //フィールドと値にわける
            int index = static_cast<int>(line.find(':'));
            if (index == std::string::npos)
            {
                continue;
            }
            std::string field = line.substr(0, index);
            if ((index = static_cast<int>(line.find_first_not_of(' ', index + 1))) != std::string::npos)
            {
                std::string value = line.substr(index);
                if (!value.empty())
                {
                    if (*value.rbegin() == '\r')
                    {
                        value = value.substr(0, value.length() - 1);
                    }
                    if (Sys::stricmp(field.c_str(), "ST") == 0)
                    {
                        tempInfo.mST = value;
                    }
                    else if (Sys::stricmp(field.c_str(), "Location") == 0)
                    {
                        tempInfo.mLocation = value;
                    }
                }
            }
        }
    }

    //値をチェック
    if (tempInfo.mST.empty()
        || tempInfo.mLocation.empty()
        || std::find(mSTList.begin(), mSTList.end(), tempInfo.mST) == mSTList.end())
    {
        return NoData;
    }

    //OK
    info = tempInfo;

    return OK;
}

//////////////////////////////////////////////////
// controlURLの取得
//////////////////////////////////////////////////
std::string YMSSDPDiscover::GetControlURL(const char* location, const char* st)
{
    std::string result;
    std::string baseURL;
    std::string relativeURL;

    URI feed(location);
    if (!feed.isValid()) {
        LOG_ERROR("invalid URL (%s)", location);
        return result;
    }
    if (feed.scheme() != "http") {
        LOG_ERROR("unsupported protocol (%s)", location);
        return result;
    }

    Host host;
    host.fromStrName(feed.host().c_str(), feed.port());
    if (host.ip == 0) {
        LOG_ERROR("Could not resolve %s", feed.host().c_str());
        return result;
    }

    std::unique_ptr<ClientSocket> rsock(sys->createSocket());
    WriteBufferedStream brsock(&*rsock);

    try {
        LOG_TRACE("Connecting to %s ...", feed.host().c_str());
        rsock->open(host);
        rsock->connect();

        HTTP rhttp(brsock);

        auto request_line = "GET " + feed.path() + " HTTP/1.0";
        LOG_TRACE("Request line to %s: %s", feed.host().c_str(), request_line.c_str());

        rhttp.writeLineF("%s", request_line.c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_HOST, feed.host().c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
        rhttp.writeLine("");

        auto code = rhttp.readResponse();
        if (code != 200) {
            LOG_ERROR("%s: status code %d", feed.host().c_str(), code);
            return result;
        }

        while (rhttp.nextHeader())
            ;

        std::string text;
        char line[1024];

        try {
            while (rhttp.readLine(line, 1024)) {
                text += line;
                text += '\n';
            }
        }
        catch (EOFException&) {
            // end of body reached.
        }

        MemoryStream xm((void*)text.c_str(), static_cast<int>(text.size()));
        XML xml;
        xml.read(xm);
        
        XML::Node *controlURLNode = xml.findNode("serviceType", st);

        if (controlURLNode)
            controlURLNode = controlURLNode->parent;
        else
            return result;

        if (controlURLNode)
            controlURLNode = controlURLNode->findNode("controlURL");
        else
            return result;

        if (controlURLNode)
            relativeURL = controlURLNode->getContent();
        else
            return result;

        XML::Node *baseURLNode = xml.findNode("URLBase");

        if (baseURLNode)
            baseURL = baseURLNode->getContent();
        else
            baseURL = location;
    }
    catch (StreamException& e) {
        LOG_ERROR("%s", e.msg);
        return result;
    }

    //URLを結合
    DWORD size = 0;
    InternetCombineUrl(baseURL.c_str(), relativeURL.c_str(), NULL, &size, 0);
    char* buf = (char*)_alloca(size);
    InternetCombineUrl(baseURL.c_str(), relativeURL.c_str(), buf, &size, 0);
    result = buf;

    return result;
}




YMSoapAction::YMSoapAction(const char* serviceType, const char* actionName)
    :mServiceType(serviceType), mActionName(actionName)
{
}

void YMSoapAction::SetParameter(const char* field, const char* value)
{
    //パラメーターをセット
    mParameter[field] = value;
}

void YMSoapAction::ResetParameter()
{
    //パラメータクリア
    mParameter.clear();
}

int YMSoapAction::Invoke(const char* url)
{
    URI feed(url);
    if (!feed.isValid()) {
        LOG_ERROR("invalid URL (%s)", url);
        return -1;
    }
    if (feed.scheme() != "http") {
        LOG_ERROR("unsupported protocol (%s)", url);
        return -1;
    }

    Host host;
    host.fromStrName(feed.host().c_str(), feed.port());
    if (host.ip == 0) {
        LOG_ERROR("Could not resolve %s", feed.host().c_str());
        return -1;
    }

    std::unique_ptr<ClientSocket> rsock(sys->createSocket());
    WriteBufferedStream brsock(&*rsock);

    try {
        LOG_TRACE("Connecting to %s ...", feed.host().c_str());
        rsock->open(host);
        rsock->connect();

        HTTP rhttp(brsock);

        auto request_line = "POST " + feed.path() + " HTTP/1.0";
        LOG_TRACE("Request line to %s: %s", feed.host().c_str(), request_line.c_str());

        rhttp.writeLineF("%s", request_line.c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_HOST, feed.host().c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
        rhttp.writeLineF("SOAPAction: %s#%s", mServiceType.c_str(), mActionName.c_str());
        rhttp.writeLine("");

        rhttp.writeString("<?xml version=\"1.0\"?>");
        rhttp.writeString("<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">");
        rhttp.writeString("<SOAP-ENV:Body>");
        rhttp.writeStringF("<m:%s xmlns:m=\"%s\">", mActionName.c_str(), mServiceType.c_str());

        for (Parameters::const_iterator p = mParameter.begin(); p != mParameter.end(); ++p)
        {
            rhttp.writeStringF("<%s>%s</%s>", p->first.c_str(), p->second.c_str(), p->first.c_str());
        }

        rhttp.writeStringF("</m:%s>", mActionName.c_str());
        rhttp.writeString("</SOAP-ENV:Body>");
        rhttp.writeString("</SOAP-ENV:Envelope>");

        auto code = rhttp.readResponse();
        if (code != 200) {
            LOG_ERROR("%s: status code %d", feed.host().c_str(), code);
            return -1;
        }

        return code;
    }
    catch (StreamException& e) {
        LOG_ERROR("%s", e.msg);
        return -1;
    }

}
