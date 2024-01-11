#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <stdio.h>
#include <format>
#include <regex>
#include <iostream>
#include <fstream>
#include <string>
#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>

enum eSearchType
{
    eSearchType_URL,            // URL(前方一致)
    eSearchType_URLRegex,       // URL全体(正規表現)
    eSearchType_Domain,         // ドメイン(完全一致)
    eSearchType_DomainRegex,    // ドメイン(正規表現)
};

struct stSearchInfo
{
    eSearchType type;
    std::string text;
    std::regex reg;
};


bool isMatch(const std::string& url, const stSearchInfo& info)
{
    std::smatch m;
    std::string domain;
    if (std::regex_search(url, m, std::regex(R"(^[a-z]+:\/{2,3}([0-9a-z\.\-:]+?):?[0-9]*?\/)"))) {
        domain = m[1].str();
    }

    switch (info.type)
    {
    case eSearchType_URL:
        return url.starts_with(info.text);
    case eSearchType_URLRegex:
        return std::regex_match(url, info.reg);
    case eSearchType_Domain:
        return domain == info.text;
    case eSearchType_DomainRegex:
        return std::regex_match(domain, info.reg);
    }

    return false;
}

void launchURLBrowser(const std::wstring& exe, const wchar_t* url)
{
    if (exe.ends_with(L":")) {
        auto cmd = std::format(L"{0}{1}", exe, url);
        ShellExecuteW(NULL, L"open", cmd.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else {
        auto cmd = std::format(L"\"{0}\" {1}", exe, url);

        STARTUPINFO si{};
        PROCESS_INFORMATION pi{};

        si.cb = sizeof(si);

        auto szCmdline = _tcsdup(cmd.c_str());
        if (CreateProcessW(nullptr, szCmdline, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        free(szCmdline);
    }
	
}

std::string to_string(const wchar_t* str)
{
    char utf8str[1000];
    memset(utf8str, 0, sizeof(utf8str));
    WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8str, sizeof(utf8str), nullptr, nullptr);
    utf8str[sizeof(utf8str) - 1] = '\0';
    return std::string(utf8str);
}

std::wstring to_wstring(const c4::csubstr& str)
{
    wchar_t wideStr[1000];
    memset(wideStr, 0, sizeof(wideStr));
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), wideStr, sizeof(wideStr) / sizeof(wideStr[0]));
    wideStr[sizeof(wideStr) / sizeof(wideStr[0]) - 1] = L'\0';
    return std::wstring(wideStr);
}

std::wstring getModuleDir()
{
    std::wstring ret;

    TCHAR path[MAX_PATH];
    if (::GetModuleFileName(NULL, path, MAX_PATH)) {
        TCHAR* ptmp = _tcsrchr(path, _T('\\'));
        if (ptmp != NULL) {
            ptmp = _tcsinc(ptmp);
            *ptmp = _T('\0');
            ret = path;
        }
    }

    return ret;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    if (__argc <= 1) {
        return 0;
    }

    const auto wUrl = __targv[1];
    const std::string url(to_string(__targv[1]));

    std::vector<std::pair<std::wstring, std::vector<stSearchInfo>>> browserList;
    {
        std::string yaml;
        {
            const auto dir = getModuleDir();
            std::ifstream ifs(dir + L"config.yaml");
            yaml.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        }

        ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yaml));

        auto root = tree.rootref();
        browserList.reserve(root.num_children());
        for (const auto& exeChild : root.children()) {
            const auto exe = exeChild.key();
            std::vector<stSearchInfo> infoList;
            infoList.reserve(exeChild.num_children());
            for (const auto& regexChild : exeChild.children()) {
                auto val = regexChild.val();
                std::string str(val.data(), val.size());

                stSearchInfo info;
                if (str.starts_with("r:")) {
                    info.type = eSearchType_URLRegex;
                    info.reg.assign(str.c_str() + 2);
                }
                else if (str.starts_with("d:")) {
                    info.type = eSearchType_Domain;
                    info.text.assign(str.c_str() + 2);
                }
                else if (str.starts_with("dr:")) {
                    info.type = eSearchType_DomainRegex;
                    info.reg.assign(str.c_str() + 3);
                }
                else {
                    info.type = eSearchType_URL;
                    info.text.assign(str);
                }

                infoList.emplace_back(info);
            }
            browserList.emplace_back(std::make_pair(to_wstring(exe), infoList));
        }
    }

    for (const auto& p : browserList) {
        const auto& exe = p.first;
        const auto& infoList = p.second;
        for (const auto& info : infoList) {
            if (isMatch(url, info)) {
                launchURLBrowser(exe, wUrl);
                return 0;
            }
        }
    }

	return 1;
}
