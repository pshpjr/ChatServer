#pragma once

#include <curl/curl.h>

#ifdef _DEBUG
#pragma comment(lib, "libcurld.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif

#pragma comment(lib, "dpp.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "CRYPT32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "libcryptoMTd.lib")
#pragma comment(lib, "libsslMTd.lib")

#include <format>
#include <string>
#include <Types.h>

class WebHook
{
public:
    WebHook(const WebHook& other) = delete;
    WebHook(WebHook&& other) = delete;
    WebHook& operator=(const WebHook& other) = delete;
    WebHook& operator=(WebHook&& other) = delete;

    explicit WebHook(const String& url)
        : _curl(curl_easy_init())
        , _res()
        , _url(url)
    {
    }


    ~WebHook()
    {
        curl_easy_cleanup(_curl);
    }

    bool Send(String title, String description)
    {
        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        const auto sendStr = std::vformat(L"{\""
            L"content\": \"\","
            L"\"embeds\" : "
            L"[{\"title\": \"{:s}\","
            L"\"description\" : \"{:s}\""
            L"}]}", std::make_wformat_args(title, description));

        curl_easy_setopt(_curl, CURLOPT_URL, _url.c_str());

        // Set the custom headers
        curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // Set the JSON data to be sent
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, sendStr.c_str());

        // Perform the HTTP POST request
        _res = curl_easy_perform(_curl);

        // Check for errors
        if (_res != CURLE_OK)
        {
            return false;
        }

        // Cleanup
        curl_slist_free_all(headers);
        return true;
    }

private:
    CURL* _curl;
    CURLcode _res;
    String _url;
};
