#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "PaintNetClient_pkp.h"
#include "hoxy_Header.h"
#include "RingBuffer_AEK999.h"

#define MAX_LOADSTRING 100

//#define SERVERIP L"127.0.0.1"
#define SERVERPORT 25000
#define HEADERSIZE 2
#define UM_NETWORK (WM_USER + 1)
#define ARR_SIZE 1000

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

HWND g_hWnd;

WCHAR g_szIP[16] = {};
SOCKET g_serverSock = INVALID_SOCKET;
SOCKADDR_IN g_serverAddr;

bool g_isConnected;
bool g_sendFlag;

bool g_bClick;
int g_xPos;
int g_yPos;
int g_PreXpos;
int g_PreYpos;

// 링버퍼
CRingBuffer g_recvQ;
CRingBuffer g_sendQ;

// 패킷
struct st_DRAW_PACKET
{
	int		iStartX;
	int		iStartY;
	int		iEndX;
	int		iEndY;
};

// SEND //
int ProcSend(void);
int SendPacket(char * buffer, int size);

// RECEV //
int ProcRead(HWND hWnd);

// FUNC //
int NetworkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MouseAction(LPARAM lParam);

// DIALOG //
BOOL CALLBACK DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
// 정의에도 CALLBACK 넣어야함

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int iReturn = DialogBox(hInstance, MAKEINTRESOURCE(IDD_ADDR), NULL, DialogProc);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_PAINTNETCLIENTPKP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PAINTNETCLIENTPKP));

	CCmdStart myCmdStart;
	CSockUtill::WSAStart();

	g_serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_serverSock == INVALID_SOCKET)
	{
		CCmdStart::CmdDebugText(L"socket()", false);
	}

	g_serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, g_szIP, &g_serverAddr.sin_addr.s_addr);
	g_serverAddr.sin_port = htons(SERVERPORT);

	int ret_select = WSAAsyncSelect(g_serverSock, g_hWnd, UM_NETWORK, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (ret_select == SOCKET_ERROR)
	{
		CCmdStart::CmdDebugText(L"WSAAsyncSelect()", false);
		return -1;
	}

	int ret_connect = connect(g_serverSock, (SOCKADDR*)&g_serverAddr, sizeof(SOCKADDR));
	if (ret_connect != NOERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			CCmdStart::CmdDebugText(L"connect()", false);
			return -1;
		}
	}

	MSG msg;

	// 기본 메시지 루프입니다.
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

// 센드큐에서 보내기
int ProcSend(void)
{
	if (!g_isConnected)
	{
		CCmdStart::CmdDebugText(L"g_isConnected", false);
		return 0;
	}

	// sendFlag가 false면 sendQ에 일단 쌓아놓는다
	if (!g_sendFlag)
	{
		CCmdStart::CmdDebugText(L"g_sendFlag", false);
		return 0;
	}

	// 센드큐에 있는거 다 보낸다
	int inUseSize = g_sendQ.GetUseSize();
	while (inUseSize >= HEADERSIZE)
	{
		// 먼저 헤더를 본다
		unsigned short packetHeader;
		g_sendQ.Peek((char*)&packetHeader, HEADERSIZE);
		if (packetHeader == 0)
		{
			// 길이가 0인 경우
			return -1;
		}

		int packetSize = HEADERSIZE + packetHeader;

		// g_recvQ에 헤더사이즈 + 페이로드 길이 만큼 있는지
		if (g_sendQ.GetUseSize() < packetSize)
		{
			// TODO : 큐에 전체 패킷이 아직 다 못 들어온 경우? (계속 받는다)
			return 0;
		}

		// 일단 픽해서 센드하고 센드큐에서 보낸만큼 데이터 날림
		char localBuf[ARR_SIZE];
		g_sendQ.Peek((char*)localBuf, packetSize);

		int ret_send = send(g_serverSock, localBuf, packetSize, 0);
		if (ret_send == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// TODO : 우드블럭 테스트
				// WOULDBLOCK 뜨면 sendFlag = false

				g_sendFlag = false;
			}
			else
			{
				CCmdStart::CmdDebugText(L"send", false);

				return -1;
			}
		}

		wcout << L"sendQ Use : " << inUseSize << L" / ret_send : " << ret_send << endl;

		inUseSize -= ret_send;
		g_sendQ.MoveFrontPos(ret_send);
	}

	// TODO : 리턴값?
	return inUseSize;
}

// 센드큐에 넣기
int SendPacket(char * buffer, int size)
{
	char* pLocalBuf = buffer;
	int localSize = size;

	// 넣으려는 크기보다 남은 공간이 적을 경우
	if (g_sendQ.GetFreeSize() < localSize)
	{
		CCmdStart::CmdDebugText(L"GetFreeSize() < localSize", false);
		return -1;
	}

	int retval = g_sendQ.Enqueue(pLocalBuf, localSize);

	return retval;
}

int ProcRead(HWND hWnd)
{
	// 일단 리시브
	char localBuf[ARR_SIZE];
	int retval = recv(g_serverSock, localBuf, ARR_SIZE, 0);
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			CCmdStart::CmdDebugText(L"recv()", false);
			return -1;
		}
	}

	// 소켓종료
	if (retval == 0)
	{
		wcout << L"closesocket" << endl;
		return -1;
	}

	//// recv 받은만큼 넣을 공간이 있는지
	//if (g_recvQ.GetFreeSize() < retval)
	//{
	//	CCmdStart::CmdDebugText(L"GetFreeSize() < retval", false);
	//	return -1;
	//}

	int ret_enqueue = g_recvQ.Enqueue(localBuf, retval);
	if (ret_enqueue == -1)
	{
		// 리시브큐 꽉 찬 경우
		CCmdStart::CmdDebugText(L"g_recvQ.Enqueue", false);
		return -1;
	}

	if (ret_enqueue != retval)
	{
		// 큐에 다 안들어 간 경우
		CCmdStart::CmdDebugText(L"ret_enqueue != retval", false);
		return -1;
	}

	// DC 얻은 후에 그린다음, 반환
	HDC localDC = GetDC(hWnd);

	while (1)
	{
		int inUseSize = g_recvQ.GetUseSize();
		if (inUseSize < HEADERSIZE)
		{
			break;
		}

		// TODO : 0 체크?
		// 먼저 헤더를 본다
		unsigned short packetHeader;
		g_recvQ.Peek((char*)&packetHeader, HEADERSIZE);
		if (packetHeader == 0)
		{
			// 길이가 0인 경우
			return -1;
		}

		int packetSize = HEADERSIZE + packetHeader;

		// g_recvQ에 헤더사이즈 + 페이로드 길이 만큼 있는지
		if (g_recvQ.GetUseSize() < packetSize)
		{
			// TODO : 큐에 전체 패킷이 아직 다 못 들어온 경우? (계속 받는다)
			return 0;
		}

		wcout << L"recvQ Use : " << inUseSize << L" / ret_enqueue : " << ret_enqueue << endl;
		//printf("%d , %d\n", ret_enqueue, inUseSize);

		char deqBuf[ARR_SIZE];
		int retval = g_recvQ.Dequeue(deqBuf, packetSize);

		st_DRAW_PACKET* deqPacket;
		deqPacket = (st_DRAW_PACKET*)(deqBuf + HEADERSIZE);

		MoveToEx(localDC, deqPacket->iStartX, deqPacket->iStartY, NULL);
		LineTo(localDC, deqPacket->iEndX, deqPacket->iEndY);

		inUseSize -= retval;
	}

	ReleaseDC(hWnd, localDC);

	// TODO : retval 리턴
	return retval;
}

int NetworkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam))
	{
		CCmdStart::CmdDebugText(L"WSAGETSELECTERROR", false);
		return -1;
	}

	switch (WSAGETSELECTEVENT(lParam))
	{
		// 통신을 위한 연결 절차가 끝났다.
	case FD_CONNECT:
	{
		// 연결 플래그
		g_isConnected = true;
	}
	break;
	case FD_WRITE:
	{
		// 처음 접속했을때 or 송신버퍼가 꽉찼다가 비워졌을때
		// WSAEWOULDBLOCK
		g_sendFlag = true;

		// 다시 연결 됐을 때 다 보낸다.
		ProcSend();
	}
	break;
	case FD_READ:
	{
		if (ProcRead(hWnd) < 0)
		{
			// TODO : -1이면 끊는다
			exit(1);
		}
	}
	break;
	case FD_CLOSE:
	{
		closesocket(g_serverSock);

		// TODO : 언제 false?
		g_isConnected = false;
		g_sendFlag = false;

		return -1;
	}
	break;
	}

	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAINTNETCLIENTPKP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PAINTNETCLIENTPKP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd)
	{
		return FALSE;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case UM_NETWORK:
	{
		int retval = NetworkProc(hWnd, message, wParam, lParam);
		if (retval < 0)
		{
			exit(1);
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_LBUTTONDOWN:
	{
		g_bClick = true;
		g_xPos = GET_X_LPARAM(lParam);
		g_yPos = GET_Y_LPARAM(lParam);
	}
	break;

	case WM_LBUTTONUP:
	{
		g_bClick = false;
	}
	break;

	case WM_MOUSEMOVE:
	{
		if (g_bClick != true)
		{
			break;
		}

		//wcout << L"-------------------------------" << endl;
		//wcout << L"MOUSEMOVE" << endl;
		//g_isConnected = true;

		MouseAction(lParam);
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		//wcout << L"PAINT" << endl;

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		closesocket(g_serverSock);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void MouseAction(LPARAM lParam)
{
	g_PreXpos = g_xPos;
	g_PreYpos = g_yPos;

	g_xPos = GET_X_LPARAM(lParam);
	g_yPos = GET_Y_LPARAM(lParam);

	unsigned short Len = 16;
	st_DRAW_PACKET localPacket;

	localPacket.iStartX = g_PreXpos;
	localPacket.iStartY = g_PreYpos;
	localPacket.iEndX = g_xPos;
	localPacket.iEndY = g_yPos;

	char toSendBuf[ARR_SIZE];
	int payloadSize = sizeof(st_DRAW_PACKET);
	memcpy(toSendBuf, &Len, HEADERSIZE);
	memcpy(toSendBuf + HEADERSIZE, &localPacket, payloadSize);

	int retval = SendPacket((char*)toSendBuf, HEADERSIZE + payloadSize);
	if (retval < 0)
	{
		// TODO : -1이면 끊고 0이면 그냥 스킵
		exit(1);
	}

	// sendQ에 있는거 다 보낸다.
	ProcSend();
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hEditBox;

	switch (iMsg)
	{
	case WM_INITDIALOG:
	{
		memset(g_szIP, 0, sizeof(WCHAR) * 16);
		hEditBox = GetDlgItem(hWnd, IDC_EDIT);
		SetWindowText(hEditBox, L"127.0.0.1");
		return TRUE;
	}
	break;
	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDOK:
		{
			GetDlgItemText(hWnd, IDC_EDIT, g_szIP, 16);
			EndDialog(hWnd, 99939);
			return TRUE;
		}
		break;
		}
	}
	}
	return FALSE;
}