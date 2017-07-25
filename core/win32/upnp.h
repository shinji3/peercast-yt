// ------------------------------------------------------------------------------
// UPnPクラス
//
// 下記ホームページで公開されているソースを利用させていただきました。
// βえんどるふぃん -平穏で不穏な日々-様 URL:http://www.bosuke.mine.nu/blog/ 
//
// ------------------------------------------------------------------------------

#pragma warning( disable : 4786 )
#pragma comment(lib, "Wininet.lib")

#ifndef _UPNP_H_
#define _UPNP_H_

#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#import <msxml.dll> named_guids
#include <string>
#include <vector>
#include <map>

class YMSSDPDiscover  
{
public:

	static bool Initialize();
	static void Uninitialize();

	YMSSDPDiscover();
	virtual ~YMSSDPDiscover();

	struct LocationInfo
	{
		std::string mST;
		std::string mLocation;
	};

	//ソケットオープンとクローズ
	bool Open();
	void Close();

	//検索リクエストを出す
	bool Send(const char* st);

	//結果を受け取る
	enum
	{
		Error = -1,
		HttpError = -2,
		NoData = 0,
		Timeout = 1,
		OK = 2
	};
	int Recv(LocationInfo& info,DWORD timeout);

	//エラー情報
	int GetLastError() const
	{
		return mLastError;
	}

	//controlURLの取得
	static std::string GetControlURL(const char* location,const char* st);

protected:

	std::vector<std::string> mSTList;
	SOCKET mSocket;
	int mLastError;
	HANDLE mEvent;

	static bool mInitialized;
};


class YMSoapAction  
{
public:

	YMSoapAction(const char* serviceType,const char* actionName);
	virtual ~YMSoapAction();

	//パラメータをセットする
	void SetParameter(const char* filed,const char* value);
	void ResetParameter();

	//POSTする
	int Invoke(const char* url);

protected:
	
	std::string mServiceType;
	std::string mActionName;

	typedef std::map<std::string,std::string> Parameters;
	Parameters mParameter;
};

#endif //! _UPNP_H_
