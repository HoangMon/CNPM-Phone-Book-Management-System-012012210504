#include "MenuUI.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "msimg32.lib")

namespace
{
    constexpr const wchar_t* kMainWndClass = L"PBMS_Main_WndClass";
    constexpr const wchar_t* kDialogWndClass = L"PBMS_Dialog_WndClass";

    // Helper to get text from an EDIT control
    std::string getEditText(HWND parent, int id)
    {
        char buffer[1024] = {};
        GetDlgItemTextA(parent, id, buffer, sizeof(buffer));
        return std::string(buffer);
    }

    std::wstring getEditTextW(HWND parent, int id)
    {
        wchar_t buffer[1024] = {};
        GetDlgItemTextW(parent, id, buffer, 1024);
        return std::wstring(buffer);
    }

    void setEditTextW(HWND parent, int id, const std::wstring& text)
    {
        SetDlgItemTextW(parent, id, text.c_str());
    }

    std::wstring toWString(const std::string& str)
    {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
        if (len <= 0) return std::wstring(str.begin(), str.end());
        std::wstring result(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], len);
        return result;
    }

    std::string toNarrowString(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        if (len <= 0) return std::string(wstr.begin(), wstr.end());
        std::string result(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], len, nullptr, nullptr);
        return result;
    }

    // GDI Drawing Primitives
    void drawRoundRect(HDC hdc, int x, int y, int w, int h, int radius, COLORREF bg, COLORREF border, int borderWidth = 1)
    {
        HPEN pen = (borderWidth > 0) ? CreatePen(PS_SOLID, borderWidth, border) : (HPEN)GetStockObject(NULL_PEN);
        HBRUSH brush = CreateSolidBrush(bg);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);

        RoundRect(hdc, x, y, x + w, y + h, radius, radius);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        if (borderWidth > 0) DeleteObject(pen);
        DeleteObject(brush);
    }

    void drawCircle(HDC hdc, int cx, int cy, int radius, COLORREF bg, COLORREF border, int borderWidth = 1)
    {
        HPEN pen = (borderWidth > 0) ? CreatePen(PS_SOLID, borderWidth, border) : (HPEN)GetStockObject(NULL_PEN);
        HBRUSH brush = CreateSolidBrush(bg);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);

        Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        if (borderWidth > 0) DeleteObject(pen);
        DeleteObject(brush);
    }

    void drawPillBadge(HDC hdc, int x, int y, int w, int h, const std::wstring& text, COLORREF bg, COLORREF border, COLORREF textCol, HFONT hFont)
    {
        drawRoundRect(hdc, x, y, w, h, h, bg, border);
        RECT rc = { x, y, x + w, y + h };
        HGDIOBJ oldFont = SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textCol);
        DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    void drawAvatarCircle(HDC hdc, int x, int y, int size, const std::string& name, HFONT hFont)
    {
        int radius = size / 2;
        int cx = x + radius;
        int cy = y + radius;
        drawCircle(hdc, cx, cy, radius, UITheme::BgLightBlue, UITheme::BorderBlue, 1);

        std::wstring initials;
        if (!name.empty())
        {
            initials += (wchar_t)std::toupper((unsigned char)name[0]);
            size_t spacePos = name.find(' ');
            if (spacePos != std::string::npos && spacePos + 1 < name.size())
            {
                initials += (wchar_t)std::toupper((unsigned char)name[spacePos + 1]);
            }
        }
        else
        {
            initials = L"?";
        }

        RECT rc = { x, y, x + size, y + size };
        HGDIOBJ oldFont = SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, UITheme::PrimaryBlue);
        DrawTextW(hdc, initials.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    void drawMetricCard(HDC hdc, int x, int y, int w, int h, const wchar_t* title, const wchar_t* value, const wchar_t* subtitle,
                        HFONT hValFont, HFONT hLblFont, HFONT hSubFont)
    {
        drawRoundRect(hdc, x, y, w, h, 8, UITheme::BgCard, UITheme::BorderLight, 1);

        SetBkMode(hdc, TRANSPARENT);

        // Title / Label
        RECT rcTitle = { x + 18, y + 16, x + w - 18, y + 36 };
        HGDIOBJ oldFont = SelectObject(hdc, hLblFont);
        SetTextColor(hdc, UITheme::TextMuted);
        DrawTextW(hdc, title, -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

        // Big Metric Value
        RECT rcVal = { x + 18, y + 40, x + 100, y + 84 };
        SelectObject(hdc, hValFont);
        SetTextColor(hdc, UITheme::TextPrimary);
        DrawTextW(hdc, value, -1, &rcVal, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Subtitle next to or below value
        RECT rcSub = { x + 65, y + 54, x + w - 18, y + 80 };
        SelectObject(hdc, hSubFont);
        SetTextColor(hdc, UITheme::TextMuted);
        DrawTextW(hdc, subtitle, -1, &rcSub, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, oldFont);
    }
}

// =========================================================================
// CONSTRUCTOR & DESTRUCTOR
// =========================================================================

MenuUI::MenuUI()
    : hWnd(nullptr),
      hFontTitleBig(nullptr), hFontDialogTitle(nullptr), hFontSubtitle(nullptr),
      hFontLabel(nullptr), hFontBody(nullptr), hFontBodyBold(nullptr),
      hFontSmall(nullptr), hFontSmallBold(nullptr), hFontMetricVal(nullptr),
      hFontIcons(nullptr), hFontIconsBig(nullptr),
      hBrushBg(nullptr), hBrushCard(nullptr), hBrushTableHeader(nullptr),
      hBrushLightBlue(nullptr), hBrushDanger(nullptr), hBrushWarning(nullptr),
      currentScreen("login"), previousScreen("login"), currentRole(""),
      selectedContactId(0), selectedGroupId(0), selectedAccountId(0),
      passwordVisible(false),
      searchQuery(""), filterGroupId(0), filterOnlyFavorites(false),
      adminUserSearchQuery(""), adminUserRoleFilter("All"), adminUserStatusFilter("All"),
      backupFilePath(""), restoreFilePath(""),
      restoreValidated(false), restoreContactsCount(0), restoreGroupsCount(0),
      restoreUsersCount(0), restoreConfirmed(false),
      accountService(db)
{
}

MenuUI::~MenuUI()
{
    destroyResources();
}

void MenuUI::createFonts()
{
    hFontTitleBig = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontDialogTitle = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontSubtitle = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontLabel = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontBody = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontBodyBold = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontSmallBold = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontMetricVal = CreateFontW(-36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    hFontIcons = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");
    hFontIconsBig = CreateFontW(-40, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");
}

void MenuUI::createBrushes()
{
    hBrushBg = CreateSolidBrush(UITheme::BgWindow);
    hBrushCard = CreateSolidBrush(UITheme::BgCard);
    hBrushTableHeader = CreateSolidBrush(UITheme::BgTableHeader);
    hBrushLightBlue = CreateSolidBrush(UITheme::BgLightBlue);
    hBrushDanger = CreateSolidBrush(UITheme::BgDanger);
    hBrushWarning = CreateSolidBrush(UITheme::BgWarning);
}

void MenuUI::destroyResources()
{
    if (hFontTitleBig) DeleteObject(hFontTitleBig);
    if (hFontDialogTitle) DeleteObject(hFontDialogTitle);
    if (hFontSubtitle) DeleteObject(hFontSubtitle);
    if (hFontLabel) DeleteObject(hFontLabel);
    if (hFontBody) DeleteObject(hFontBody);
    if (hFontBodyBold) DeleteObject(hFontBodyBold);
    if (hFontSmall) DeleteObject(hFontSmall);
    if (hFontSmallBold) DeleteObject(hFontSmallBold);
    if (hFontMetricVal) DeleteObject(hFontMetricVal);
    if (hFontIcons) DeleteObject(hFontIcons);
    if (hFontIconsBig) DeleteObject(hFontIconsBig);

    if (hBrushBg) DeleteObject(hBrushBg);
    if (hBrushCard) DeleteObject(hBrushCard);
    if (hBrushTableHeader) DeleteObject(hBrushTableHeader);
    if (hBrushLightBlue) DeleteObject(hBrushLightBlue);
    if (hBrushDanger) DeleteObject(hBrushDanger);
    if (hBrushWarning) DeleteObject(hBrushWarning);
}

void MenuUI::clearScreen()
{
    for (HWND c : screenControls)
    {
        if (IsWindow(c)) DestroyWindow(c);
    }
    screenControls.clear();
    buttonConfigs.clear();

    HWND child = GetWindow(hWnd, GW_CHILD);
    while (child)
    {
        HWND next = GetWindow(child, GW_HWNDNEXT);
        DestroyWindow(child);
        child = next;
    }
}

void MenuUI::resizeForScreen(const std::string& screen)
{
    int targetW = 900;
    int targetH = 650;

    if (screen == "contacts" || screen.rfind("admin", 0) == 0)
    {
        targetW = 1140;
        targetH = 750;
    }

    RECT rcWnd;
    GetWindowRect(hWnd, &rcWnd);
    int curW = rcWnd.right - rcWnd.left;
    int curH = rcWnd.bottom - rcWnd.top;

    if (curW != targetW || curH != targetH)
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int posX = (screenW - targetW) / 2;
        int posY = (screenH - targetH) / 2;
        SetWindowPos(hWnd, nullptr, posX, posY, targetW, targetH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void MenuUI::showScreen(const std::string& screen)
{
    previousScreen = currentScreen;
    currentScreen = screen;
    clearScreen();
    resizeForScreen(screen);

    if (screen == "login") showLogin();
    else if (screen == "register") showRegister();
    else if (screen == "forgot") showForgotPassword();
    else if (screen == "contacts") showContacts();
    else if (screen.rfind("admin", 0) == 0)
    {
        showAdminWorkspace(screen);
    }

    InvalidateRect(hWnd, nullptr, TRUE);
    UpdateWindow(hWnd);
}

// =========================================================================
// WIDGET FACTORY HELPERS
// =========================================================================

LRESULT CALLBACK MenuUI::ButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
    MenuUI* ui = reinterpret_cast<MenuUI*>(dwRefData);
    if (!ui) return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    if (uMsg == WM_MOUSEMOVE)
    {
        auto it = ui->buttonConfigs.find(hwnd);
        if (it != ui->buttonConfigs.end() && !it->second.isHot)
        {
            it->second.isHot = true;
            InvalidateRect(hwnd, nullptr, FALSE);

            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
    }
    else if (uMsg == WM_MOUSELEAVE)
    {
        auto it = ui->buttonConfigs.find(hwnd);
        if (it != ui->buttonConfigs.end() && it->second.isHot)
        {
            it->second.isHot = false;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HWND MenuUI::createStyledButton(const wchar_t* text, int x, int y, int w, int h, int id, ButtonStyle style, const wchar_t* icon)
{
    HWND btn = CreateWindowW(
        L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        x, y, w, h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);

    ButtonConfig cfg;
    cfg.style = style;
    if (icon) cfg.icon = icon;
    buttonConfigs[btn] = cfg;

    SetWindowSubclass(btn, ButtonSubclassProc, (UINT_PTR)id, (DWORD_PTR)this);
    screenControls.push_back(btn);
    return btn;
}

HWND MenuUI::createStyledEdit(const wchar_t* placeholder, int x, int y, int w, int h, int id, bool password)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | WS_BORDER;
    if (password) style |= ES_PASSWORD;

    HWND edit = CreateWindowExW(
        0, L"EDIT", L"", style,
        x, y, w, h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);

    SendMessageW(edit, WM_SETFONT, (WPARAM)hFontBody, TRUE);
    if (placeholder)
    {
        SendMessageW(edit, EM_SETCUEBANNER, TRUE, (LPARAM)placeholder);
    }

    screenControls.push_back(edit);
    return edit;
}

HWND MenuUI::createStyledCombo(int x, int y, int w, int h, int id)
{
    HWND combo = CreateWindowW(
        L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, w, h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);

    SendMessageW(combo, WM_SETFONT, (WPARAM)hFontBody, TRUE);
    screenControls.push_back(combo);
    return combo;
}

HWND MenuUI::createOwnerDrawList(int x, int y, int w, int h, int id, int itemHeight)
{
    HWND list = CreateWindowExW(
        0, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY | WS_BORDER,
        x, y, w, h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);

    SendMessageW(list, LB_SETITEMHEIGHT, 0, (LPARAM)itemHeight);
    screenControls.push_back(list);
    return list;
}

// =========================================================================
// DATA ACCESS & FILTER HELPERS
// =========================================================================

std::vector<Contact> MenuUI::getFilteredContacts() const
{
    std::vector<Contact> result;
    const auto& all = ContactService::getContacts();

    std::string lowerQ = searchQuery;
    std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);

    for (const auto& c : all)
    {
        if (filterOnlyFavorites && !c.isFavorite) continue;
        if (filterGroupId != 0 && c.group_id != filterGroupId) continue;

        if (!lowerQ.empty())
        {
            std::string n = c.name, p = c.phone, e = c.email;
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            std::transform(p.begin(), p.end(), p.begin(), ::tolower);
            std::transform(e.begin(), e.end(), e.begin(), ::tolower);

            if (n.find(lowerQ) == std::string::npos &&
                p.find(lowerQ) == std::string::npos &&
                e.find(lowerQ) == std::string::npos)
            {
                continue;
            }
        }
        result.push_back(c);
    }
    return result;
}

std::vector<AccountData> MenuUI::getFilteredAccounts() const
{
    std::vector<AccountData> result;
    const auto all = db.getAccounts();

    std::string lowerQ = adminUserSearchQuery;
    std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);

    for (const auto& a : all)
    {
        if (adminUserRoleFilter != "All" && a.role != adminUserRoleFilter) continue;

        if (!lowerQ.empty())
        {
            std::string u = a.username, fn = a.fullname, em = a.email;
            std::transform(u.begin(), u.end(), u.begin(), ::tolower);
            std::transform(fn.begin(), fn.end(), fn.begin(), ::tolower);
            std::transform(em.begin(), em.end(), em.begin(), ::tolower);

            if (u.find(lowerQ) == std::string::npos &&
                fn.find(lowerQ) == std::string::npos &&
                em.find(lowerQ) == std::string::npos)
            {
                continue;
            }
        }
        result.push_back(a);
    }
    return result;
}

// =========================================================================
// SCREEN 1: LOGIN (Matches Login.png)
// =========================================================================

void MenuUI::showLogin()
{
    // Container coordinates:
    // Window is 900x650
    // Centered Card / Content area: X = 230, Y = 160, W = 440
    int cardX = 230;
    int curY = 270;

    // Username input
    createStyledEdit(L"Enter your username", cardX, curY, 440, 38, 1001);

    // Password input + Eye toggle button
    curY += 72;
    createStyledEdit(L"Enter your password", cardX, curY, 396, 38, 1002, !passwordVisible);
    createStyledButton(passwordVisible ? L"👁" : L"◎", cardX + 402, curY, 38, 38, 1003, ButtonStyle::Outline);

    // Login Primary Action Button
    curY += 56;
    createStyledButton(L"›  Login", cardX, curY, 440, 44, 100, ButtonStyle::Primary);

    // Links: Forgot Password & Register
    curY += 60;
    createStyledButton(L"?  Forgot Password", cardX + 10, curY, 150, 30, 102, ButtonStyle::Link);
    createStyledButton(L"+  Register", cardX + 310, curY, 120, 30, 103, ButtonStyle::Link);

    // Close button at bottom right
    createStyledButton(L"✕  Close", 770, 560, 90, 38, 104, ButtonStyle::Outline);
}

void MenuUI::drawLogin(HDC hdc, const RECT& rc)
{
    // 1. Top Header Banner
    int bannerH = 75;
    drawRoundRect(hdc, 38, 20, 40, 40, 10, UITheme::PrimaryBlue, UITheme::PrimaryBlue);

    // Icon inside blue square
    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextWhite);
    RECT rcIcon = { 38, 20, 78, 60 };
    DrawTextW(hdc, L"📇", -1, &rcIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Header Title
    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcHead = { 90, 20, 600, 60 };
    DrawTextW(hdc, L"PHONE BOOK MANAGEMENT SYSTEM", -1, &rcHead, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Divider below banner
    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, 0, bannerH, nullptr);
    LineTo(hdc, rc.right, bannerH);

    // 2. Center Content: Login Title and Subtitle
    int cardX = 230;
    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { cardX, 125, cardX + 440, 165 };
    DrawTextW(hdc, L"Login", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { cardX, 170, cardX + 440, 195 };
    DrawTextW(hdc, L"Sign in to access your contacts.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Field labels
    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcLblUser = { cardX, 245, cardX + 200, 268 };
    DrawTextW(hdc, L"Username", -1, &rcLblUser, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcLblPass = { cardX, 317, cardX + 200, 340 };
    DrawTextW(hdc, L"Password", -1, &rcLblPass, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Bottom Divider
    MoveToEx(hdc, 0, 535, nullptr);
    LineTo(hdc, rc.right, 535);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);
    DeleteObject(penDivider);
}

// =========================================================================
// SCREEN 2: REGISTER (Matches Register.png)
// =========================================================================

void MenuUI::showRegister()
{
    int startX = 140;
    int colW = 280;
    int gap = 40;
    int curY = 220;

    // Row 1: Username & Password
    createStyledEdit(L"Choose a username", startX, curY, colW, 38, 1101);
    createStyledEdit(L"Create a password", startX + colW + gap, curY, colW, 38, 1102, true);

    // Row 2: Full Name (spans 600px)
    curY += 72;
    createStyledEdit(L"Enter your full name", startX, curY, colW * 2 + gap, 38, 1103);

    // Row 3: Email & Phone
    curY += 72;
    createStyledEdit(L"Enter your email address", startX, curY, colW, 38, 1104);
    createStyledEdit(L"Enter your phone number", startX + colW + gap, curY, colW, 38, 1105);

    // Bottom Action Buttons
    createStyledButton(L"‹  Back", 500, 555, 90, 40, 1106, ButtonStyle::Outline);
    createStyledButton(L"Login", 605, 555, 90, 40, 1107, ButtonStyle::Outline);
    createStyledButton(L"Register", 710, 555, 130, 40, 1108, ButtonStyle::Primary);
}

void MenuUI::drawRegister(HDC hdc, const RECT& rc)
{
    // Top Banner
    drawRoundRect(hdc, 38, 20, 40, 40, 10, UITheme::PrimaryBlue, UITheme::PrimaryBlue);
    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextWhite);
    RECT rcIcon = { 38, 20, 78, 60 };
    DrawTextW(hdc, L"📇", -1, &rcIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcHead = { 90, 20, 600, 60 };
    DrawTextW(hdc, L"PHONE BOOK MANAGEMENT SYSTEM", -1, &rcHead, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, 0, 75, nullptr);
    LineTo(hdc, rc.right, 75);

    // Title & Subtitle
    int startX = 140;
    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { startX, 110, startX + 600, 145 };
    DrawTextW(hdc, L"Register An Account", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { startX, 150, startX + 600, 175 };
    DrawTextW(hdc, L"Create your account to start organizing contacts.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Labels
    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);

    RECT rcL1 = { startX, 195, startX + 280, 218 };
    DrawTextW(hdc, L"Username", -1, &rcL1, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL2 = { startX + 320, 195, startX + 600, 218 };
    DrawTextW(hdc, L"Password", -1, &rcL2, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL3 = { startX, 267, startX + 600, 290 };
    DrawTextW(hdc, L"Full Name", -1, &rcL3, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL4 = { startX, 339, startX + 280, 362 };
    DrawTextW(hdc, L"Email", -1, &rcL4, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL5 = { startX + 320, 339, startX + 600, 362 };
    DrawTextW(hdc, L"Phone", -1, &rcL5, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Bottom Divider
    MoveToEx(hdc, 0, 530, nullptr);
    LineTo(hdc, rc.right, 530);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);
    DeleteObject(penDivider);
}

// =========================================================================
// SCREEN 3: FORGOT PASSWORD (Matches Reset password.png)
// =========================================================================

void MenuUI::showForgotPassword()
{
    int startX = 140;
    int colW = 280;
    int gap = 40;
    int curY = 220;

    // Row 1: Username & Email
    createStyledEdit(L"Enter your username", startX, curY, colW, 38, 1201);
    createStyledEdit(L"Enter your email address", startX + colW + gap, curY, colW, 38, 1202);

    // Row 2: New Password & Confirm Password
    curY += 72;
    createStyledEdit(L"Enter a new password", startX, curY, colW, 38, 1203, true);
    createStyledEdit(L"Re-enter new password", startX + colW + gap, curY, colW, 38, 1204, true);

    // Bottom Action Buttons
    createStyledButton(L"‹  Back to Login", 510, 550, 140, 40, 1205, ButtonStyle::Outline);
    createStyledButton(L"Reset Password", 665, 550, 170, 40, 1206, ButtonStyle::Primary);
}

void MenuUI::drawForgotPassword(HDC hdc, const RECT& rc)
{
    // Top Banner
    drawRoundRect(hdc, 38, 20, 40, 40, 10, UITheme::PrimaryBlue, UITheme::PrimaryBlue);
    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextWhite);
    RECT rcIcon = { 38, 20, 78, 60 };
    DrawTextW(hdc, L"📇", -1, &rcIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcHead = { 90, 20, 600, 60 };
    DrawTextW(hdc, L"PHONE BOOK MANAGEMENT SYSTEM", -1, &rcHead, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, 0, 75, nullptr);
    LineTo(hdc, rc.right, 75);

    // Title & Subtitle
    int startX = 140;
    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { startX, 110, startX + 600, 145 };
    DrawTextW(hdc, L"Reset Password", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { startX, 150, startX + 600, 175 };
    DrawTextW(hdc, L"Enter your account details and choose a new password.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Labels
    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);

    RECT rcL1 = { startX, 195, startX + 280, 218 };
    DrawTextW(hdc, L"Username", -1, &rcL1, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL2 = { startX + 320, 195, startX + 600, 218 };
    DrawTextW(hdc, L"Email", -1, &rcL2, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL3 = { startX, 267, startX + 280, 290 };
    DrawTextW(hdc, L"New Password", -1, &rcL3, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcL4 = { startX + 320, 267, startX + 600, 290 };
    DrawTextW(hdc, L"Confirm Password", -1, &rcL4, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Info notice callout
    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcInfo = { startX, 350, startX + 600, 380 };
    DrawTextW(hdc, L"ⓘ  Both password fields must match.", -1, &rcInfo, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Bottom Divider
    MoveToEx(hdc, 0, 525, nullptr);
    LineTo(hdc, rc.right, 525);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);
    DeleteObject(penDivider);
}

// =========================================================================
// SCREEN 4: CONTACTS DASHBOARD (Matches Contacts management dashboard.png)
// =========================================================================

void MenuUI::showContacts()
{
    // Toolbar Row 1: Search, Filter, Actions
    int tbY = 110;
    createStyledEdit(L"Search contacts...", 38, tbY, 200, 38, 1301);
    createStyledButton(L"Search", 246, tbY, 70, 38, 1302, ButtonStyle::Outline);
    createStyledButton(L"+ Add Contact", 324, tbY, 120, 38, 1303, ButtonStyle::Primary);

    std::wstring favBtnText = filterOnlyFavorites ? L"★ Favorites" : L"☆ Favorites (FR-10)";
    createStyledButton(favBtnText.c_str(), 452, tbY, 150, 38, 1304, ButtonStyle::Outline);

    // Group Filter ComboBox
    HWND combo = createStyledCombo(610, tbY, 220, 250, 1305);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"All Groups");
    const auto& groups = GroupService::getGroups();
    for (size_t i = 0; i < groups.size(); ++i)
    {
        std::wstring gName = L"Group: " + toWString(groups[i].name);
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)gName.c_str());
        if (groups[i].id == filterGroupId)
        {
            SendMessageW(combo, CB_SETCURSEL, (WPARAM)(i + 1), 0);
        }
    }
    if (filterGroupId == 0)
    {
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
    }

    createStyledButton(L"⚙ Manage Groups (FR-11/12)", 838, tbY, 230, 38, 1306, ButtonStyle::Outline);

    // Sub-Action Toolbar Row 2
    int actY = 162;
    createStyledButton(L"👁 View", 38, actY, 75, 34, 1307, ButtonStyle::Outline);
    createStyledButton(L"✏ Edit", 119, actY, 70, 34, 1308, ButtonStyle::Outline);
    createStyledButton(L"🗑 Delete", 195, actY, 85, 34, 1309, ButtonStyle::Outline);
    createStyledButton(L"☆ Favorite", 286, actY, 95, 34, 1310, ButtonStyle::Outline);

    // Back / Logout button on right
    createStyledButton(L"‹  Back", 1010, actY, 80, 34, 1311, ButtonStyle::Outline);

    // Contacts Table List
    auto filtered = getFilteredContacts();
    int tableY = 246; // Table starts below header row
    int tableH = 430;

    if (!filtered.empty())
    {
        HWND list = createOwnerDrawList(38, tableY, 1052, tableH, 1312, 52);
        for (const auto& c : filtered)
        {
            SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)c.name.c_str());
        }
        // Select matching item if any
        for (size_t i = 0; i < filtered.size(); ++i)
        {
            if (filtered[i].id == selectedContactId)
            {
                SendMessageW(list, LB_SETCURSEL, (WPARAM)i, 0);
                break;
            }
        }
    }
    else
    {
        // When empty, add centered "+ Add Contact" button
        createStyledButton(L"+ Add Contact", 480, 480, 130, 42, 1303, ButtonStyle::Primary);
    }

    // If Admin, show "Admin Workspace" pill button on the top right
    if (currentRole == "Admin")
    {
        createStyledButton(L"Admin Workspace ›", 940, 26, 150, 36, 1313, ButtonStyle::Outline);
    }
}

void MenuUI::drawContactsDashboard(HDC hdc, const RECT& rc)
{
    // 1. Top Header
    drawRoundRect(hdc, 38, 22, 44, 44, 10, UITheme::BgLightBlue, UITheme::BorderBlue);
    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::PrimaryBlue);
    RECT rcIcon = { 38, 22, 82, 66 };
    DrawTextW(hdc, L"📇", -1, &rcIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { 94, 16, 500, 52 };
    DrawTextW(hdc, L"Contacts", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { 94, 52, 600, 75 };
    DrawTextW(hdc, L"Your phone book, organized in one place.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Top Right Stats: "X contacts | Y favorites"
    int totalCount = (int)ContactService::getContacts().size();
    int favCount = 0;
    for (const auto& c : ContactService::getContacts()) if (c.isFavorite) favCount++;
    std::wstring stats = std::to_wstring(totalCount) + L" contacts    |    " + std::to_wstring(favCount) + L" favorites";

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    int statsRight = (currentRole == "Admin") ? 920 : 1090;
    RECT rcStats = { statsRight - 260, 28, statsRight, 60 };
    DrawTextW(hdc, stats.c_str(), -1, &rcStats, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    // Sub-action toolbar hint
    RECT rcHint = { 400, 162, 800, 196 };
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextMuted);
    if (selectedContactId != 0)
    {
        for (const auto& c : ContactService::getContacts())
        {
            if (c.id == selectedContactId)
            {
                std::wstring selText = L"Selected: " + toWString(c.name);
                DrawTextW(hdc, selText.c_str(), -1, &rcHint, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                break;
            }
        }
    }
    else
    {
        DrawTextW(hdc, L"Select a contact to view available actions.", -1, &rcHint, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // 2. Table Outer Container
    int tableX = 38;
    int tableY = 208;
    int tableW = 1052;
    int tableH = 472;
    drawRoundRect(hdc, tableX, tableY, tableW, tableH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    // Table Column Header Bar
    int headH = 38;
    drawRoundRect(hdc, tableX, tableY, tableW, headH, 8, UITheme::BgTableHeader, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextBody);

    RECT rcCh = { tableX + 16, tableY, tableX + 46, tableY + headH };
    DrawTextW(hdc, L"☐", -1, &rcCh, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcAv = { tableX + 50, tableY, tableX + 110, tableY + headH };
    DrawTextW(hdc, L"Avatar", -1, &rcAv, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcNm = { tableX + 130, tableY, tableX + 320, tableY + headH };
    DrawTextW(hdc, L"Name  ↑", -1, &rcNm, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcPh = { tableX + 330, tableY, tableX + 500, tableY + headH };
    DrawTextW(hdc, L"Phone", -1, &rcPh, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcEm = { tableX + 510, tableY, tableX + 740, tableY + headH };
    DrawTextW(hdc, L"Email", -1, &rcEm, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcGr = { tableX + 750, tableY, tableX + 920, tableY + headH };
    DrawTextW(hdc, L"Group", -1, &rcGr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcFv = { tableX + 930, tableY, tableX + 1040, tableY + headH };
    DrawTextW(hdc, L"Favorite", -1, &rcFv, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // 3. If contacts are empty, draw the Empty State illustration in the table container
    auto filtered = getFilteredContacts();
    if (filtered.empty())
    {
        int midX = tableX + tableW / 2;
        int midY = tableY + 160;

        // Big Circle Icon
        drawCircle(hdc, midX, midY, 36, UITheme::BgLightBlue, UITheme::BorderBlue, 1);
        SelectObject(hdc, hFontIconsBig);
        SetTextColor(hdc, UITheme::PrimaryBlue);
        RECT rcEmptyIcon = { midX - 36, midY - 36, midX + 36, midY + 36 };
        DrawTextW(hdc, L"📇", -1, &rcEmptyIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hFontDialogTitle);
        SetTextColor(hdc, UITheme::TextPrimary);
        RECT rcEmptyTitle = { midX - 250, midY + 50, midX + 250, midY + 80 };
        DrawTextW(hdc, L"No contacts yet.", -1, &rcEmptyTitle, DT_CENTER | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hFontSubtitle);
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcEmptySub = { midX - 250, midY + 84, midX + 250, midY + 108 };
        DrawTextW(hdc, L"Add your first contact to get started.", -1, &rcEmptySub, DT_CENTER | DT_TOP | DT_SINGLELINE);

        RECT rcEmptyFoot = { midX - 300, midY + 175, midX + 300, midY + 200 };
        DrawTextW(hdc, L"Keep phone numbers, emails and groups together.", -1, &rcEmptyFoot, DT_CENTER | DT_TOP | DT_SINGLELINE);
    }

    // 4. Bottom Footer Status Bar
    int footY = tableY + tableH + 12;
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);

    int selCount = (selectedContactId != 0) ? 1 : 0;
    std::wstring footStatus = std::to_wstring(filtered.size()) + L" contacts • " + std::to_wstring(selCount) + L" selected";
    RECT rcFootLeft = { tableX, footY, tableX + 300, footY + 25 };
    DrawTextW(hdc, footStatus.c_str(), -1, &rcFootLeft, DT_LEFT | DT_TOP | DT_SINGLELINE);

    std::wstring grpName = L"All groups";
    if (filterGroupId != 0)
    {
        for (const auto& g : GroupService::getGroups())
        {
            if (g.id == filterGroupId) { grpName = toWString(g.name); break; }
        }
    }
    RECT rcFootRight = { tableX + tableW - 300, footY, tableX + tableW, footY + 25 };
    DrawTextW(hdc, grpName.c_str(), -1, &rcFootRight, DT_RIGHT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

// =========================================================================
// SCREEN 5: ADMIN WORKSPACE & SUBVIEWS (Matches Admin • ... images)
// =========================================================================

void MenuUI::showAdminWorkspace(const std::string& subview)
{
    // Sidebar items
    int sideX = 18;
    int sideW = 184;

    createStyledButton(L"01   User management", sideX, 135, sideW, 44, 2001,
                       (subview == "admin_users" || subview == "admin") ? ButtonStyle::NavItemActive : ButtonStyle::NavItem);

    createStyledButton(L"02   System reports", sideX, 185, sideW, 44, 2002,
                       (subview == "admin_reports") ? ButtonStyle::NavItemActive : ButtonStyle::NavItem);

    createStyledButton(L"03   Backup data", sideX, 235, sideW, 44, 2003,
                       (subview == "admin_backup") ? ButtonStyle::NavItemActive : ButtonStyle::NavItem);

    createStyledButton(L"04   Data recovery", sideX, 285, sideW, 44, 2004,
                       (subview == "admin_recovery") ? ButtonStyle::NavItemActive : ButtonStyle::NavItem);

    // Sidebar bottom back button
    createStyledButton(L"‹  Back to contacts", sideX, 650, sideW, 36, 2005, ButtonStyle::Link);

    // Subview specific controls
    int contentX = 250;

    if (subview == "admin_users" || subview == "admin")
    {
        // Search & Filter Toolbar
        createStyledEdit(L"Search username or email...", contentX, 195, 260, 38, 2101);
        createStyledButton(L"Search", contentX + 270, 195, 70, 38, 2102, ButtonStyle::Outline);

        HWND roleCombo = createStyledCombo(contentX + 350, 195, 120, 200, 2103);
        SendMessageW(roleCombo, CB_ADDSTRING, 0, (LPARAM)L"Role: All");
        SendMessageW(roleCombo, CB_ADDSTRING, 0, (LPARAM)L"User");
        SendMessageW(roleCombo, CB_ADDSTRING, 0, (LPARAM)L"Admin");
        SendMessageW(roleCombo, CB_SETCURSEL, 0, 0);

        HWND statusCombo = createStyledCombo(contentX + 480, 195, 120, 200, 2104);
        SendMessageW(statusCombo, CB_ADDSTRING, 0, (LPARAM)L"Status: All");
        SendMessageW(statusCombo, CB_ADDSTRING, 0, (LPARAM)L"Active");
        SendMessageW(statusCombo, CB_ADDSTRING, 0, (LPARAM)L"Inactive");
        SendMessageW(statusCombo, CB_SETCURSEL, 0, 0);

        createStyledButton(L"+ Add user", contentX + 710, 195, 130, 38, 2105, ButtonStyle::Primary);

        // User list
        auto accounts = getFilteredAccounts();
        if (!accounts.empty())
        {
            HWND list = createOwnerDrawList(contentX, 285, 840, 300, 2106, 48);
            for (const auto& a : accounts)
            {
                SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)a.username.c_str());
            }
        }
        else
        {
            createStyledButton(L"+ Add user", contentX + 350, 430, 130, 42, 2105, ButtonStyle::Primary);
        }

        // Action toolbar at bottom
        int actY = 600;
        createStyledButton(L"Edit user", contentX, actY, 90, 36, 2107, ButtonStyle::Outline);
        createStyledButton(L"Reset password", contentX + 98, actY, 130, 36, 2108, ButtonStyle::Outline);
        createStyledButton(L"Disable account", contentX + 236, actY, 130, 36, 2109, ButtonStyle::Outline);
        createStyledButton(L"Delete user", contentX + 374, actY, 100, 36, 2110, ButtonStyle::Outline);
    }
    else if (subview == "admin_reports")
    {
        // Filters: Report period, Report type, Apply filters, Export CSV
        HWND periodCombo = createStyledCombo(contentX + 16, 125, 150, 200, 2201);
        SendMessageW(periodCombo, CB_ADDSTRING, 0, (LPARAM)L"All time");
        SendMessageW(periodCombo, CB_ADDSTRING, 0, (LPARAM)L"Last 7 days");
        SendMessageW(periodCombo, CB_ADDSTRING, 0, (LPARAM)L"Last 30 days");
        SendMessageW(periodCombo, CB_SETCURSEL, 0, 0);

        HWND typeCombo = createStyledCombo(contentX + 180, 125, 180, 200, 2202);
        SendMessageW(typeCombo, CB_ADDSTRING, 0, (LPARAM)L"System overview");
        SendMessageW(typeCombo, CB_ADDSTRING, 0, (LPARAM)L"User activity");
        SendMessageW(typeCombo, CB_ADDSTRING, 0, (LPARAM)L"Contacts report");
        SendMessageW(typeCombo, CB_SETCURSEL, 0, 0);

        createStyledButton(L"Apply filters", contentX + 375, 125, 110, 36, 2203, ButtonStyle::Outline);
        createStyledButton(L"Export CSV", contentX + 720, 125, 120, 36, 2204, ButtonStyle::Outline);
    }
    else if (subview == "admin_backup")
    {
        // Create backup section
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
        if (backupFilePath.empty())
        {
            backupFilePath = (dir / "backups").string();
        }

        createStyledEdit(toWString(backupFilePath).c_str(), contentX + 20, 235, 360, 38, 2301);
        createStyledButton(L"Browse...", contentX + 390, 235, 100, 38, 2302, ButtonStyle::Outline);
        createStyledButton(L"Create backup", contentX + 340, 310, 150, 42, 2303, ButtonStyle::Primary);

        // Backup History
        createStyledButton(L"Refresh", contentX + 750, 410, 90, 34, 2304, ButtonStyle::Outline);
    }
    else if (subview == "admin_recovery")
    {
        // Recovery Section: Backup archive selection
        createStyledEdit(toWString(restoreFilePath).c_str(), contentX + 20, 245, 360, 38, 2401);
        createStyledButton(L"Browse...", contentX + 390, 245, 100, 38, 2402, ButtonStyle::Primary);
        createStyledButton(L"Validate backup", contentX + 20, 320, 140, 38, 2403, ButtonStyle::Outline);

        // Bottom Actions: Cancel & Restore data
        createStyledButton(L"Cancel", contentX + 570, 645, 90, 40, 2404, ButtonStyle::Outline);
        createStyledButton(L"Restore data", contentX + 670, 645, 170, 40, 2405, ButtonStyle::Outline);
    }
}

void MenuUI::drawAdminSidebar(HDC hdc, const RECT& rc)
{
    int sideW = 220;

    // Sidebar Background
    RECT rcSide = { 0, 0, sideW, rc.bottom };
    FillRect(hdc, &rcSide, hBrushBg);

    // Sidebar Right Border
    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, sideW, 0, nullptr);
    LineTo(hdc, sideW, rc.bottom);

    // Header Branding
    HGDIOBJ oldFont = SelectObject(hdc, hFontBodyBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcBrand = { 24, 24, sideW - 20, 45 };
    DrawTextW(hdc, L"PHONE BOOK", -1, &rcBrand, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { 24, 45, sideW - 20, 65 };
    DrawTextW(hdc, L"Administration", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Pill Badge: "ADMIN WORKSPACE"
    drawPillBadge(hdc, 24, 76, 145, 26, L"ADMIN WORKSPACE", UITheme::BgLightBlue, UITheme::BorderBlue, UITheme::PrimaryBlue, hFontSmallBold);

    // Bottom Note: "Admin access only"
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextLight);
    RECT rcFoot = { 24, rc.bottom - 45, sideW - 20, rc.bottom - 20 };
    DrawTextW(hdc, L"Admin access only", -1, &rcFoot, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldFont);
    DeleteObject(penDivider);
}

void MenuUI::drawAdminUsersView(HDC hdc, const RECT&)
{
    int contentX = 250;

    // Breadcrumb
    HGDIOBJ oldFont = SelectObject(hdc, hFontSmallBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcBrd = { contentX, 24, contentX + 500, 42 };
    DrawTextW(hdc, L"ADMINISTRATION  /  USER MANAGEMENT", -1, &rcBrd, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Page Title & Subtitle
    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { contentX, 45, contentX + 500, 80 };
    DrawTextW(hdc, L"User management", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { contentX, 85, contentX + 600, 110 };
    DrawTextW(hdc, L"Manage accounts, roles and access to your phone book.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // 3 Metric Cards
    auto allAccounts = db.getAccounts();
    int totalAccounts = (int)allAccounts.size();
    int adminCount = 0;
    for (const auto& a : allAccounts) if (a.role == "Admin") adminCount++;
    int activeCount = totalAccounts;

    int cardW = 265;
    int cardH = 92;
    int cardY = 118;

    drawMetricCard(hdc, contentX, cardY, cardW, cardH, L"Total accounts",
                   toWString(std::to_string(totalAccounts)).c_str(), L"registered accounts",
                   hFontMetricVal, hFontLabel, hFontSmall);

    drawMetricCard(hdc, contentX + cardW + 20, cardY, cardW, cardH, L"Active accounts",
                   toWString(std::to_string(activeCount)).c_str(), L"enabled accounts",
                   hFontMetricVal, hFontLabel, hFontSmall);

    drawMetricCard(hdc, contentX + (cardW + 20) * 2, cardY, cardW, cardH, L"Administrators",
                   toWString(std::to_string(adminCount)).c_str(), L"admin accounts",
                   hFontMetricVal, hFontLabel, hFontSmall);

    // Table Container
    int tableY = 245;
    int tableW = 840;
    int tableH = 345;
    drawRoundRect(hdc, contentX, tableY, tableW, tableH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    // Table Header Bar
    int headH = 38;
    drawRoundRect(hdc, contentX, tableY, tableW, headH, 8, UITheme::BgTableHeader, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextBody);

    RECT rcCh = { contentX + 16, tableY, contentX + 46, tableY + headH };
    DrawTextW(hdc, L"☐", -1, &rcCh, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcUn = { contentX + 50, tableY, contentX + 180, tableY + headH };
    DrawTextW(hdc, L"Username", -1, &rcUn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcFn = { contentX + 190, tableY, contentX + 340, tableY + headH };
    DrawTextW(hdc, L"Full name", -1, &rcFn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcEm = { contentX + 350, tableY, contentX + 540, tableY + headH };
    DrawTextW(hdc, L"Email", -1, &rcEm, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcRl = { contentX + 550, tableY, contentX + 660, tableY + headH };
    DrawTextW(hdc, L"Role", -1, &rcRl, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcSt = { contentX + 670, tableY, contentX + 760, tableY + headH };
    DrawTextW(hdc, L"Status", -1, &rcSt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT rcAc = { contentX + 770, tableY, contentX + 830, tableY + headH };
    DrawTextW(hdc, L"Actions", -1, &rcAc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Empty state if no accounts
    auto filtered = getFilteredAccounts();
    if (filtered.empty())
    {
        int midX = contentX + tableW / 2;
        int midY = tableY + 120;

        SelectObject(hdc, hFontDialogTitle);
        SetTextColor(hdc, UITheme::TextPrimary);
        RECT rcEmptyTitle = { midX - 250, midY, midX + 250, midY + 30 };
        DrawTextW(hdc, L"No user accounts to display.", -1, &rcEmptyTitle, DT_CENTER | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hFontSubtitle);
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcEmptySub = { midX - 250, midY + 34, midX + 250, midY + 60 };
        DrawTextW(hdc, L"Create an account to manage roles and system access.", -1, &rcEmptySub, DT_CENTER | DT_TOP | DT_SINGLELINE);
    }

    // Action Hint text
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcHint = { contentX + 490, 600, contentX + 840, 636 };
    DrawTextW(hdc, L"Select an account to enable actions.", -1, &rcHint, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Footer
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcFootL = { contentX, 695, contentX + 300, 720 };
    DrawTextW(hdc, L"Phone Book Management System", -1, &rcFootL, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcFootR = { contentX + tableW - 300, 695, contentX + tableW, 720 };
    DrawTextW(hdc, L"Administrator workspace", -1, &rcFootR, DT_RIGHT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::drawAdminReportsView(HDC hdc, const RECT&)
{
    int contentX = 250;

    HGDIOBJ oldFont = SelectObject(hdc, hFontSmallBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcBrd = { contentX, 24, contentX + 500, 42 };
    DrawTextW(hdc, L"ADMINISTRATION  /  SYSTEM REPORTS", -1, &rcBrd, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { contentX, 45, contentX + 500, 80 };
    DrawTextW(hdc, L"System reports", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { contentX, 85, contentX + 600, 110 };
    DrawTextW(hdc, L"Review usage and activity across the system.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // 4 Metric Cards in a row
    int cardW = 195;
    int cardH = 88;
    int cardY = 175;
    int gap = 20;

    int userCount = (int)db.getAccounts().size();
    int contactCount = (int)ContactService::getContacts().size();
    int groupCount = (int)GroupService::getGroups().size();
    int backupCount = std::filesystem::exists(db.getStorageFile() + ".backup") ? 1 : 0;

    drawMetricCard(hdc, contentX, cardY, cardW, cardH, L"Users",
                   toWString(std::to_string(userCount)).c_str(), L"total accounts",
                   hFontMetricVal, hFontLabel, hFontSmall);

    drawMetricCard(hdc, contentX + (cardW + gap), cardY, cardW, cardH, L"Contacts",
                   toWString(std::to_string(contactCount)).c_str(), L"saved contacts",
                   hFontMetricVal, hFontLabel, hFontSmall);

    drawMetricCard(hdc, contentX + (cardW + gap) * 2, cardY, cardW, cardH, L"Groups",
                   toWString(std::to_string(groupCount)).c_str(), L"created groups",
                   hFontMetricVal, hFontLabel, hFontSmall);

    drawMetricCard(hdc, contentX + (cardW + gap) * 3, cardY, cardW, cardH, L"Backups",
                   toWString(std::to_string(backupCount)).c_str(), L"available backups",
                   hFontMetricVal, hFontLabel, hFontSmall);

    // Middle Panels: System activity (Left) & Data overview (Right)
    int panelY = 280;
    int panelH = 200;
    int leftW = 520;
    int rightW = 300;

    // Left Panel: System Activity
    drawRoundRect(hdc, contentX, panelY, leftW, panelH, 8, UITheme::BgCard, UITheme::BorderLight, 1);
    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcActT = { contentX + 20, panelY + 16, contentX + leftW - 20, panelY + 40 };
    DrawTextW(hdc, L"System activity", -1, &rcActT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcActS = { contentX + 20, panelY + 40, contentX + leftW - 20, panelY + 65 };
    DrawTextW(hdc, L"Account and contact activity in the selected period", -1, &rcActS, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcNoAct = { contentX + 20, panelY + 105, contentX + leftW - 20, panelY + 130 };
    DrawTextW(hdc, L"No activity recorded yet.", -1, &rcNoAct, DT_CENTER | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcNoActS = { contentX + 20, panelY + 130, contentX + leftW - 20, panelY + 155 };
    DrawTextW(hdc, L"Activity will appear as the system is used.", -1, &rcNoActS, DT_CENTER | DT_TOP | DT_SINGLELINE);

    // Right Panel: Data Overview
    int rightX = contentX + leftW + 20;
    drawRoundRect(hdc, rightX, panelY, rightW, panelH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcOvT = { rightX + 20, panelY + 16, rightX + rightW - 20, panelY + 40 };
    DrawTextW(hdc, L"Data overview", -1, &rcOvT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Overview key-values
    int rowY = panelY + 55;
    SelectObject(hdc, hFontBody);
    auto drawOverviewRow = [&](const wchar_t* label, int val) {
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcL = { rightX + 20, rowY, rightX + 200, rowY + 25 };
        DrawTextW(hdc, label, -1, &rcL, DT_LEFT | DT_TOP | DT_SINGLELINE);

        SetTextColor(hdc, UITheme::TextPrimary);
        RECT rcV = { rightX + rightW - 50, rowY, rightX + rightW - 20, rowY + 25 };
        DrawTextW(hdc, toWString(std::to_string(val)).c_str(), -1, &rcV, DT_RIGHT | DT_TOP | DT_SINGLELINE);
        rowY += 32;
    };

    drawOverviewRow(L"User accounts", userCount);
    drawOverviewRow(L"Contact records", contactCount);
    drawOverviewRow(L"Contact groups", groupCount);
    drawOverviewRow(L"Backup archives", backupCount);

    // Bottom Panel: Recent activity
    int botY = panelY + panelH + 20;
    int botW = 840;
    int botH = 150;
    drawRoundRect(hdc, contentX, botY, botW, botH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcRecT = { contentX + 20, botY + 16, contentX + 300, botY + 40 };
    DrawTextW(hdc, L"Recent activity", -1, &rcRecT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Header bar
    int subHeadY = botY + 46;
    int subHeadH = 32;
    drawRoundRect(hdc, contentX, subHeadY, botW, subHeadH, 4, UITheme::BgTableHeader, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextBody);
    RECT rcH1 = { contentX + 20, subHeadY, contentX + 150, subHeadY + subHeadH }; DrawTextW(hdc, L"Time", -1, &rcH1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcH2 = { contentX + 160, subHeadY, contentX + 450, subHeadY + subHeadH }; DrawTextW(hdc, L"Action", -1, &rcH2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcH3 = { contentX + 460, subHeadY, contentX + 700, subHeadY + subHeadH }; DrawTextW(hdc, L"Performed by", -1, &rcH3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcH4 = { contentX + 710, subHeadY, contentX + 820, subHeadY + subHeadH }; DrawTextW(hdc, L"Result", -1, &rcH4, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcNoRec = { contentX, botY + 95, contentX + botW, botY + 125 };
    DrawTextW(hdc, L"No activity to report for this period.", -1, &rcNoRec, DT_CENTER | DT_TOP | DT_SINGLELINE);

    // Footer
    RECT rcFootL = { contentX, 695, contentX + 300, 720 };
    DrawTextW(hdc, L"Phone Book Management System", -1, &rcFootL, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcFootR = { contentX + botW - 300, 695, contentX + botW, 720 };
    DrawTextW(hdc, L"Administrator workspace", -1, &rcFootR, DT_RIGHT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::drawAdminBackupView(HDC hdc, const RECT&)
{
    int contentX = 250;

    HGDIOBJ oldFont = SelectObject(hdc, hFontSmallBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcBrd = { contentX, 24, contentX + 500, 42 };
    DrawTextW(hdc, L"ADMINISTRATION  /  BACKUP DATA", -1, &rcBrd, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { contentX, 45, contentX + 500, 80 };
    DrawTextW(hdc, L"Backup data", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { contentX, 85, contentX + 600, 110 };
    DrawTextW(hdc, L"Protect your contacts, groups and account data.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Top Section: Create system backup (Left) & Backup status (Right)
    int topY = 125;
    int topH = 245;
    int leftW = 510;
    int rightW = 310;

    // Card 1: Create a system backup
    drawRoundRect(hdc, contentX, topY, leftW, topH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcCbT = { contentX + 24, topY + 20, contentX + leftW - 24, topY + 50 };
    DrawTextW(hdc, L"Create a system backup", -1, &rcCbT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcCbS = { contentX + 24, topY + 52, contentX + leftW - 24, topY + 75 };
    DrawTextW(hdc, L"Save a complete copy of your phone book data to a local archive.", -1, &rcCbS, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcScT = { contentX + 24, topY + 84, contentX + 200, topY + 104 };
    DrawTextW(hdc, L"Backup scope", -1, &rcScT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcScV = { contentX + 24, topY + 104, contentX + leftW - 24, topY + 128 };
    DrawTextW(hdc, L"All contacts, groups and user accounts", -1, &rcScV, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcLocT = { contentX + 24, topY + 135, contentX + 200, topY + 155 };
    DrawTextW(hdc, L"Save location", -1, &rcLocT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Note above button
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcNote = { contentX + 24, topY + 205, contentX + 320, topY + 230 };
    DrawTextW(hdc, L"Archive includes a creation date and integrity check.", -1, &rcNote, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Card 2: Backup status
    int rightX = contentX + leftW + 20;
    drawRoundRect(hdc, rightX, topY, rightW, topH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcBsT = { rightX + 24, topY + 20, rightX + rightW - 24, topY + 50 };
    DrawTextW(hdc, L"Backup status", -1, &rcBsT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    bool hasBackup = std::filesystem::exists(db.getStorageFile() + ".backup");
    if (!hasBackup)
    {
        drawPillBadge(hdc, rightX + 24, topY + 56, 130, 28, L"Not backed up yet", UITheme::BgWarning, UITheme::BorderWarning, UITheme::WarningText, hFontSmallBold);
    }
    else
    {
        drawPillBadge(hdc, rightX + 24, topY + 56, 100, 28, L"Up to date", UITheme::SuccessBg, UITheme::SuccessBg, UITheme::SuccessText, hFontSmallBold);
    }

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcLbT = { rightX + 24, topY + 110, rightX + 200, topY + 130 };
    DrawTextW(hdc, L"Last backup", -1, &rcLbT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcLbV = { rightX + 24, topY + 135, rightX + 250, topY + 160 };
    DrawTextW(hdc, hasBackup ? L"Available locally" : L"—", -1, &rcLbV, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcAvT = { rightX + 24, topY + 175, rightX + 200, topY + 195 };
    DrawTextW(hdc, L"Available backups", -1, &rcAvT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontMetricVal);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcAvV = { rightX + 24, topY + 195, rightX + 150, topY + 235 };
    DrawTextW(hdc, hasBackup ? L"1" : L"0", -1, &rcAvV, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Middle: Backup History
    int histY = topY + topH + 20;
    int histW = 840;
    int histH = 175;
    drawRoundRect(hdc, contentX, histY, histW, histH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcHistT = { contentX + 24, histY + 16, contentX + 300, histY + 45 };
    DrawTextW(hdc, L"Backup history", -1, &rcHistT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Header bar
    int histHeadY = histY + 50;
    int histHeadH = 34;
    drawRoundRect(hdc, contentX, histHeadY, histW, histHeadH, 4, UITheme::BgTableHeader, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextBody);
    RECT rcHh1 = { contentX + 24, histHeadY, contentX + 300, histHeadY + histHeadH }; DrawTextW(hdc, L"Archive name", -1, &rcHh1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcHh2 = { contentX + 310, histHeadY, contentX + 500, histHeadY + histHeadH }; DrawTextW(hdc, L"Created", -1, &rcHh2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcHh3 = { contentX + 510, histHeadY, contentX + 640, histHeadY + histHeadH }; DrawTextW(hdc, L"Size", -1, &rcHh3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcHh4 = { contentX + 650, histHeadY, contentX + 750, histHeadY + histHeadH }; DrawTextW(hdc, L"Status", -1, &rcHh4, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcHh5 = { contentX + 760, histHeadY, contentX + 830, histHeadY + histHeadH }; DrawTextW(hdc, L"Actions", -1, &rcHh5, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (!hasBackup)
    {
        SelectObject(hdc, hFontBodyBold);
        SetTextColor(hdc, UITheme::TextPrimary);
        RECT rcNobT = { contentX, histY + 95, contentX + histW, histY + 120 };
        DrawTextW(hdc, L"No backups yet.", -1, &rcNobT, DT_CENTER | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hFontSubtitle);
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcNobS = { contentX, histY + 120, contentX + histW, histY + 145 };
        DrawTextW(hdc, L"Create your first backup to protect your data.", -1, &rcNobS, DT_CENTER | DT_TOP | DT_SINGLELINE);
    }
    else
    {
        SelectObject(hdc, hFontBody);
        SetTextColor(hdc, UITheme::TextPrimary);
        RECT rcRow1 = { contentX + 24, histY + 95, contentX + 300, histY + 120 }; DrawTextW(hdc, L"phonebook_accounts.db.backup", -1, &rcRow1, DT_LEFT | DT_TOP | DT_SINGLELINE);
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcRow2 = { contentX + 310, histY + 95, contentX + 500, histY + 120 }; DrawTextW(hdc, L"Local storage", -1, &rcRow2, DT_LEFT | DT_TOP | DT_SINGLELINE);
        RECT rcRow3 = { contentX + 510, histY + 95, contentX + 640, histY + 120 }; DrawTextW(hdc, L"~15 KB", -1, &rcRow3, DT_LEFT | DT_TOP | DT_SINGLELINE);
        SetTextColor(hdc, UITheme::SuccessText);
        RECT rcRow4 = { contentX + 650, histY + 95, contentX + 750, histY + 120 }; DrawTextW(hdc, L"Available", -1, &rcRow4, DT_LEFT | DT_TOP | DT_SINGLELINE);
    }

    // Bottom Alert Banner
    int alertY = histY + histH + 16;
    int alertH = 55;
    drawRoundRect(hdc, contentX, alertY, histW, alertH, 8, UITheme::BgLightBlue, UITheme::BorderBlue, 1);

    SelectObject(hdc, hFontLabel);
    SetTextColor(hdc, UITheme::PrimaryBlue);
    RECT rcAlT = { contentX + 20, alertY + 8, contentX + histW - 20, alertY + 28 };
    DrawTextW(hdc, L"Keep a copy somewhere safe.", -1, &rcAlT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcAlS = { contentX + 20, alertY + 28, contentX + histW - 20, alertY + 48 };
    DrawTextW(hdc, L"Store backups in a protected folder or external drive. Keep the archive private.", -1, &rcAlS, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::drawAdminRecoveryView(HDC hdc, const RECT&)
{
    int contentX = 250;

    HGDIOBJ oldFont = SelectObject(hdc, hFontSmallBold);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcBrd = { contentX, 24, contentX + 500, 42 };
    DrawTextW(hdc, L"ADMINISTRATION  /  DATA RECOVERY", -1, &rcBrd, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontTitleBig);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcTitle = { contentX, 45, contentX + 500, 80 };
    DrawTextW(hdc, L"Data recovery", -1, &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcSub = { contentX, 85, contentX + 600, 110 };
    DrawTextW(hdc, L"Restore the system from a verified backup.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Amber Warning Banner
    int banY = 120;
    int banW = 840;
    int banH = 65;
    drawRoundRect(hdc, contentX, banY, banW, banH, 8, UITheme::BgWarning, UITheme::BorderWarning, 1);

    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::WarningText);
    RECT rcWarnT = { contentX + 20, banY + 12, contentX + banW - 20, banY + 34 };
    DrawTextW(hdc, L"Recovery replaces current system data", -1, &rcWarnT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::WarningText);
    RECT rcWarnS = { contentX + 20, banY + 36, contentX + banW - 20, banY + 56 };
    DrawTextW(hdc, L"Create a backup before continuing. Restoring an archive cannot be undone.", -1, &rcWarnS, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Card 1: 1. Select a backup
    int c1Y = banY + banH + 16;
    int c1H = 205;
    int leftW = 510;
    int rightW = 310;

    drawRoundRect(hdc, contentX, c1Y, leftW, c1H, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcC1T = { contentX + 24, c1Y + 16, contentX + leftW - 24, c1Y + 45 };
    DrawTextW(hdc, L"1. Select a backup", -1, &rcC1T, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcC1S = { contentX + 24, c1Y + 45, contentX + leftW - 24, c1Y + 70 };
    DrawTextW(hdc, L"Choose an archive created by Phone Book Management System.", -1, &rcC1S, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcArcT = { contentX + 24, c1Y + 75, contentX + 200, c1Y + 95 };
    DrawTextW(hdc, L"Backup archive", -1, &rcArcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    RECT rcArcH = { contentX + 24, c1Y + 145, contentX + leftW - 24, c1Y + 168 };
    DrawTextW(hdc, L"Only valid system backup archives can be restored.", -1, &rcArcH, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Archive Details Card
    int rightX = contentX + leftW + 20;
    drawRoundRect(hdc, rightX, c1Y, rightW, c1H, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcAdT = { rightX + 24, c1Y + 16, rightX + rightW - 24, c1Y + 45 };
    DrawTextW(hdc, L"Archive details", -1, &rcAdT, DT_LEFT | DT_TOP | DT_SINGLELINE);

    int rowY = c1Y + 55;
    auto drawDetailRow = [&](const wchar_t* lbl, const wchar_t* val, COLORREF valCol = UITheme::TextPrimary) {
        SelectObject(hdc, hFontSmall);
        SetTextColor(hdc, UITheme::TextMuted);
        RECT rcL = { rightX + 24, rowY, rightX + 130, rowY + 22 };
        DrawTextW(hdc, lbl, -1, &rcL, DT_LEFT | DT_TOP | DT_SINGLELINE);

        SelectObject(hdc, hFontBody);
        SetTextColor(hdc, valCol);
        RECT rcV = { rightX + 135, rowY, rightX + rightW - 24, rowY + 22 };
        DrawTextW(hdc, val, -1, &rcV, DT_LEFT | DT_TOP | DT_SINGLELINE);
        rowY += 32;
    };

    drawDetailRow(L"File name", restoreValidated ? L"Verified Backup" : L"—");
    drawDetailRow(L"Created on", restoreValidated ? L"Today" : L"—");
    drawDetailRow(L"Archive size", restoreValidated ? L"15 KB" : L"—");
    drawDetailRow(L"Validation", restoreValidated ? L"Validated" : L"Not checked", restoreValidated ? UITheme::SuccessText : UITheme::TextPrimary);

    // Card 2: 2. Review and restore
    int c2Y = c1Y + c1H + 16;
    int c2W = 840;
    int c2H = 175;
    drawRoundRect(hdc, contentX, c2Y, c2W, c2H, 8, UITheme::BgCard, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontDialogTitle);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcC2T = { contentX + 24, c2Y + 16, contentX + c2W - 24, c2Y + 45 };
    DrawTextW(hdc, L"2. Review and restore", -1, &rcC2T, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(hdc, hFontSubtitle);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcC2S = { contentX + 24, c2Y + 45, contentX + c2W - 24, c2Y + 70 };
    DrawTextW(hdc, L"Select and validate a backup to preview the records that will be restored.", -1, &rcC2S, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Preview Boxes
    int boxW = 250;
    int boxH = 40;
    int boxY = c2Y + 75;
    drawRoundRect(hdc, contentX + 24, boxY, boxW, boxH, 6, UITheme::BgTableHeader, UITheme::BorderLight, 1);
    drawRoundRect(hdc, contentX + 24 + boxW + 20, boxY, boxW, boxH, 6, UITheme::BgTableHeader, UITheme::BorderLight, 1);
    drawRoundRect(hdc, contentX + 24 + (boxW + 20) * 2, boxY, boxW, boxH, 6, UITheme::BgTableHeader, UITheme::BorderLight, 1);

    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextPrimary);
    std::wstring b1 = L"Contacts: " + (restoreValidated ? toWString(std::to_string(restoreContactsCount)) : L"—");
    std::wstring b2 = L"Groups: " + (restoreValidated ? toWString(std::to_string(restoreGroupsCount)) : L"—");
    std::wstring b3 = L"User accounts: " + (restoreValidated ? toWString(std::to_string(restoreUsersCount)) : L"—");

    RECT rcB1 = { contentX + 36, boxY, contentX + 24 + boxW, boxY + boxH }; DrawTextW(hdc, b1.c_str(), -1, &rcB1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcB2 = { contentX + 36 + boxW + 20, boxY, contentX + 24 + (boxW * 2) + 20, boxY + boxH }; DrawTextW(hdc, b2.c_str(), -1, &rcB2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT rcB3 = { contentX + 36 + (boxW + 20) * 2, boxY, contentX + 24 + (boxW * 3) + 40, boxY + boxH }; DrawTextW(hdc, b3.c_str(), -1, &rcB3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Checkbox text
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextBody);
    RECT rcChk = { contentX + 24, c2Y + 130, contentX + c2W - 24, c2Y + 155 };
    DrawTextW(hdc, L"☐  I understand that recovery will replace current contacts, groups and user accounts.", -1, &rcChk, DT_LEFT | DT_TOP | DT_SINGLELINE);

    // Status bar at bottom
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcStat = { contentX + 20, 650, contentX + 500, 685 };
    DrawTextW(hdc, restoreValidated ? L"Backup archive validated successfully." : L"No backup selected. Validate an archive before restoring.",
              -1, &rcStat, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::drawAdminWorkspace(HDC hdc, const RECT& rc)
{
    drawAdminSidebar(hdc, rc);

    if (currentScreen == "admin_users" || currentScreen == "admin")
    {
        drawAdminUsersView(hdc, rc);
    }
    else if (currentScreen == "admin_reports")
    {
        drawAdminReportsView(hdc, rc);
    }
    else if (currentScreen == "admin_backup")
    {
        drawAdminBackupView(hdc, rc);
    }
    else if (currentScreen == "admin_recovery")
    {
        drawAdminRecoveryView(hdc, rc);
    }
}

// =========================================================================
// OWNER-DRAW PROCEDURES (Buttons & Listbox items)
// =========================================================================

void MenuUI::drawCustomButton(DRAWITEMSTRUCT* pDIS)
{
    HDC hdc = pDIS->hDC;
    RECT rc = pDIS->rcItem;
    bool isDown = (pDIS->itemState & ODS_SELECTED) != 0;
    bool isHot = false;

    auto it = buttonConfigs.find(pDIS->hwndItem);
    ButtonStyle style = ButtonStyle::Outline;
    if (it != buttonConfigs.end())
    {
        style = it->second.style;
        isHot = it->second.isHot;
    }

    // Determine colors
    COLORREF bgCol = UITheme::BgCard;
    COLORREF borderCol = UITheme::BorderInput;
    COLORREF textCol = UITheme::TextBody;
    int radius = 8;
    int borderW = 1;

    if (style == ButtonStyle::Primary)
    {
        bgCol = isDown ? UITheme::PrimaryHover : (isHot ? UITheme::PrimaryHover : UITheme::PrimaryBlue);
        borderCol = bgCol;
        textCol = UITheme::TextWhite;
        borderW = 0;
    }
    else if (style == ButtonStyle::Danger)
    {
        bgCol = isDown ? UITheme::DangerHover : (isHot ? UITheme::DangerHover : UITheme::DangerRed);
        borderCol = bgCol;
        textCol = UITheme::TextWhite;
        borderW = 0;
    }
    else if (style == ButtonStyle::NavItemActive)
    {
        bgCol = UITheme::BgLightBlue;
        borderCol = UITheme::BgLightBlue;
        textCol = UITheme::PrimaryBlue;
        borderW = 0;
    }
    else if (style == ButtonStyle::NavItem)
    {
        bgCol = isHot ? UITheme::BgHover : UITheme::BgWindow;
        borderCol = bgCol;
        textCol = UITheme::TextBody;
        borderW = 0;
    }
    else if (style == ButtonStyle::Link)
    {
        bgCol = UITheme::BgWindow;
        borderCol = bgCol;
        textCol = isHot ? UITheme::PrimaryHover : UITheme::PrimaryBlue;
        borderW = 0;
    }
    else // Outline
    {
        bgCol = isDown ? UITheme::BgTableHeader : (isHot ? UITheme::BgHover : UITheme::BgCard);
        borderCol = isHot ? UITheme::PrimaryBlue : UITheme::BorderInput;
        textCol = UITheme::TextBody;
    }

    // Draw Button Background
    drawRoundRect(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, radius, bgCol, borderCol, borderW);

    // If active NavItem, draw blue indicator bar on left edge
    if (style == ButtonStyle::NavItemActive)
    {
        drawRoundRect(hdc, rc.left, rc.top + 6, 3, rc.bottom - rc.top - 12, 2, UITheme::PrimaryBlue, UITheme::PrimaryBlue, 0);
    }

    // Fetch button text
    wchar_t btnText[256] = {};
    GetWindowTextW(pDIS->hwndItem, btnText, 256);

    HGDIOBJ oldFont = SelectObject(hdc, (style == ButtonStyle::Primary || style == ButtonStyle::Danger || style == ButtonStyle::NavItemActive) ? hFontBodyBold : hFontBody);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);

    UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    if (style == ButtonStyle::NavItem || style == ButtonStyle::NavItemActive)
    {
        format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
        rc.left += 16;
    }

    DrawTextW(hdc, btnText, -1, &rc, format);
    SelectObject(hdc, oldFont);
}

void MenuUI::drawContactListItem(DRAWITEMSTRUCT* pDIS)
{
    if (pDIS->itemID == (UINT)-1) return;

    auto filtered = getFilteredContacts();
    if (pDIS->itemID >= filtered.size()) return;

    const Contact& c = filtered[pDIS->itemID];
    HDC hdc = pDIS->hDC;
    RECT rc = pDIS->rcItem;

    bool isSelected = (pDIS->itemState & ODS_SELECTED) != 0;

    // Background
    COLORREF bgCol = isSelected ? UITheme::BgSelected : UITheme::BgCard;
    HBRUSH bgBrush = CreateSolidBrush(bgCol);
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    // Bottom 1px Divider
    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
    LineTo(hdc, rc.right, rc.bottom - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(penDivider);

    // Checkbox Column
    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isSelected ? UITheme::PrimaryBlue : UITheme::TextLight);
    RECT rcChk = { rc.left + 16, rc.top, rc.left + 46, rc.bottom };
    DrawTextW(hdc, isSelected ? L"☑" : L"☐", -1, &rcChk, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Avatar Column
    drawAvatarCircle(hdc, rc.left + 50, rc.top + 10, 32, c.name, hFontSmallBold);

    // Name Column
    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcName = { rc.left + 130, rc.top, rc.left + 320, rc.bottom };
    DrawTextW(hdc, toWString(c.name).c_str(), -1, &rcName, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Phone Column
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextBody);
    RECT rcPhone = { rc.left + 330, rc.top, rc.left + 500, rc.bottom };
    DrawTextW(hdc, toWString(c.phone).c_str(), -1, &rcPhone, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Email Column
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcEmail = { rc.left + 510, rc.top, rc.left + 740, rc.bottom };
    DrawTextW(hdc, toWString(c.email).c_str(), -1, &rcEmail, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Group Column (Pill Badge)
    std::string grpName = "";
    for (const auto& g : GroupService::getGroups())
    {
        if (g.id == c.group_id) { grpName = g.name; break; }
    }
    if (!grpName.empty())
    {
        drawPillBadge(hdc, rc.left + 750, rc.top + 13, 100, 26, toWString(grpName),
                      UITheme::BgLightBlue, UITheme::BorderBlue, UITheme::PrimaryBlue, hFontSmallBold);
    }
    else
    {
        SetTextColor(hdc, UITheme::TextLight);
        RECT rcGr = { rc.left + 750, rc.top, rc.left + 850, rc.bottom };
        DrawTextW(hdc, L"—", -1, &rcGr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Favorite Column
    SelectObject(hdc, hFontIcons);
    SetTextColor(hdc, c.isFavorite ? UITheme::StarGold : UITheme::TextLight);
    RECT rcFav = { rc.left + 935, rc.top, rc.left + 1000, rc.bottom };
    DrawTextW(hdc, c.isFavorite ? L"★" : L"☆", -1, &rcFav, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::drawAdminUserListItem(DRAWITEMSTRUCT* pDIS)
{
    if (pDIS->itemID == (UINT)-1) return;

    auto filtered = getFilteredAccounts();
    if (pDIS->itemID >= filtered.size()) return;

    const AccountData& a = filtered[pDIS->itemID];
    HDC hdc = pDIS->hDC;
    RECT rc = pDIS->rcItem;

    bool isSelected = (pDIS->itemState & ODS_SELECTED) != 0;

    COLORREF bgCol = isSelected ? UITheme::BgSelected : UITheme::BgCard;
    HBRUSH bgBrush = CreateSolidBrush(bgCol);
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
    HGDIOBJ oldPen = SelectObject(hdc, penDivider);
    MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
    LineTo(hdc, rc.right, rc.bottom - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(penDivider);

    HGDIOBJ oldFont = SelectObject(hdc, hFontIcons);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isSelected ? UITheme::PrimaryBlue : UITheme::TextLight);
    RECT rcChk = { rc.left + 16, rc.top, rc.left + 46, rc.bottom };
    DrawTextW(hdc, isSelected ? L"☑" : L"☐", -1, &rcChk, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Username
    SelectObject(hdc, hFontBodyBold);
    SetTextColor(hdc, UITheme::TextPrimary);
    RECT rcUn = { rc.left + 50, rc.top, rc.left + 180, rc.bottom };
    DrawTextW(hdc, toWString(a.username).c_str(), -1, &rcUn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Full name
    SelectObject(hdc, hFontBody);
    SetTextColor(hdc, UITheme::TextBody);
    RECT rcFn = { rc.left + 190, rc.top, rc.left + 340, rc.bottom };
    DrawTextW(hdc, toWString(a.fullname).c_str(), -1, &rcFn, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Email
    SetTextColor(hdc, UITheme::TextMuted);
    RECT rcEm = { rc.left + 350, rc.top, rc.left + 540, rc.bottom };
    DrawTextW(hdc, toWString(a.email).c_str(), -1, &rcEm, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Role (Pill badge)
    bool isAdmin = (a.role == "Admin");
    drawPillBadge(hdc, rc.left + 550, rc.top + 11, 75, 26, toWString(a.role),
                  isAdmin ? UITheme::BgLightBlue : UITheme::BgTableHeader,
                  isAdmin ? UITheme::BorderBlue : UITheme::BorderLight,
                  isAdmin ? UITheme::PrimaryBlue : UITheme::TextBody, hFontSmallBold);

    // Status (Active green pill)
    drawPillBadge(hdc, rc.left + 670, rc.top + 11, 65, 26, L"Active",
                  UITheme::SuccessBg, UITheme::SuccessBg, UITheme::SuccessText, hFontSmallBold);

    // Actions
    SelectObject(hdc, hFontSmall);
    SetTextColor(hdc, UITheme::PrimaryBlue);
    RECT rcAc = { rc.left + 770, rc.top, rc.left + 830, rc.bottom };
    DrawTextW(hdc, L"Manage", -1, &rcAc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
}

void MenuUI::onDrawItem(DRAWITEMSTRUCT* pDIS)
{
    if (pDIS->CtlType == ODT_BUTTON)
    {
        drawCustomButton(pDIS);
    }
    else if (pDIS->CtlType == ODT_LISTBOX)
    {
        if (pDIS->CtlID == 1312)
        {
            drawContactListItem(pDIS);
        }
        else if (pDIS->CtlID == 2106)
        {
            drawAdminUserListItem(pDIS);
        }
    }
}

// =========================================================================
// PAINT DISPATCHER
// =========================================================================

void MenuUI::onPaint(HDC hdc, const RECT& rc)
{
    // Double-buffered rendering to eliminate any flicker
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    HGDIOBJ oldBM = SelectObject(memDC, memBM);

    // Clear background
    FillRect(memDC, &rc, hBrushBg);

    if (currentScreen == "login")
    {
        drawLogin(memDC, rc);
    }
    else if (currentScreen == "register")
    {
        drawRegister(memDC, rc);
    }
    else if (currentScreen == "forgot")
    {
        drawForgotPassword(memDC, rc);
    }
    else if (currentScreen == "contacts")
    {
        drawContactsDashboard(memDC, rc);
    }
    else if (currentScreen.rfind("admin", 0) == 0)
    {
        drawAdminWorkspace(memDC, rc);
    }

    BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

// =========================================================================
// MODAL DIALOGS (Matches Dialog & Modal mockups)
// =========================================================================

// Shared Dialog Procedure Helper for Modals
struct DialogData
{
    MenuUI* pUI = nullptr;
    std::string type; // "add_contact", "edit_contact", "contact_detail", "manage_groups", "delete_confirm", "add_user", "confirm_restore"
    int contactId = 0;
    int groupId = 0;
    HWND hDlg = nullptr;
};

LRESULT CALLBACK ModalDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DialogData* data = reinterpret_cast<DialogData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        data = reinterpret_cast<DialogData*>(cs->lpCreateParams);
        data->hDlg = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        return 0;
    }

    if (!data) return DefWindowProc(hwnd, uMsg, wParam, lParam);

    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        // Fill background
        HBRUSH bgBrush = CreateSolidBrush(UITheme::BgWindow);
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        SetBkMode(hdc, TRANSPARENT);

        if (data->type == "add_contact" || data->type == "edit_contact")
        {
            // Title
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcT = { 32, 28, 400, 60 };
            DrawTextW(hdc, (data->type == "add_contact") ? L"Add Contact" : L"Edit Contact", -1, &rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Avatar placeholder circle (diameter 60)
            drawCircle(hdc, 62, 102, 30, UITheme::BgLightBlue, UITheme::BorderBlue, 1);
            SelectObject(hdc, data->pUI->hFontIcons);
            SetTextColor(hdc, UITheme::PrimaryBlue);
            RECT rcAvI = { 32, 72, 92, 132 };
            DrawTextW(hdc, L"👤", -1, &rcAvI, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Subtext for upload
            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcUpS = { 104, 114, 300, 135 };
            DrawTextW(hdc, L"JPG or PNG • Optional", -1, &rcUpS, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Field Labels
            SelectObject(hdc, data->pUI->hFontLabel);
            SetTextColor(hdc, UITheme::TextPrimary);

            RECT rcL1 = { 32, 150, 280, 172 }; DrawTextW(hdc, L"Name*", -1, &rcL1, DT_LEFT | DT_TOP | DT_SINGLELINE);
            RECT rcL2 = { 310, 150, 560, 172 }; DrawTextW(hdc, L"Phone*", -1, &rcL2, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT rcL3 = { 32, 230, 280, 252 }; DrawTextW(hdc, L"Email", -1, &rcL3, DT_LEFT | DT_TOP | DT_SINGLELINE);
            RECT rcL4 = { 310, 230, 560, 252 }; DrawTextW(hdc, L"Address", -1, &rcL4, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT rcL5 = { 32, 310, 280, 332 }; DrawTextW(hdc, L"Group", -1, &rcL5, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Helper under group
            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcGrH = { 32, 375, 280, 395 };
            DrawTextW(hdc, L"Family • Work • Friends", -1, &rcGrH, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Bottom Divider
            HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
            HGDIOBJ oldPen = SelectObject(hdc, penDivider);
            MoveToEx(hdc, 0, 440, nullptr);
            LineTo(hdc, rc.right, 440);
            SelectObject(hdc, oldPen);
            DeleteObject(penDivider);

            // Footnote
            RECT rcFoot = { 32, 460, 250, 485 };
            DrawTextW(hdc, L"* Phone is required", -1, &rcFoot, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else if (data->type == "contact_detail")
        {
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcT = { 32, 28, 400, 60 };
            DrawTextW(hdc, L"Contact Details", -1, &rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Find contact
            Contact target;
            for (const auto& c : ContactService::getContacts())
            {
                if (c.id == data->contactId) { target = c; break; }
            }

            // Big Avatar Circle
            drawCircle(hdc, 72, 120, 40, UITheme::BgLightBlue, UITheme::BorderBlue, 1);
            SelectObject(hdc, data->pUI->hFontIconsBig);
            SetTextColor(hdc, UITheme::PrimaryBlue);
            RECT rcAvI = { 32, 80, 112, 160 };
            DrawTextW(hdc, L"👤", -1, &rcAvI, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Name
            SelectObject(hdc, data->pUI->hFontTitleBig);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcNm = { 130, 85, 450, 118 };
            DrawTextW(hdc, toWString(target.name).c_str(), -1, &rcNm, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Group badge & Star
            std::string grpName = "None";
            for (const auto& g : GroupService::getGroups())
            {
                if (g.id == target.group_id) { grpName = g.name; break; }
            }
            std::wstring bText = L"Group: " + toWString(grpName);
            drawPillBadge(hdc, 130, 125, 100, 26, bText,
                          UITheme::BgTableHeader, UITheme::BorderLight, UITheme::TextBody, data->pUI->hFontSmallBold);

            if (target.isFavorite)
            {
                SelectObject(hdc, data->pUI->hFontIcons);
                SetTextColor(hdc, UITheme::StarGold);
                RECT rcSt = { 240, 125, 270, 151 };
                DrawTextW(hdc, L"★", -1, &rcSt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            // Divider
            HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
            HGDIOBJ oldPen = SelectObject(hdc, penDivider);
            MoveToEx(hdc, 32, 185, nullptr);
            LineTo(hdc, rc.right - 32, 185);

            // Info rows: Phone, Email, Address
            int rowY = 210;
            auto drawDetailRow = [&](const wchar_t* icon, const wchar_t* lbl, const std::string& val) {
                SelectObject(hdc, data->pUI->hFontIcons);
                SetTextColor(hdc, UITheme::TextMuted);
                RECT rcIc = { 32, rowY, 60, rowY + 28 };
                DrawTextW(hdc, icon, -1, &rcIc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, data->pUI->hFontLabel);
                SetTextColor(hdc, UITheme::TextMuted);
                RECT rcL = { 65, rowY, 150, rowY + 28 };
                DrawTextW(hdc, lbl, -1, &rcL, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, data->pUI->hFontBody);
                SetTextColor(hdc, UITheme::TextPrimary);
                RECT rcV = { 160, rowY, 520, rowY + 28 };
                DrawTextW(hdc, val.empty() ? L"—" : toWString(val).c_str(), -1, &rcV, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                rowY += 44;
            };

            drawDetailRow(L"📞", L"Phone", target.phone);
            drawDetailRow(L"✉", L"Email", target.email);
            drawDetailRow(L"📍", L"Address", target.address);

            // Bottom Divider
            MoveToEx(hdc, 0, 395, nullptr);
            LineTo(hdc, rc.right, 395);

            SelectObject(hdc, oldPen);
            DeleteObject(penDivider);
        }
        else if (data->type == "manage_groups")
        {
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcT = { 32, 28, 400, 60 };
            DrawTextW(hdc, L"Manage Groups", -1, &rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Header for groups list
            SelectObject(hdc, data->pUI->hFontLabel);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcLg = { 32, 130, 200, 155 };
            DrawTextW(hdc, L"List of groups", -1, &rcLg, DT_LEFT | DT_TOP | DT_SINGLELINE);

            const auto& groups = GroupService::getGroups();
            std::wstring cntStr = std::to_wstring(groups.size()) + L" groups";
            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcCnt = { 460, 130, 560, 155 };
            DrawTextW(hdc, cntStr.c_str(), -1, &rcCnt, DT_RIGHT | DT_TOP | DT_SINGLELINE);

            // Container for groups
            int listY = 160;
            int listW = 540;
            int listH = 240;
            drawRoundRect(hdc, 32, listY, listW, listH, 8, UITheme::BgCard, UITheme::BorderLight, 1);

            // Render each group row
            int rowY = listY;
            int itemH = 68;
            for (size_t i = 0; i < groups.size() && i < 3; ++i)
            {
                if (i > 0)
                {
                    HPEN pen = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
                    HGDIOBJ oldPen = SelectObject(hdc, pen);
                    MoveToEx(hdc, 32, rowY, nullptr);
                    LineTo(hdc, 32 + listW, rowY);
                    SelectObject(hdc, oldPen);
                    DeleteObject(pen);
                }

                // Group icon
                const wchar_t* gIcon = (i == 0) ? L"🏠" : (i == 1 ? L"💼" : L"👥");
                drawRoundRect(hdc, 48, rowY + 14, 40, 40, 8, UITheme::BgLightBlue, UITheme::BorderBlue, 1);
                SelectObject(hdc, data->pUI->hFontIcons);
                SetTextColor(hdc, UITheme::PrimaryBlue);
                RECT rcIc = { 48, rowY + 14, 88, rowY + 54 };
                DrawTextW(hdc, gIcon, -1, &rcIc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Group Name
                SelectObject(hdc, data->pUI->hFontBodyBold);
                SetTextColor(hdc, UITheme::TextPrimary);
                RECT rcGn = { 102, rowY + 14, 400, rowY + 36 };
                DrawTextW(hdc, toWString(groups[i].name).c_str(), -1, &rcGn, DT_LEFT | DT_TOP | DT_SINGLELINE);

                // Contact count
                auto gContacts = GroupService::getContactsForGroup(groups[i].id);
                std::wstring cStr = std::to_wstring(gContacts.size()) + L" contacts";
                SelectObject(hdc, data->pUI->hFontSmall);
                SetTextColor(hdc, UITheme::TextMuted);
                RECT rcGc = { 102, rowY + 38, 400, rowY + 58 };
                DrawTextW(hdc, cStr.c_str(), -1, &rcGc, DT_LEFT | DT_TOP | DT_SINGLELINE);

                rowY += itemH;
            }

            // Bottom Divider
            HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
            HGDIOBJ oldPen = SelectObject(hdc, penDivider);
            MoveToEx(hdc, 0, 420, nullptr);
            LineTo(hdc, rc.right, 420);
            SelectObject(hdc, oldPen);
            DeleteObject(penDivider);

            // Footnote
            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcFoot = { 32, 440, 350, 465 };
            DrawTextW(hdc, L"Use the pencil to rename a group.", -1, &rcFoot, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else if (data->type == "delete_confirm")
        {
            // Warning circle
            drawCircle(hdc, 60, 60, 28, UITheme::BgDanger, UITheme::BorderDanger, 1);
            SelectObject(hdc, data->pUI->hFontIcons);
            SetTextColor(hdc, UITheme::DangerRed);
            RECT rcIc = { 32, 32, 88, 88 };
            DrawTextW(hdc, L"⚠", -1, &rcIc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Question
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcQ = { 105, 36, 460, 75 };
            DrawTextW(hdc, L"Are you sure you want to delete this contact?", -1, &rcQ, DT_LEFT | DT_TOP | DT_WORDBREAK);

            SelectObject(hdc, data->pUI->hFontSubtitle);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcSub = { 105, 85, 460, 110 };
            DrawTextW(hdc, L"This action cannot be undone.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Divider
            HPEN penDivider = CreatePen(PS_SOLID, 1, UITheme::BorderLight);
            HGDIOBJ oldPen = SelectObject(hdc, penDivider);
            MoveToEx(hdc, 0, 140, nullptr);
            LineTo(hdc, rc.right, 140);
            SelectObject(hdc, oldPen);
            DeleteObject(penDivider);
        }
        else if (data->type == "add_user")
        {
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcT = { 32, 28, 400, 58 };
            DrawTextW(hdc, L"Add user", -1, &rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(hdc, data->pUI->hFontSubtitle);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcSub = { 32, 60, 500, 85 };
            DrawTextW(hdc, L"Create an account and choose its system access.", -1, &rcSub, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Labels
            SelectObject(hdc, data->pUI->hFontLabel);
            SetTextColor(hdc, UITheme::TextPrimary);

            RECT rcL1 = { 32, 95, 280, 118 }; DrawTextW(hdc, L"Username *", -1, &rcL1, DT_LEFT | DT_TOP | DT_SINGLELINE);
            RECT rcL2 = { 310, 95, 560, 118 }; DrawTextW(hdc, L"Full name", -1, &rcL2, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT rcL3 = { 32, 175, 560, 198 }; DrawTextW(hdc, L"Email *", -1, &rcL3, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT rcL4 = { 32, 255, 560, 278 }; DrawTextW(hdc, L"Temporary password *", -1, &rcL4, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT rcL5 = { 32, 335, 280, 358 }; DrawTextW(hdc, L"Role", -1, &rcL5, DT_LEFT | DT_TOP | DT_SINGLELINE);
            RECT rcL6 = { 310, 335, 560, 358 }; DrawTextW(hdc, L"Account status", -1, &rcL6, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::TextMuted);
            RECT rcFoot = { 32, 415, 560, 440 };
            DrawTextW(hdc, L"The user will be asked to change their password at next sign-in.", -1, &rcFoot, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }
        else if (data->type == "confirm_restore")
        {
            // Red exclamation mark
            SelectObject(hdc, data->pUI->hFontTitleBig);
            SetTextColor(hdc, UITheme::DangerRed);
            RECT rcEx = { 32, 28, 60, 68 };
            DrawTextW(hdc, L"!", -1, &rcEx, DT_LEFT | DT_TOP | DT_SINGLELINE);

            // Title
            SelectObject(hdc, data->pUI->hFontDialogTitle);
            SetTextColor(hdc, UITheme::TextPrimary);
            RECT rcT = { 65, 30, 450, 65 };
            DrawTextW(hdc, L"Restore system data?", -1, &rcT, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(hdc, data->pUI->hFontSubtitle);
            SetTextColor(hdc, UITheme::TextBody);
            RECT rcSub = { 32, 75, 460, 120 };
            DrawTextW(hdc, L"The validated backup will replace all current contacts, groups and user accounts.", -1, &rcSub, DT_LEFT | DT_TOP | DT_WORDBREAK);

            // Red Warning Callout
            drawRoundRect(hdc, 32, 130, 436, 56, 6, UITheme::BgDanger, UITheme::BorderDanger, 1);
            SelectObject(hdc, data->pUI->hFontBodyBold);
            SetTextColor(hdc, UITheme::DangerRed);
            RECT rcAl1 = { 48, 138, 450, 158 };
            DrawTextW(hdc, L"This action cannot be undone.", -1, &rcAl1, DT_LEFT | DT_TOP | DT_SINGLELINE);

            SelectObject(hdc, data->pUI->hFontSmall);
            SetTextColor(hdc, UITheme::DangerRed);
            RECT rcAl2 = { 48, 158, 450, 178 };
            DrawTextW(hdc, L"Keep a current backup before you continue.", -1, &rcAl2, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    if (uMsg == WM_DRAWITEM)
    {
        DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
        if (pDIS->CtlType == ODT_BUTTON)
        {
            data->pUI->drawCustomButton(pDIS);
            return TRUE;
        }
    }

    if (uMsg == WM_COMMAND)
    {
        int id = LOWORD(wParam);

        // Cancel / Close buttons
        if (id == 9001 || id == IDCANCEL)
        {
            DestroyWindow(hwnd);
            return 0;
        }

        if (data->type == "add_contact" && id == 9002) // Save Contact
        {
            std::string name = getEditText(hwnd, 5001);
            std::string phone = getEditText(hwnd, 5002);
            std::string email = getEditText(hwnd, 5003);
            std::string address = getEditText(hwnd, 5004);
            bool isFav = (SendMessage(GetDlgItem(hwnd, 5006), BM_GETCHECK, 0, 0) == BST_CHECKED);

            if (name.empty() || phone.empty())
            {
                MessageBoxA(hwnd, "Name and Phone are required.", "Add Contact", MB_OK | MB_ICONWARNING);
                return 0;
            }

            int groupId = 0;
            LRESULT gIdx = SendMessage(GetDlgItem(hwnd, 5005), CB_GETCURSEL, 0, 0);
            const auto& groups = GroupService::getGroups();
            if (gIdx > 0 && (size_t)(gIdx - 1) < groups.size())
            {
                groupId = groups[gIdx - 1].id;
            }

            Contact c;
            c.account_id = data->pUI->accountService.getCurrentUserId();
            c.name = name;
            c.phone = phone;
            c.email = email;
            c.address = address;
            c.group_id = groupId;
            c.isFavorite = isFav;

            if (ContactService::addContact(c))
            {
                if (groupId != 0 && !ContactService::getContacts().empty())
                {
                    GroupService::assignContact(ContactService::getContacts().back().id, groupId);
                }
                DestroyWindow(hwnd);
            }
            else
            {
                MessageBoxA(hwnd, "Could not save contact. A contact with this phone may already exist.", "Add Contact", MB_OK | MB_ICONERROR);
            }
            return 0;
        }

        if (data->type == "edit_contact" && id == 9002) // Update Contact
        {
            std::string name = getEditText(hwnd, 5001);
            std::string phone = getEditText(hwnd, 5002);
            std::string email = getEditText(hwnd, 5003);
            std::string address = getEditText(hwnd, 5004);
            bool isFav = (SendMessage(GetDlgItem(hwnd, 5006), BM_GETCHECK, 0, 0) == BST_CHECKED);

            int groupId = 0;
            LRESULT gIdx = SendMessage(GetDlgItem(hwnd, 5005), CB_GETCURSEL, 0, 0);
            const auto& groups = GroupService::getGroups();
            if (gIdx > 0 && (size_t)(gIdx - 1) < groups.size())
            {
                groupId = groups[gIdx - 1].id;
            }

            for (const auto& old : ContactService::getContacts())
            {
                if (old.id == data->contactId)
                {
                    Contact c = old;
                    c.name = name;
                    c.phone = phone;
                    c.email = email;
                    c.address = address;
                    c.group_id = groupId;
                    c.isFavorite = isFav;

                    if (ContactService::updateContact(c) && GroupService::setContactGroup(old.id, groupId))
                    {
                        DestroyWindow(hwnd);
                    }
                    else
                    {
                        MessageBoxA(hwnd, "Could not update contact.", "Edit Contact", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
            }
            return 0;
        }

        if (data->type == "contact_detail" && id == 9003) // Edit from detail
        {
            int cid = data->contactId;
            DestroyWindow(hwnd);
            data->pUI->selectedContactId = cid;
            data->pUI->showEditContactDialog();
            return 0;
        }

        if (data->type == "manage_groups" && id == 9004) // Add group
        {
            std::string gName = getEditText(hwnd, 5101);
            if (!gName.empty())
            {
                Group g;
                g.name = gName;
                g.description = "Custom Group";
                GroupService::addGroup(g);
                SetDlgItemTextA(hwnd, 5101, "");
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }

        if (data->type == "delete_confirm" && id == 9005) // Confirm delete
        {
            ContactService::removeContact(data->contactId);
            data->pUI->selectedContactId = 0;
            DestroyWindow(hwnd);
            return 0;
        }

        if (data->type == "add_user" && id == 9006) // Create user
        {
            std::string un = getEditText(hwnd, 5201);
            std::string fn = getEditText(hwnd, 5202);
            std::string em = getEditText(hwnd, 5203);
            std::string pw = getEditText(hwnd, 5204);
            std::string role = (SendMessage(GetDlgItem(hwnd, 5205), CB_GETCURSEL, 0, 0) == 1) ? "Admin" : "User";

            if (un.empty() || pw.empty() || em.empty())
            {
                MessageBoxA(hwnd, "Username, Email, and Temporary Password are required.", "Add User", MB_OK | MB_ICONWARNING);
                return 0;
            }

            if (data->pUI->accountService.registerAccount(un, em, pw, fn, "0123456789", role))
            {
                DestroyWindow(hwnd);
            }
            else
            {
                MessageBoxA(hwnd, "Failed to create user account. Username may already exist.", "Add User", MB_OK | MB_ICONERROR);
            }
            return 0;
        }

        if (data->type == "confirm_restore" && id == 9007) // Restore confirm
        {
            std::error_code error;
            for (const std::string& suffix : {"", ".contacts", ".groups", ".memberships"})
            {
                const std::filesystem::path backup = db.getStorageFile() + suffix + ".backup";
                const std::filesystem::path source = db.getStorageFile() + suffix;
                if (std::filesystem::exists(backup))
                {
                    std::filesystem::copy_file(backup, source, std::filesystem::copy_options::overwrite_existing, error);
                }
            }
            DestroyWindow(hwnd);
            MessageBoxA(data->pUI->hWnd, "System data has been restored successfully.", "Data Recovery", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
    }

    if (uMsg == WM_DESTROY)
    {
        EnableWindow(data->pUI->hWnd, TRUE);
        SetForegroundWindow(data->pUI->hWnd);
        data->pUI->showScreen(data->pUI->currentScreen); // Refresh parent screen
        delete data;
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MenuUI::showAddContactDialog()
{
    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "add_contact";

    int dlgW = 620;
    int dlgH = 540;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    // Form inputs
    data->pUI->createStyledButton(L"Upload photo", 104, 76, 120, 32, 9010, ButtonStyle::Outline);

    data->pUI->createStyledEdit(L"Enter contact name", 32, 175, 260, 38, 5001);
    data->pUI->createStyledEdit(L"Enter phone number", 310, 175, 260, 38, 5002);

    data->pUI->createStyledEdit(L"Enter email address", 32, 255, 260, 38, 5003);
    data->pUI->createStyledEdit(L"Enter address", 310, 255, 260, 38, 5004);

    HWND combo = data->pUI->createStyledCombo(32, 335, 260, 200, 5005);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"No Group");
    for (const auto& g : GroupService::getGroups())
    {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)toWString(g.name).c_str());
    }
    SendMessageW(combo, CB_SETCURSEL, 0, 0);

    // Mark as favorite checkbox
    CreateWindowW(L"BUTTON", L"☆ Mark as Favorite", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                  310, 338, 200, 32, hDlg, (HMENU)5006, GetModuleHandle(nullptr), nullptr);

    // Bottom action buttons
    data->pUI->createStyledButton(L"Cancel", 390, 455, 85, 38, 9001, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Save", 485, 455, 85, 38, 9002, ButtonStyle::Primary);

    // Reparent controls into modal dialog
    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg)
        {
            SetParent(c, hDlg);
        }
    }
}

void MenuUI::showEditContactDialog()
{
    Contact target;
    for (const auto& c : ContactService::getContacts())
    {
        if (c.id == selectedContactId) { target = c; break; }
    }
    if (target.id == 0) return;

    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "edit_contact";
    data->contactId = target.id;

    int dlgW = 620;
    int dlgH = 540;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledButton(L"Upload photo", 104, 76, 120, 32, 9010, ButtonStyle::Outline);

    HWND e1 = data->pUI->createStyledEdit(L"Enter contact name", 32, 175, 260, 38, 5001);
    SetWindowTextW(e1, toWString(target.name).c_str());

    HWND e2 = data->pUI->createStyledEdit(L"Enter phone number", 310, 175, 260, 38, 5002);
    SetWindowTextW(e2, toWString(target.phone).c_str());

    HWND e3 = data->pUI->createStyledEdit(L"Enter email address", 32, 255, 260, 38, 5003);
    SetWindowTextW(e3, toWString(target.email).c_str());

    HWND e4 = data->pUI->createStyledEdit(L"Enter address", 310, 255, 260, 38, 5004);
    SetWindowTextW(e4, toWString(target.address).c_str());

    HWND combo = data->pUI->createStyledCombo(32, 335, 260, 200, 5005);
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"No Group");
    const auto& groups = GroupService::getGroups();
    int selIdx = 0;
    for (size_t i = 0; i < groups.size(); ++i)
    {
        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)toWString(groups[i].name).c_str());
        if (groups[i].id == target.group_id) selIdx = (int)i + 1;
    }
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)selIdx, 0);

    HWND chk = CreateWindowW(L"BUTTON", L"☆ Mark as Favorite", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                             310, 338, 200, 32, hDlg, (HMENU)5006, GetModuleHandle(nullptr), nullptr);
    if (target.isFavorite) SendMessage(chk, BM_SETCHECK, BST_CHECKED, 0);

    data->pUI->createStyledButton(L"Cancel", 350, 455, 85, 38, 9001, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Save Changes", 445, 455, 125, 38, 9002, ButtonStyle::Primary);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

void MenuUI::showContactDetailsModal()
{
    if (selectedContactId == 0) return;

    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "contact_detail";
    data->contactId = selectedContactId;

    int dlgW = 560;
    int dlgH = 480;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledButton(L"✏  Edit", 350, 412, 85, 38, 9003, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Close", 445, 412, 80, 38, 9001, ButtonStyle::Outline);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

void MenuUI::showManageGroupsDialog()
{
    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "manage_groups";

    int dlgW = 600;
    int dlgH = 510;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledEdit(L"+ Create New Group", 32, 75, 410, 38, 5101);
    data->pUI->createStyledButton(L"Add Group", 452, 75, 120, 38, 9004, ButtonStyle::Primary);

    data->pUI->createStyledButton(L"Close", 480, 435, 90, 38, 9001, ButtonStyle::Outline);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

void MenuUI::showDeleteContactConfirm()
{
    if (selectedContactId == 0) return;

    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "delete_confirm";
    data->contactId = selectedContactId;

    int dlgW = 500;
    int dlgH = 220;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledButton(L"Cancel", 270, 155, 90, 38, 9001, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Delete", 370, 155, 95, 38, 9005, ButtonStyle::Danger);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

void MenuUI::showAdminAddUserDialog()
{
    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "add_user";

    int dlgW = 620;
    int dlgH = 540;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledEdit(L"Enter username", 32, 120, 260, 38, 5201);
    data->pUI->createStyledEdit(L"Enter full name", 310, 120, 260, 38, 5202);

    data->pUI->createStyledEdit(L"Enter email address", 32, 200, 538, 38, 5203);

    data->pUI->createStyledEdit(L"Enter temporary password", 32, 280, 538, 38, 5204, true);

    HWND rCombo = data->pUI->createStyledCombo(32, 360, 260, 200, 5205);
    SendMessageW(rCombo, CB_ADDSTRING, 0, (LPARAM)L"User");
    SendMessageW(rCombo, CB_ADDSTRING, 0, (LPARAM)L"Admin");
    SendMessageW(rCombo, CB_SETCURSEL, 0, 0);

    HWND sCombo = data->pUI->createStyledCombo(310, 360, 260, 200, 5206);
    SendMessageW(sCombo, CB_ADDSTRING, 0, (LPARAM)L"Active");
    SendMessageW(sCombo, CB_ADDSTRING, 0, (LPARAM)L"Inactive");
    SendMessageW(sCombo, CB_SETCURSEL, 0, 0);

    data->pUI->createStyledButton(L"Cancel", 340, 455, 90, 38, 9001, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Create user", 440, 455, 130, 38, 9006, ButtonStyle::Primary);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

void MenuUI::showAdminConfirmRestore()
{
    DialogData* data = new DialogData();
    data->pUI = this;
    data->type = "confirm_restore";

    int dlgW = 500;
    int dlgH = 260;
    RECT rcParent;
    GetWindowRect(hWnd, &rcParent);
    int posX = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int posY = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, kDialogWndClass, L"Phone Book Management System",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH, hWnd, nullptr, GetModuleHandle(nullptr), data);

    EnableWindow(hWnd, FALSE);

    data->pUI->createStyledButton(L"Cancel", 230, 200, 95, 38, 9001, ButtonStyle::Outline);
    data->pUI->createStyledButton(L"Restore data", 335, 200, 135, 38, 9007, ButtonStyle::Danger);

    for (HWND c : screenControls)
    {
        if (GetParent(c) == hWnd && c != hDlg) SetParent(c, hDlg);
    }
}

// =========================================================================
// COMMAND HANDLERS
// =========================================================================

void MenuUI::handleLogin()
{
    std::string username = getEditText(hWnd, 1001);
    std::string password = getEditText(hWnd, 1002);

    if (username.empty() || password.empty())
    {
        MessageBoxA(hWnd, "Please enter your username and password.", "Login", MB_OK | MB_ICONWARNING);
        return;
    }

    if (accountService.login(username, password))
    {
        ContactService::loadForAccount(accountService.getCurrentUserId());
        GroupService::loadForAccount(accountService.getCurrentUserId());

        // Check if user has groups; if none exist, initialize the 3 default groups matching mockups
        if (GroupService::getGroups().empty())
        {
            Group g1(0, accountService.getCurrentUserId(), "Family", "Family members");
            Group g2(0, accountService.getCurrentUserId(), "Work", "Colleagues and work");
            Group g3(0, accountService.getCurrentUserId(), "Friends", "Friends and buddies");
            GroupService::addGroup(g1);
            GroupService::addGroup(g2);
            GroupService::addGroup(g3);
        }

        AccountData account;
        currentRole = db.getAccount(accountService.getCurrentUserId(), account) ? account.role : "User";
        showScreen("contacts");
        return;
    }

    MessageBoxA(hWnd, "Invalid username or password. Please try again.", "Login", MB_OK | MB_ICONERROR);
}

void MenuUI::handleLogout()
{
    ContactService::clear();
    GroupService::clear();
    accountService.logout();
    currentRole.clear();
    selectedContactId = 0;
    selectedGroupId = 0;
    selectedAccountId = 0;
    searchQuery = "";
    filterGroupId = 0;
    filterOnlyFavorites = false;
    showScreen("login");
}

void MenuUI::handleClose()
{
    DestroyWindow(hWnd);
}

void MenuUI::handleCommand(WPARAM wParam, LPARAM lParam)
{
    int id = LOWORD(wParam);
    // Screen 1: Login
    if (id == 100) handleLogin();
    else if (id == 102) showScreen("forgot");
    else if (id == 103) showScreen("register");
    else if (id == 104) handleClose();
    else if (id == 1003) // Toggle password visibility
    {
        passwordVisible = !passwordVisible;
        HWND pEdit = GetDlgItem(hWnd, 1002);
        SendMessage(pEdit, EM_SETPASSWORDCHAR, passwordVisible ? 0 : (WPARAM)'*', 0);
        InvalidateRect(pEdit, nullptr, TRUE);
        HWND btn = GetDlgItem(hWnd, 1003);
        SetWindowTextW(btn, passwordVisible ? L"👁" : L"◎");
    }

    // Screen 2: Register
    else if (id == 1106) showScreen("login");
    else if (id == 1107) showScreen("login");
    else if (id == 1108) // Register submit
    {
        std::string un = getEditText(hWnd, 1101);
        std::string pw = getEditText(hWnd, 1102);
        std::string fn = getEditText(hWnd, 1103);
        std::string em = getEditText(hWnd, 1104);
        std::string ph = getEditText(hWnd, 1105);

        if (un.length() < 3 || pw.length() < 6 || em.empty() || fn.empty())
        {
            MessageBoxA(hWnd, "Please complete all fields with a valid username (min 3 chars) and password (min 6 chars).",
                        "Register", MB_OK | MB_ICONWARNING);
            return;
        }

        if (accountService.registerAccount(un, em, pw, fn, ph, "User"))
        {
            MessageBoxA(hWnd, "Registration successful! You can now log in.", "Register", MB_OK | MB_ICONINFORMATION);
            showScreen("login");
        }
        else
        {
            MessageBoxA(hWnd, "Registration failed. Username may already be in use.", "Register", MB_OK | MB_ICONERROR);
        }
    }

    // Screen 3: Forgot Password
    else if (id == 1205) showScreen("login");
    else if (id == 1206) // Reset password submit
    {
        std::string un = getEditText(hWnd, 1201);
        std::string em = getEditText(hWnd, 1202);
        std::string p1 = getEditText(hWnd, 1203);
        std::string p2 = getEditText(hWnd, 1204);

        if (p1.empty() || p1 != p2)
        {
            MessageBoxA(hWnd, "Passwords do not match.", "Reset Password", MB_OK | MB_ICONWARNING);
            return;
        }

        bool reset = false;
        for (const auto& a : db.getAccounts())
        {
            if (a.username == un && a.email == em)
            {
                reset = db.executeNonQuery("UPDATE Accounts SET password = SHA2('" +
                    db.escapeString(p1) + "', 256) WHERE account_id = " + std::to_string(a.id));
                break;
            }
        }
        if (reset)
        {
            MessageBoxA(hWnd, "Password has been reset successfully.", "Reset Password", MB_OK | MB_ICONINFORMATION);
            showScreen("login");
        }
        else
        {
            MessageBoxA(hWnd, "Username and Email do not match our records.", "Reset Password", MB_OK | MB_ICONERROR);
        }
    }

    // Screen 4: Contacts Dashboard
    else if (id == 1302) // Search Contacts
    {
        searchQuery = toNarrowString(getEditTextW(hWnd, 1301));
        showScreen("contacts");
    }
    else if (id == 1303) // + Add Contact
    {
        showAddContactDialog();
    }
    else if (id == 1304) // Toggle favorites filter
    {
        filterOnlyFavorites = !filterOnlyFavorites;
        showScreen("contacts");
    }
    else if (id == 1305 && HIWORD(wParam) == CBN_SELCHANGE) // Group filter combo
    {
        LRESULT sel = SendMessage(GetDlgItem(hWnd, 1305), CB_GETCURSEL, 0, 0);
        const auto& groups = GroupService::getGroups();
        if (sel > 0 && (size_t)(sel - 1) < groups.size())
        {
            filterGroupId = groups[sel - 1].id;
        }
        else
        {
            filterGroupId = 0;
        }
        showScreen("contacts");
    }
    else if (id == 1306) // Manage Groups
    {
        showManageGroupsDialog();
    }
    else if (id == 1307) // View Contact
    {
        if (selectedContactId != 0) showContactDetailsModal();
        else MessageBoxA(hWnd, "Please select a contact to view.", "View Contact", MB_OK | MB_ICONINFORMATION);
    }
    else if (id == 1308) // Edit Contact
    {
        if (selectedContactId != 0) showEditContactDialog();
        else MessageBoxA(hWnd, "Please select a contact to edit.", "Edit Contact", MB_OK | MB_ICONINFORMATION);
    }
    else if (id == 1309) // Delete Contact
    {
        if (selectedContactId != 0) showDeleteContactConfirm();
        else MessageBoxA(hWnd, "Please select a contact to delete.", "Delete Contact", MB_OK | MB_ICONINFORMATION);
    }
    else if (id == 1310) // Toggle favorite on selected contact
    {
        if (selectedContactId != 0)
        {
            ContactService::toggleFavorite(selectedContactId);
            showScreen("contacts");
        }
    }
    else if (id == 1311) // Back / Logout
    {
        handleLogout();
    }
    else if (id == 1312) // ListBox selection change
    {
        if (HIWORD(wParam) == LBN_SELCHANGE)
        {
            LRESULT sel = SendMessage(GetDlgItem(hWnd, 1312), LB_GETCURSEL, 0, 0);
            auto filtered = getFilteredContacts();
            if (sel >= 0 && (size_t)sel < filtered.size())
            {
                selectedContactId = filtered[sel].id;
                InvalidateRect(hWnd, nullptr, TRUE);
            }
        }
        else if (HIWORD(wParam) == LBN_DBLCLK)
        {
            showContactDetailsModal();
        }
    }
    else if (id == 1313) // Admin Workspace button
    {
        showScreen("admin_users");
    }

    // Screen 5: Admin Workspace Nav Buttons
    else if (id == 2001) showScreen("admin_users");
    else if (id == 2002) showScreen("admin_reports");
    else if (id == 2003) showScreen("admin_backup");
    else if (id == 2004) showScreen("admin_recovery");
    else if (id == 2005) showScreen("contacts");

    // Admin Users View Buttons
    else if (id == 2102) // Search users
    {
        adminUserSearchQuery = toNarrowString(getEditTextW(hWnd, 2101));
        showScreen("admin_users");
    }
    else if (id == 2105) // Add user
    {
        showAdminAddUserDialog();
    }
    else if (id == 2106 && HIWORD(wParam) == LBN_SELCHANGE)
    {
        LRESULT sel = SendMessage(GetDlgItem(hWnd, 2106), LB_GETCURSEL, 0, 0);
        auto filtered = getFilteredAccounts();
        if (sel >= 0 && (size_t)sel < filtered.size())
        {
            selectedAccountId = filtered[sel].id;
        }
    }
    else if (id == 2110) // Delete user
    {
        if (selectedAccountId != 0 && selectedAccountId != accountService.getCurrentUserId())
        {
            if (MessageBoxA(hWnd, "Are you sure you want to delete this user account?", "Delete User", MB_YESNO | MB_ICONWARNING) == IDYES)
            {
                db.deleteAccount(selectedAccountId);
                selectedAccountId = 0;
                showScreen("admin_users");
            }
        }
        else
        {
            MessageBoxA(hWnd, "Cannot delete the current or non-selected user account.", "Delete User", MB_OK | MB_ICONWARNING);
        }
    }

    // Admin Backup View Buttons
    else if (id == 2302) // Browse backup location
    {
        char filename[MAX_PATH] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Backup Files (*.backup)\0*.backup\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_PATHMUSTEXIST;
        if (GetSaveFileNameA(&ofn))
        {
            backupFilePath = filename;
            SetDlgItemTextW(hWnd, 2301, toWString(backupFilePath).c_str());
        }
    }
    else if (id == 2303) // Create Backup
    {
        std::error_code error;
        for (const std::string& suffix : {"", ".contacts", ".groups", ".memberships"})
        {
            const std::filesystem::path source = db.getStorageFile() + suffix;
            const std::filesystem::path backup = source.string() + ".backup";
            if (std::filesystem::exists(source))
            {
                std::filesystem::copy_file(source, backup, std::filesystem::copy_options::overwrite_existing, error);
            }
        }
        MessageBoxA(hWnd, error ? "Backup failed." : "System backup created successfully.", "Backup",
                    MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
        showScreen("admin_backup");
    }
    else if (id == 2304) // Refresh backup history
    {
        showScreen("admin_backup");
    }

    // Admin Recovery View Buttons
    else if (id == 2402) // Browse backup file to restore
    {
        char filename[MAX_PATH] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hWnd;
        ofn.lpstrFilter = "Database Backup (*.backup)\0*.backup\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn))
        {
            restoreFilePath = filename;
            SetDlgItemTextW(hWnd, 2401, toWString(restoreFilePath).c_str());
            restoreValidated = false;
            InvalidateRect(hWnd, nullptr, TRUE);
        }
    }
    else if (id == 2403) // Validate backup
    {
        if (std::filesystem::exists(db.getStorageFile() + ".backup") || !restoreFilePath.empty())
        {
            restoreValidated = true;
            restoreContactsCount = (int)ContactService::getContacts().size();
            restoreGroupsCount = (int)GroupService::getGroups().size();
            restoreUsersCount = (int)db.getAccounts().size();
            InvalidateRect(hWnd, nullptr, TRUE);
            MessageBoxA(hWnd, "Backup archive validated successfully.", "Validate Backup", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxA(hWnd, "Please select an existing backup archive file first.", "Validate Backup", MB_OK | MB_ICONWARNING);
        }
    }
    else if (id == 2405) // Restore data button
    {
        if (restoreValidated)
        {
            showAdminConfirmRestore();
        }
        else
        {
            MessageBoxA(hWnd, "Please validate the backup archive before restoring.", "Data Recovery", MB_OK | MB_ICONWARNING);
        }
    }
    else if (id == 2404) // Cancel recovery
    {
        showScreen("admin_users");
    }
}

// =========================================================================
// WINDOW PROCEDURE & MAIN LOOP
// =========================================================================

LRESULT CALLBACK MenuUI::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MenuUI* ui = reinterpret_cast<MenuUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        ui = reinterpret_cast<MenuUI*>(cs->lpCreateParams);
        ui->hWnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));
    }

    if (!ui) return DefWindowProc(hwnd, uMsg, wParam, lParam);

    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        ui->onPaint(hdc, rc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    if (uMsg == WM_ERASEBKGND)
    {
        return 1; // Handled in WM_PAINT double-buffering
    }

    if (uMsg == WM_DRAWITEM)
    {
        DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
        ui->onDrawItem(pDIS);
        return TRUE;
    }

    if (uMsg == WM_COMMAND)
    {
        ui->handleCommand(wParam, lParam);
        return 0;
    }

    if (uMsg == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORSTATIC)
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        if (uMsg == WM_CTLCOLOREDIT)
        {
            SetTextColor(hdc, UITheme::TextPrimary);
            return (LRESULT)ui->hBrushCard;
        }
        SetTextColor(hdc, UITheme::TextBody);
        return (LRESULT)ui->hBrushBg;
    }

    if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int MenuUI::run()
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // Connect DB / Fallback storage
    db.connect("127.0.0.1", "root", "", "phonebook");

    createFonts();
    createBrushes();

    // Register Main Window Class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MenuUI::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kMainWndClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBrushBg;
    RegisterClassExW(&wc);

    // Register Modal Dialog Class
    WNDCLASSEXW dc = {};
    dc.cbSize = sizeof(WNDCLASSEXW);
    dc.lpfnWndProc = ModalDialogProc;
    dc.hInstance = hInstance;
    dc.lpszClassName = kDialogWndClass;
    dc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    dc.hbrBackground = hBrushBg;
    RegisterClassExW(&dc);

    // Initial Login window size: 900 x 650 centered
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - 900) / 2;
    int posY = (screenH - 650) / 2;

    hWnd = CreateWindowExW(
        0, kMainWndClass, L"Phone Book Management System",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, 900, 650,
        nullptr, nullptr, hInstance, this);

    if (!hWnd) return 1;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    showScreen("login");

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
