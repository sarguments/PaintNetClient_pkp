#pragma once

// 구버전 제외
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib") // TimePeriod

#include <Windows.h>
#include <Windowsx.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#include <iostream>
#include <iomanip>
//#include <locale>
//#include <locale.h>

#include <time.h>
#include <Mmsystem.h>

using std::wcout;
using std::wcin;
using std::endl;

#include "hoxy_CmdStart.h"
#include "hoxy_SockUtill.h"