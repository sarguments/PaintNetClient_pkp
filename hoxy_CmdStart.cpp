#include "stdafx.h"
#include "hoxy_Header.h"
#include "hoxy_CmdStart.h"

CCmdStart::CCmdStart()
{
	// 랜덤 시드
	//srand((unsigned int)time(NULL));

	// TimePeriod
	int retval = timeBeginPeriod(1);
	if (retval != TIMERR_NOERROR ||
		retval == TIMERR_NOCANDO)
	{
		CCmdStart::CmdDebugText(L"CMD START Init", false);
	}

	/*
	HANDLE  hConsole;
	CONSOLE_CURSOR_INFO stConsoleCursor;

	//-------------------------------------------------------------
	// 화면의 커서를 안보이게끔 설정한다.
	//-------------------------------------------------------------
	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1;			// 커서 크기.
	// 이상하게도 0 이면 나온다. 1로하면 안나온다.

	//-------------------------------------------------------------
	// 콘솔화면 (스텐다드 아웃풋) 핸들을 구한다.
	//-------------------------------------------------------------
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(hConsole, &stConsoleCursor);
	*/

	// 콘솔 창 띄우기
	AllocConsole();
	FILE *acStreamOut;
	freopen_s(&acStreamOut, "CONOUT$", "wt", stdout);

	//_wsetlocale(LC_ALL, L"");
	//std::locale::global(std::locale(""));

	CCmdStart::CmdDebugText(L"CMD START Init", true);
}

CCmdStart::~CCmdStart()
{
	timeEndPeriod(1);
	FreeConsole();
}

void CCmdStart::CmdDebugText(WCHAR* text, bool bParam)
{
	if (bParam)
	{
		wcout << std::setw(30) << text << std::setw(50) << L"|OK|" << endl;
	}
	else
	{
		wcout << std::setw(30) << text << std::setw(40) << L"|ERROR| : " << std::setw(10) << GetLastError() << endl;
	}
}