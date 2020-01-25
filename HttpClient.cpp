// HttpClientStub.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <WinInet.h>

//=====================================================================================================
char* GetHeaders (HINTERNET hRequest)
{
    DWORD infoLevel = HTTP_QUERY_STATUS_CODE;
    DWORD infoBufferLength = 10;
    char* infoBuffer = (char*)::malloc(infoBufferLength + 1);

    while (!::HttpQueryInfoA(hRequest, infoLevel, infoBuffer, &infoBufferLength, NULL))
    {
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            ::free(infoBuffer);
            infoBuffer = (char*)::malloc(infoBufferLength + 1);
        }
        else
        {
            ::printf("HttpQueryInfo failed, error = %d (0x%x)\n", ::GetLastError(), ::GetLastError());
            break;
        }
    }

    infoBuffer[infoBufferLength] = '\0';
    return infoBuffer;
}

//=====================================================================================================
void ReadFile (HINTERNET hRequest, char** buffer, int& bufferSize)
{
    DWORD bytesAvailable(0);
    DWORD bytesRead(0);

    if (!::InternetQueryDataAvailable(hRequest, &bytesAvailable, 0, 0))
    {
        ::printf("No bytes available");
        return;
    }
        
    *buffer = (char*)::malloc(bytesAvailable + 1);
    ::memset(*buffer, 0, bytesAvailable + 1);

    bool result = ::InternetReadFile(hRequest, *buffer, bytesAvailable, &bytesRead);
    bufferSize = bytesRead;

    while (result &&
           bufferSize < bytesAvailable)
    {
        result = ::InternetReadFile(hRequest, *buffer, bytesAvailable, &bytesRead);
        bufferSize += bytesRead;
    }
}

//=====================================================================================================
int _tmain(int argc, _TCHAR* argv[])
{
    const char* serverIp = "www.google.com";
    const char* headers = "Host: www.google.com\nContent-Type: application/x-www-form-urlencoded";
    LPCSTR accept[2] = {"*/*", NULL};

    HINTERNET hSession = ::InternetOpenA("http generic",
                                         INTERNET_OPEN_TYPE_PRECONFIG,
                                         NULL,
                                         NULL,
                                         0);
    HINTERNET hConnect = ::InternetConnectA(hSession,
                                            serverIp,
                                            INTERNET_DEFAULT_HTTPS_PORT,
                                            NULL,
                                            NULL,
                                            INTERNET_SERVICE_HTTP,
                                            0,
                                            1);
    HINTERNET hRequest = ::HttpOpenRequestA(hConnect,
                                            "GET",
                                            "/",
                                            NULL,
                                            NULL,
                                            accept,
                                            INTERNET_FLAG_SECURE |
                                            INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
                                            0);

    bool result = ::HttpSendRequestA(hRequest, headers, ::strlen(headers), LPVOID(0), 0);

    if (!result)
    {
        ::printf("HttpSendRequest failed, code=%d", ::GetLastError());
        ::system("pause>nul");
        return 0;
    }

    FILE* file = ::fopen("C:\\Temp\\HttpResponse.txt", "w");

    if (!file)
    {
        ::printf("Could not open file C:\\Temp\\HttpResponse.txt for writing.");
        ::system("pause>nul");
        return 0;
    }

    char* responseHeaders = GetHeaders(hRequest);

    if (::strcmp(responseHeaders, "200"))
    {
        ::printf("HTTP result is not HTTP_OK");
        ::free(responseHeaders);
        ::system("pause>nul");
        return 0;
    }
    
    ::free(responseHeaders);

    char* buffer(0);
    int bufferSize(0);

    ReadFile(hRequest, &buffer, bufferSize);

    while (bufferSize)
    {
        ::fwrite(buffer, sizeof(char), bufferSize, file);
        ::free(buffer);
        ReadFile(hRequest, &buffer, bufferSize);
    }

    ::fclose(file);

    ::InternetCloseHandle(hRequest);
    ::InternetCloseHandle(hConnect);
    ::InternetCloseHandle(hSession);

	return 0;
}

