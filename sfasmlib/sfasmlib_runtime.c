/*
 * 32-bit DLL for output.exe (IMAGE_FILE_MACHINE_I386). Build with -m32 under MinGW.
 * Exports match import_table (ExitProgram, GetNumber, PrintNumber, sockets).
 */
#if !defined(_WIN32)
#error sfasmlib_runtime is Windows-only
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static WSADATA g_wsa;
static int g_wsa_started;

static void flush_stdio_once(void)
{
    static int configured = 0;

    if (!configured)
    {
        configured = 1;
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}

__declspec(dllexport) void __cdecl ExitProgram(void)
{
    if (g_wsa_started)
    {
        WSACleanup();
        g_wsa_started = 0;
    }
    ExitProcess(0);
}

__declspec(dllexport) int __cdecl GetNumber(void)
{
    char buf[96];

    flush_stdio_once();
    printf("> ");
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return -1;

    return (int)strtol(buf, NULL, 10);
}

__declspec(dllexport) void __cdecl PrintNumber(int x)
{
    flush_stdio_once();
    printf("%d\n", x);
    fflush(stdout);
}

__declspec(dllexport) int __cdecl SockInit(void)
{
    if (WSAStartup(MAKEWORD(2, 2), &g_wsa) != 0)
        return -1;
    g_wsa_started = 1;
    return 0;
}

__declspec(dllexport) int __cdecl SockTcpConnect(int o1, int o2, int o3, int o4, int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned short)(port & 0xFFFF));
    addr.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)(o1 & 0xFF);
    addr.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)(o2 & 0xFF);
    addr.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)(o3 & 0xFF);
    addr.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)(o4 & 0xFF);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        closesocket(s);
        return -1;
    }

    return (int)s;
}

__declspec(dllexport) void __cdecl SockClose(int sock)
{
    if (sock >= 0)
        closesocket((SOCKET)sock);
}

__declspec(dllexport) int __cdecl SockSendInt(int sock, int val)
{
    char buf[72];
    int  len = snprintf(buf, sizeof(buf), "%d\n", val);
    if (len <= 0 || sock < 0)
        return -1;

    int sent = send((SOCKET)sock, buf, len, 0);
    return sent == len ? 0 : -1;
}

__declspec(dllexport) int __cdecl SockRecvInt(int sock)
{
    char buf[96];
    int  total = 0;

    if (sock < 0)
        return -1;

    while (total < (int)sizeof(buf) - 1)
    {
        char c;
        int  r = recv((SOCKET)sock, &c, 1, 0);
        if (r <= 0)
            return -1;
        if (c == '\n' || c == '\r')
            break;
        buf[total++] = c;
    }

    buf[total] = '\0';
    return (int)strtol(buf, NULL, 10);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void)hinstDLL;
    (void)lpvReserved;
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}
