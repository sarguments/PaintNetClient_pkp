#pragma once

// == MEMO ==
// 1. sin_addr.S_un.S_addr == sin_addr.s_addr

/*
ex)
	WCHAR serverDomain[] = L"procademyserver.iptime.org";
	IN_ADDR s_inAddr;
	ZeroMemory(&s_inAddr, sizeof(IN_ADDR));

	CSocketUtil::DomainToIP(serverDomain, &s_inAddr);
	WCHAR proIP[20] = { 0, };
	InetNtop(AF_INET, &s_inAddr, proIP, 20);

	wcout << L"SERVER IP : " << proIP << endl;
*/

class CSockUtill
{
public:
	static bool DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr);
	static bool SetNonBlock(SOCKET sock, bool bParam);
	static bool SetBroadCast(SOCKET sock, bool bParam);

	static bool	WSAStart();
	static void	CleanUp();

private:
};