#include "pch.h"
#include "MainWindow.h"

const std::wstring MainWindow::ClassName = L"TopmostThumbnail.MainWindow";

void MainWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEXW wcex = {};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbSize = sizeof(wcex);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(instance, IDI_APPLICATION);
    winrt::check_bool(RegisterClassExW(&wcex));
}

MainWindow::MainWindow(std::wstring const& titleString, HWND windowToThumbnail)
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));

    m_windowToThumbnail = windowToThumbnail;

    RECT windowToThumbnailRect = {};
    winrt::check_hresult(DwmGetWindowAttribute(windowToThumbnail, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&windowToThumbnailRect), sizeof(windowToThumbnailRect)));

    auto exStyle = WS_EX_TOPMOST;
    auto style = WS_OVERLAPPEDWINDOW;

    RECT rect = { 0, 0, windowToThumbnailRect.right - windowToThumbnailRect.left, windowToThumbnailRect.bottom - windowToThumbnailRect.top };
    winrt::check_bool(AdjustWindowRectEx(&rect, style, false, exStyle));

    auto width = rect.right - rect.left;
    auto height = rect.bottom - rect.top;

    winrt::check_bool(CreateWindowExW(WS_EX_TOPMOST, ClassName.c_str(), titleString.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    winrt::check_hresult(DwmRegisterThumbnail(m_window, windowToThumbnail, &m_thumbnail));

    RECT clientRect = {};
    winrt::check_bool(GetClientRect(m_window, &clientRect));

    DWM_THUMBNAIL_PROPERTIES properties = {};
    properties.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_VISIBLE | DWM_TNP_OPACITY | DWM_TNP_RECTDESTINATION;
    properties.fSourceClientAreaOnly = false;
    properties.fVisible = true;
    properties.opacity = 255;
    properties.rcDestination = clientRect;
    winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));

    ShowWindow(m_window, SW_SHOWDEFAULT);
    UpdateWindow(m_window);
}

LRESULT MainWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    switch (message)
    {
    case WM_SIZE:
    case WM_SIZING:
    {
        RECT windowToThumbnailRect = {};
        winrt::check_hresult(DwmGetWindowAttribute(m_windowToThumbnail, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&windowToThumbnailRect), sizeof(windowToThumbnailRect)));

        RECT clientRect = {};
        winrt::check_bool(GetClientRect(m_window, &clientRect));

        DWM_THUMBNAIL_PROPERTIES properties = {};
        properties.dwFlags = DWM_TNP_RECTDESTINATION;
        properties.rcDestination = clientRect;
        winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));
    }
        break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
    }
    return 0;
}
