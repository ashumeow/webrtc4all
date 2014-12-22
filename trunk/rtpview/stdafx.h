// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <winsock2.h>
#include <IPHlpApi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional> 
#include <cctype>
#include <locale>
#include <regex>
#include <codecvt>

// define common inline functions for convenience here (not best practice to do in stdafx.h)
//using namespace std;

#ifndef UNICODE  
typedef std::string TString; 
typedef std::tr1::regex  TRegex; 
typedef std::codecvt_utf8<char> TConvertUtf8;
#else
typedef std::wstring TString; 
typedef std::tr1::wregex  TRegex; 
typedef std::codecvt_utf8<wchar_t> TConvertUtf8;
#endif


// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

// tolower for strings
static inline std::string &tolowercase(std::string &s) {
    std::transform(s.begin(), s.end(), s.begin(), 
               std::bind2nd(std::ptr_fun(&std::tolower<char>), std::locale("")));
    return s;
}

// contains
static inline bool contains(const std::string& s, const std::string& w)
{
    size_t pos = 0;
    while ((pos = s.substr(pos).find(w)) != std::string::npos) {
        if (!(isalpha(s[pos - 1])) || !(isalpha(s[pos + w.size() + 1])))
            return true;
    }
    return false;
}
