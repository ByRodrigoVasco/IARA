#include <stddef.h>

#include "cloud_http.h"

#if defined(IARA_NUVEM_URL) && defined(IARA_NUVEM_CHAVE)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Só Windows aqui: o windows.h não pode se misturar com o raylib.h. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#define RESPONSE_MAX (4 * 1024 * 1024)   // Uma lista de saves enorme cabe com muita folga
#define TIMEOUT_RESOLVE_MS 5000
#define TIMEOUT_CONNECT_MS 5000
#define TIMEOUT_TRANSFER_MS 8000

// Lê a resposta inteira; NULL se a conexão cair no meio
static char *read_response(HINTERNET request)
{
    char *text = NULL;
    size_t length = 0, capacity = 0;

    for (;;)
    {
        DWORD received = 0;

        if (length + 4096 + 1 > capacity)
        {
            char *grown;

            capacity = capacity != 0 ? capacity * 2 : 16384;
            grown = capacity <= RESPONSE_MAX ? realloc(text, capacity) : NULL;
            if (grown == NULL) break;
            text = grown;
        }
        if (!WinHttpReadData(request, text + length, 4096, &received)) break;
        if (received == 0)
        {
            text[length] = '\0';
            return text;
        }
        length += received;
    }
    free(text);
    return NULL;
}

static bool send_request(const char *function, const char *body, int *http_status, char **response)
{
    wchar_t url[512], host[256], base_path[256], path[512], headers[640];
    URL_COMPONENTS parts = {0};
    HINTERNET session = NULL, connection = NULL, request = NULL;
    DWORD status = 0, size = sizeof(status), length = (DWORD)strlen(body);
    size_t base_length;
    bool ok = false;

    *response = NULL;
    *http_status = 0;
    if (MultiByteToWideChar(CP_UTF8, 0, IARA_NUVEM_URL, -1, url, 512) == 0) return false;

    parts.dwStructSize = sizeof(parts);
    parts.lpszHostName = host;
    parts.dwHostNameLength = 256;
    parts.lpszUrlPath = base_path;
    parts.dwUrlPathLength = 256;
    // Só HTTPS: a chave e a senha nunca viajam abertas
    if (!WinHttpCrackUrl(url, 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) return false;

    base_length = wcslen(base_path);
    while (base_length > 0 && base_path[base_length - 1] == L'/') base_path[--base_length] = L'\0';
    if (_snwprintf(path, 512, L"%ls/rest/v1/rpc/%hs", base_path, function) < 0) return false;
    path[511] = L'\0';
    // A chave "publishable" não é um JWT: vai só no cabeçalho apikey
    if (_snwprintf(headers, 640, L"apikey: %hs\r\nContent-Type: application/json\r\n", IARA_NUVEM_CHAVE) < 0) return false;
    headers[639] = L'\0';

    session = WinHttpOpen(L"IARA", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session != NULL)
    {
        WinHttpSetTimeouts(session, TIMEOUT_RESOLVE_MS, TIMEOUT_CONNECT_MS, TIMEOUT_TRANSFER_MS, TIMEOUT_TRANSFER_MS);
        connection = WinHttpConnect(session, host, parts.nPort, 0);
    }
    if (connection != NULL)
    {
        request = WinHttpOpenRequest(connection, L"POST", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    }
    if (request != NULL &&
        WinHttpSendRequest(request, headers, (DWORD)-1L, (LPVOID)body, length, length, 0) &&
        WinHttpReceiveResponse(request, NULL) &&
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                            &status, &size, WINHTTP_NO_HEADER_INDEX))
    {
        *response = read_response(request);
        ok = *response != NULL;
        *http_status = ok ? (int)status : -(int)GetLastError();
    }
    else *http_status = -(int)GetLastError();   // Antes de fechar os handles, que trocam o erro
    if (request != NULL) WinHttpCloseHandle(request);
    if (connection != NULL) WinHttpCloseHandle(connection);
    if (session != NULL) WinHttpCloseHandle(session);
    return ok;
}

cloud_transport cloud_http_transport(void)
{
    return IARA_NUVEM_URL[0] != '\0' && IARA_NUVEM_CHAVE[0] != '\0' ? send_request : NULL;
}

#else

cloud_transport cloud_http_transport(void)
{
    return NULL;
}

#endif
