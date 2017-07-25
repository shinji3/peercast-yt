// ------------------------------------------------------------------------------
// UPnP�N���X
//
// ���L�z�[���y�[�W�Ō��J����Ă���\�[�X�𗘗p�����Ă��������܂����B
// ������ǂ�ӂ��� -�����ŕs���ȓ��X-�l URL:http://www.bosuke.mine.nu/blog/ 
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

	//�\�P�b�g�I�[�v���ƃN���[�Y
	bool Open();
	void Close();

	//�������N�G�X�g���o��
	bool Send(const char* st);

	//���ʂ��󂯎��
	enum
	{
		Error = -1,
		HttpError = -2,
		NoData = 0,
		Timeout = 1,
		OK = 2
	};
	int Recv(LocationInfo& info,DWORD timeout);

	//�G���[���
	int GetLastError() const
	{
		return mLastError;
	}

	//controlURL�̎擾
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

	//�p�����[�^���Z�b�g����
	void SetParameter(const char* filed,const char* value);
	void ResetParameter();

	//POST����
	int Invoke(const char* url);

protected:
	
	std::string mServiceType;
	std::string mActionName;

	typedef std::map<std::string,std::string> Parameters;
	Parameters mParameter;
};

#endif //! _UPNP_H_
