#include <stdio.h>
#include <Windows.h>

typedef UINT(WINAPI* WINEXEC)(LPCSTR, UINT);  //함수 포인터


//Thread Parameter
typedef struct _THREAD_PARAM
{
    FARPROC pFunc[2];
    char szBuf[4][128]; 
} THREAD_PARAM, * PTHREAD_PARAM;


// LoadLibraryA() 함수포인터
typedef HMODULE(WINAPI* PFLOADLIBRARYA)
(
    LPCSTR lpLibFileName
    );

//GetProcAddress() 함수포인터
typedef FARPROC(WINAPI* PFGETPROCADDRESS)
(
    HMODULE hModule,
    LPCSTR lpProcName
    );

//MessageBoxA() 함수포인터
typedef int (WINAPI* PFMESSAGEBOXA)
(
    HWND hWnd,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType
    );

DWORD WINAPI ThreadProc(LPVOID lParam)
{
    PTHREAD_PARAM pParam = (PTHREAD_PARAM)lParam;
    HMODULE hMod = NULL;
    FARPROC pFunc = NULL;

    hMod = ((PFLOADLIBRARYA)pParam->pFunc[0])(pParam->szBuf[0]);

    pFunc = (FARPROC)((PFGETPROCADDRESS)pParam->pFunc[1])(hMod, pParam->szBuf[1]);

    
    ((PFMESSAGEBOXA)pFunc)(NULL, pParam->szBuf[2], pParam->szBuf[3], MB_OK);

    return 0;
}

void AfterFunc() {};

//코드를 삽입할 프로세스의 PID를 전달받음
BOOL InjectCode(DWORD dwPID)
{
    HMODULE hMod = NULL;
    THREAD_PARAM param = { 0, };
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    LPVOID pRemoteBuf[2] = { 0, };
    DWORD dwSize = 0;

    hMod = GetModuleHandleA("kernel32.dll");

    param.pFunc[0] = GetProcAddress(hMod, "LoadLibraryA");
    param.pFunc[1] = GetProcAddress(hMod, "GetProcAddress");
    printf("%x", param.pFunc[1]);

    
    strcpy(param.szBuf[0], "user32.dll"); 
    strcpy(param.szBuf[1], "MessageBoxA");
    strcpy(param.szBuf[2], "This is codeinject");  //메시지 박스에 나올 문자열
    strcpy(param.szBuf[3], "HYW");

    //Target Process의 Handle 획득
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

    dwSize = sizeof(THREAD_PARAM);
    pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);


    WriteProcessMemory(hProcess, // 메모리를 쓸 프로세스의 핸들
        pRemoteBuf[0], //프로세스의 기본 주소 포인터
        (LPVOID)&param, //메모리에 쓸 데이터에 대한 포인터
        dwSize, //메모리에 쓸 데이터 크기
        NULL); //전송된 데이터 크기를 수신받는 포인터
     

    dwSize = (DWORD)InjectCode - (DWORD)ThreadProc;
    pRemoteBuf[1] = VirtualAllocEx(hProcess,
        NULL,
        dwSize,
        MEM_COMMIT,
        PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hProcess,
        pRemoteBuf[1],
        (LPVOID)ThreadProc,
        dwSize,
        NULL);


    hThread = CreateRemoteThread(hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)pRemoteBuf[1],
        pRemoteBuf[0],
        0,
        NULL);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}


int main()
{
    int getPID = 0;
    
    printf("PID : ");
    scanf("%d", &getPID);

    InjectCode(getPID);

    return 0;
}
