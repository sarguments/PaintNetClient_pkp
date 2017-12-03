#include "stdafx.h"
#include "hoxy_Header.h"

bool CSockUtill::DomainToIP(WCHAR * szDomain, IN_ADDR * pAddr)
{
	ADDRINFOW* pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0)
	{
		return false;
	}
	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return true;
}

bool CSockUtill::SetNonBlock(SOCKET sock, bool bParam)
{
	u_long optionParam = 0;
	if (bParam)
	{
		optionParam = 1;
	}

	int ret_ioctl = ioctlsocket(sock, FIONBIO, &optionParam);
	if (ret_ioctl == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"ioctlsocket", false);
		return false;
	}
	CCmdStart::CmdDebugText(L"ioctlsocket", true);

	return true;
}

bool CSockUtill::SetBroadCast(SOCKET sock, bool bParam)
{
	BOOL bEnable = FALSE;
	if (bParam)
	{
		bEnable = TRUE;
	}

	int ret_setsock = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&bEnable, sizeof(bEnable));
	if (ret_setsock == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"setsockopt() SO_BROADCAST", false);
		return false;
	}
	CCmdStart::CmdDebugText(L"setsockopt() SO_BROADCAST", true);

	return true;
}

bool CSockUtill::WSAStart()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		CCmdStart::CmdDebugText(L"WSAStartup", false);
		return false;
	}
	CCmdStart::CmdDebugText(L"WSAStartup", true);
	return true;
}

void CSockUtill::CleanUp()
{
	WSACleanup();
}