//
// Created by Park on 24. 8. 22.
//

#include "Utility.h"

#include <codecvt>
#include <locale>

std::string psh::util::WToS(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}
