// ------------------------------------------------------------------------------
// UPnPクラス
//
// 下記ホームページで公開されているソースを利用させていただきました。
// βえんどるふぃん -平穏で不穏な日々-様 URL:http://www.bosuke.mine.nu/blog/ 
//
// ------------------------------------------------------------------------------
#include "upnp.h"
#include "sys.h"
#include "socket.h"
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

bool YMSSDPDiscover::mInitialized = false;

bool YMSSDPDiscover::Initialize()
{
	if(!mInitialized)
	{
		WSADATA data;
		if(WSAStartup(MAKEWORD(2,0),&data) != 0)
		{
			return false;
		}
		if(LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 0)
		{
			WSACleanup();
			return false;
		}
		mInitialized = true;
	}
	return true;
}

void YMSSDPDiscover::Uninitialize()
{
	if(mInitialized)
	{
		WSACleanup();
		mInitialized = false;
	}
}

YMSSDPDiscover::YMSSDPDiscover()
:mSocket(INVALID_SOCKET),mLastError(0)
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
	if(mSocket != INVALID_SOCKET)
	{
		return true;
	}

	//UDPソケットつくりーの
	mSocket = socket(AF_INET,SOCK_DGRAM,0);
	if(mSocket == INVALID_SOCKET)
	{
		mLastError = WSAGetLastError();
		return false;
	}

	//空いているポートにbind
	sockaddr_in addr;
	ZeroMemory(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	unsigned short port = 20000;
	while(1)
	{
		addr.sin_port = htons(port);
		if(bind(mSocket,(sockaddr*)&addr,sizeof(addr)) == 0)
		{
			//成功
			break;
		}
		int err = WSAGetLastError();
		if(err == WSAEADDRINUSE)
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
	setsockopt(mSocket,IPPROTO_IP,IP_MULTICAST_TTL,&TTL,sizeof(TTL));
	ioctlsocket(mSocket,FIONBIO,&flags);

	//イベントオブジェクト作成
	mEvent = WSACreateEvent();

	return true;
}

void YMSSDPDiscover::Close()
{
	mLastError = 0;
	if(mSocket != INVALID_SOCKET)
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
	SOCKET s = socket(AF_INET,SOCK_DGRAM,0);
	WSAIoctl(s,SIO_ADDRESS_LIST_QUERY,NULL,0,NULL,0,&bufferSize,NULL,NULL);
	if(bufferSize == 0)
	{
		mLastError = WSAGetLastError();
		closesocket(s);
		return false;
	}
	SOCKET_ADDRESS_LIST* list = (SOCKET_ADDRESS_LIST*)_alloca(bufferSize);
	if(WSAIoctl(s,SIO_ADDRESS_LIST_QUERY,NULL,0,list,bufferSize,&bufferSize,NULL,NULL) == SOCKET_ERROR)
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
	ZeroMemory(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = ClientSocket::getIP(UPNP_GROUP);
	dest_addr.sin_port = htons(UPNP_PORT);

	in_addr interface_addr;
	ZeroMemory(&interface_addr,sizeof(interface_addr));

	bool ok = false;
	for(int i = 0; i < list->iAddressCount; ++i)
	{
		interface_addr.s_addr = ((sockaddr_in*)list->Address[i].lpSockaddr)->sin_addr.s_addr;
		if(setsockopt(mSocket,IPPROTO_IP,IP_MULTICAST_IF,(char*)&interface_addr, sizeof(interface_addr)) == 0)
		{
			if(sendto(mSocket,data.data(),static_cast<int>(data.length()),0,(sockaddr*)&dest_addr,sizeof(dest_addr)) > 0)
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
	if(ok)
	{
		mLastError = 0;
		
		//STのリストにぶち込む
		if(std::find(mSTList.begin(),mSTList.end(),st) == mSTList.end())
		{
			mSTList.push_back(st);
		}
	}
	return ok;
}


//////////////////////////////////////////////////
// データ受信
//////////////////////////////////////////////////
int YMSSDPDiscover::Recv(LocationInfo& info,DWORD timeout)
{

	mLastError = 0;
	
	//指定ミリセカンドデータが受信できるまで待つ
	WSAResetEvent(mEvent);
	if(WSAEventSelect(mSocket,mEvent,FD_READ) == SOCKET_ERROR)
	{
		mLastError = WSAGetLastError();
		return Error;
	}

	WSANETWORKEVENTS events;
	DWORD ret = WSAWaitForMultipleEvents(1,&mEvent,FALSE,timeout,FALSE);
	if(ret == WSA_WAIT_TIMEOUT)
	{
		return Timeout;
	}
	if(ret == WSA_WAIT_FAILED
		|| WSAEnumNetworkEvents(mSocket,mEvent,&events) != 0
		|| !(events.lNetworkEvents & FD_READ))
	{
		mLastError = WSAGetLastError();
		return Error;
	}

	//受信
	char buffer[BUFFER_SIZE + 1];
	sockaddr_in addr;
	int addrLen = sizeof(addr);
	int readLen = recvfrom(mSocket,buffer,BUFFER_SIZE,0,(sockaddr*)&addr,&addrLen);
	if(readLen == SOCKET_ERROR)
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
	while(std::getline(input,line))
	{

		//ATLTRACE("recv:%s\n",line.c_str());
		
		if(first)
		{
			//200以外はダメポ
			std::istringstream responseLine(line);
			std::string dummy;
			int code = 0;
			responseLine >> dummy >> code;
			if(code != 200)
			{
				return HttpError;
			}
			first = false;
		}
		else
		{
			//フィールドと値にわける
			int index = static_cast<int>(line.find(':'));
			if(index == std::string::npos)
			{
				continue;
			}
			std::string field = line.substr(0,index);
			if((index = static_cast<int>(line.find_first_not_of(' ',index + 1))) != std::string::npos)
			{
				std::string value = line.substr(index);
				if(!value.empty())
				{
					if(*value.rbegin() == '\r')
					{
						value = value.substr(0,value.length() - 1); 
					}
					if(Sys::stricmp(field.c_str(),"ST") == 0)
					{
						tempInfo.mST = value;
					}
					else if(Sys::stricmp(field.c_str(),"Location") == 0)
					{
						tempInfo.mLocation = value;
					}
				}
			}
		}
	}

	//値をチェック
	if(tempInfo.mST.empty()
		|| tempInfo.mLocation.empty()
		||std::find(mSTList.begin(),mSTList.end(),tempInfo.mST) == mSTList.end())
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
std::string YMSSDPDiscover::GetControlURL(const char* location,const char* st)
{
	std::string result;
	std::string baseURL;
	std::string relativeURL;

	//XMLを取得
	try
	{
		MSXML2::IXMLHTTPRequestPtr http;
		if(FAILED(http.CreateInstance(__uuidof(MSXML2::XMLHTTP60))))
		{
			return result;
		}
		http->open("GET",location,VARIANT_FALSE);
		http->send();
		if(http->status != 200)
		{
			return result;
		}
		MSXML2::IXMLDOMDocumentPtr doc = http->responseXML;

		//controlURL取得
		std::string xPath = "//service[serviceType=\"";
		xPath += st;
		xPath += "\"]/controlURL";
		MSXML2::IXMLDOMNodePtr controlURLNode = doc->selectSingleNode(xPath.c_str());
		if(controlURLNode)
		{
			relativeURL = controlURLNode->text;
		}
		else
		{
			return result;
		}

		
		//BASEURL取得
		MSXML2::IXMLDOMNodePtr baseURLNode = doc->selectSingleNode("//URLBase");
		if(baseURLNode)
		{
			baseURL = baseURLNode->text;
		}
		else
		{
			baseURL = location;
		}
	}
	catch(_com_error&)
	{
		return result;
	}

	//URLを結合
	DWORD size = 0;
	InternetCombineUrl(baseURL.c_str(),relativeURL.c_str(),NULL,&size,0);
	char* buf = (char*)_alloca(size);
	InternetCombineUrl(baseURL.c_str(),relativeURL.c_str(),buf,&size,0);
	result = buf;
	
	return result;
}




YMSoapAction::YMSoapAction(const char* serviceType,const char* actionName)
:mServiceType(serviceType),mActionName(actionName)
{
	CoInitialize(NULL);
}

YMSoapAction::~YMSoapAction()
{
	CoUninitialize();
}

void YMSoapAction::SetParameter(const char* field,const char* value)
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
	//送信データ作成

	try
	{
		MSXML2::IXMLDOMDocumentPtr sendXML;
		if(FAILED(sendXML.CreateInstance(__uuidof(MSXML2::DOMDocument60))))
		{
			return -1;
		}
		sendXML->async = VARIANT_FALSE;
		sendXML->preserveWhiteSpace = VARIANT_FALSE;
		sendXML->resolveExternals = VARIANT_FALSE;
		sendXML->loadXML("<?xml version=\"1.0\"?>"
			"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
			"<SOAP-ENV:Body>"
			"</SOAP-ENV:Body>"
			"</SOAP-ENV:Envelope>");
		MSXML2::IXMLDOMElementPtr body = sendXML->selectSingleNode("/SOAP-ENV:Envelope/SOAP-ENV:Body");

		std::string name = "m:" + mActionName;
		_variant_t varNodeType((short)MSXML2::NODE_ELEMENT);
		MSXML2::IXMLDOMNodePtr node = NULL;
		node = sendXML->createNode(varNodeType,name.c_str(),mServiceType.c_str());
		Parameters::const_iterator p;
		for(p = mParameter.begin(); p != mParameter.end(); ++p)
		{
			MSXML2::IXMLDOMElementPtr param = sendXML->createElement(p->first.c_str());
			param->text = p->second.c_str();
			node->appendChild(param);
		}
		body->appendChild(node);

		//送信する
		MSXML2::IXMLHTTPRequestPtr http;
		if(FAILED(http.CreateInstance(__uuidof(MSXML2::XMLHTTP60))))
		{
			return -1;
		}
		if(FAILED(http->open("POST",url,VARIANT_FALSE)))
		{
			return -1;
		}
		std::string action = mServiceType + "#" + mActionName;
		http->setRequestHeader("SOAPAction",action.c_str());
		http->send(sendXML->xml);

		return http->status;
	}
	catch(_com_error&/* ex */)
	{
		//ATLTRACE((const char*)ex.Description());
		return -1;
	}
}
