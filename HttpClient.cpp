// HttpClientStub.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <WinInet.h>

#include <Utils.h>

class CommandLineOptions
{
public:
    //=================================================================================================
    CommandLineOptions ()
        : m_url(0)
        , m_remoteFilePath(0)
        , m_useHttps(false)
        , m_localFilePath(0)
    {
    }

    //=================================================================================================
    ~CommandLineOptions ()
    {
        if (m_url)
        {
            ::free(m_url);
        }

        if (m_remoteFilePath)
        {
            ::free(m_remoteFilePath);
        }

        if (m_localFilePath)
        {
            ::free(m_localFilePath);
        }
    }

    //=================================================================================================
    void setUrl (const char* url)
    {
        if (!url)
        {
            return;
        }

        size_t urlSize(::strlen(url));

        if (!urlSize)
        {
            return;
        }

        for (size_t c = 0; c < urlSize; ++c)
        {
            if (url[c] == '/')
            {
                if (c)
                {
                    m_url = (char*)::malloc(sizeof(char) * (c + 1));
                    ::memset(m_url, 0, sizeof(char) * (c + 1));
                    ::memcpy(m_url, url, sizeof(char) * c);
                }

                m_remoteFilePath = (char*)::malloc(sizeof(char) * (urlSize - c + 1));
                ::memset(m_remoteFilePath, 0, sizeof(char) * (urlSize - c + 1));
                ::strcpy(m_remoteFilePath, url + c);
                return;
            }
        }

        m_url = (char*)::malloc(sizeof(char) * (urlSize + 1));
        ::strcpy(m_url, url);
        m_remoteFilePath = (char*)::malloc(sizeof(char) * 2);
        ::memset(m_remoteFilePath, 0, sizeof(char) * 2);
        m_remoteFilePath[0] = '/';
    }

    const char* url () const { return m_url; }
    const char* remoteFilePath () const { return m_remoteFilePath; }
    bool useHttps () const { return m_useHttps; }
    void setUseHttps (bool useHttp) { m_useHttps = useHttp; }
    const char* localFilePath () const { return m_localFilePath; }
    void setLocalFilePath (const char* localFilePath) { m_localFilePath = Clone(localFilePath); }

private:
    /// The server URL
    /// @owns
    char* m_url;

    /// The remote file path
    /// @owns
    char* m_remoteFilePath;

    /// If true HTTPS is used, otherwise HTTP
    bool m_useHttps;

    /// The file to store the returned file from the server
    /// @owns
    char* m_localFilePath;
};

//=====================================================================================================
void ShowUsage ()
{
    ::printf("Usage: HttpClient [/u server_url /s /f file_path]\n" \
             "/u The URL to the server, e.g. www.google.com\n" \
             "/s (optional) Use HTTPS instead of HTTP\n" \
             "/f (optional) Path to the file to store the file returned " \
             "from the server\n");
}

//=====================================================================================================
void ParseCommandLine (char* argv[],
                       int argc,
                       CommandLineOptions& options)
{
    if (argc < 3)
    {
        ShowUsage();
        exit(1);
    }

    for (int a = 1; a < argc; ++a)
    {
        if (!::strcmp(argv[a], "/u"))
        {
            ++a;

            if (a >= argc)
            {
                ShowUsage();
                exit(1);
            }

            options.setUrl(argv[a]);
        }
        else if (!::strcmp(argv[a], "/s"))
        {
            options.setUseHttps(true);
        }
        else if (!::strcmp(argv[a], "/f"))
        {
            ++a;

            if (a >= argc)
            {
                ShowUsage();
                exit(1);
            }

            options.setLocalFilePath(argv[a]);
        }
        else
        {
            ShowUsage();
            exit(1);
        }
    }

    if (!options.url())
    {
        ShowUsage();
        exit(1);
    }
}

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
int _tmain (int argc, _TCHAR* argv[])
{
    CommandLineOptions options;
    ParseCommandLine(argv, argc, options);

    char* headers = Format("Host: %s\nContent-Type: application/x-www-form-urlencoded",
                           options.url());
    LPCSTR accept[2] = {"*/*", NULL};

    HINTERNET hSession = ::InternetOpenA("http generic",
                                         INTERNET_OPEN_TYPE_PRECONFIG,
                                         NULL,
                                         NULL,
                                         0);
    HINTERNET hConnect = ::InternetConnectA(hSession,
                                            options.url(),
                                            options.useHttps() ?
                                            INTERNET_DEFAULT_HTTPS_PORT :
                                            INTERNET_DEFAULT_HTTP_PORT,
                                            NULL,
                                            NULL,
                                            INTERNET_SERVICE_HTTP,
                                            0,
                                            1);
    HINTERNET hRequest = ::HttpOpenRequestA(hConnect,
                                            "GET",
                                            options.remoteFilePath(),
                                            NULL,
                                            NULL,
                                            accept,
                                            (options.useHttps() ?
                                             INTERNET_FLAG_SECURE |
                                             INTERNET_FLAG_IGNORE_CERT_CN_INVALID :
                                             0),
                                            0);

    bool result = ::HttpSendRequestA(hRequest, headers, ::strlen(headers), LPVOID(0), 0);

    if (!result)
    {
        ::free(headers);
        ::printf("HttpSendRequest failed, code=%d\n", ::GetLastError());
        return 0;
    }

    ::free(headers);

    char* responseHeaders = GetHeaders(hRequest);

    if (::strcmp(responseHeaders, "200"))
    {
        ::printf("HTTP result is not HTTP_OK\n");
        ::free(responseHeaders);
        return 0;
    }
    
    ::free(responseHeaders);

    FILE* file(0);

    if (options.localFilePath())
    {
        file = ::fopen(options.localFilePath(), "wb");

        if (!file)
        {
            ::printf("Could not open file %s for writing.\n", options.localFilePath());
            return 0;
        }
    }

    char* buffer(0);
    int bufferSize(0);

    ReadFile(hRequest, &buffer, bufferSize);

    while (bufferSize)
    {
        if (file)
        {
            ::fwrite(buffer, sizeof(char), bufferSize, file);
        }

        ::free(buffer);
        ReadFile(hRequest, &buffer, bufferSize);
    }

    if (file)
    {
        ::fclose(file);
    }

    ::InternetCloseHandle(hRequest);
    ::InternetCloseHandle(hConnect);
    ::InternetCloseHandle(hSession);

	return 0;
}

