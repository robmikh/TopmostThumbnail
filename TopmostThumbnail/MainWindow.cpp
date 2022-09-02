#include "pch.h"
#include "MainWindow.h"

namespace winrt
{
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

const std::wstring MainWindow::ClassName = L"TopmostThumbnail.MainWindow";
const float MainWindow::BorderThickness = 5;
std::once_flag MainWindowClassRegistration;

float ComputeScaleFactor(RECT const& windowRect, RECT const& contentRect)
{
    auto windowWidth = static_cast<float>(windowRect.right - windowRect.left);
    auto windowHeight = static_cast<float>(windowRect.bottom - windowRect.top);
    auto contentWidth = static_cast<float>(contentRect.right - contentRect.left);
    auto contentHeight = static_cast<float>(contentRect.bottom - contentRect.top);

    auto windowRatio = windowWidth / windowHeight;
    auto contentRatio = contentWidth / contentHeight;

    auto scaleFactor = windowWidth / contentWidth;
    if (windowRatio > contentRatio)
    {
        scaleFactor = windowHeight / contentHeight;
    }

    return scaleFactor;
}

RECT ComputeDestRect(RECT const& windowRect, RECT const& contentRect)
{
    auto scaleFactor = ComputeScaleFactor(windowRect, contentRect);

    auto windowWidth = static_cast<float>(windowRect.right - windowRect.left);
    auto windowHeight = static_cast<float>(windowRect.bottom - windowRect.top);
    auto contentWidth = static_cast<float>(contentRect.right - contentRect.left) * scaleFactor;
    auto contentHeight = static_cast<float>(contentRect.bottom - contentRect.top) * scaleFactor;

    auto remainingWidth = windowWidth - contentWidth;
    auto remainingHeight = windowHeight - contentHeight;

    auto left = static_cast<LONG>(remainingWidth / 2.0f);
    auto top = static_cast<LONG>(remainingHeight / 2.0f);
    auto right = left + static_cast<LONG>(contentWidth);
    auto bottom = top + static_cast<LONG>(contentHeight);

    return RECT{ left, top, right, bottom };
}

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
    wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(instance, IDI_APPLICATION);
    winrt::check_bool(RegisterClassExW(&wcex));
}

MainWindow::MainWindow(winrt::Compositor const& compositor, std::wstring const& titleString, HWND windowToThumbnail)
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    m_compositor = compositor;

    std::call_once(MainWindowClassRegistration, []() { RegisterWindowClass(); });

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
    m_destRect = clientRect;
    m_sourceRect = rect;

    DWM_THUMBNAIL_PROPERTIES properties = {};
    properties.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_VISIBLE | DWM_TNP_OPACITY | DWM_TNP_RECTDESTINATION;
    properties.fSourceClientAreaOnly = false;
    properties.fVisible = true;
    properties.opacity = 255;
    properties.rcDestination = clientRect;
    winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));

    // Create the visual tree
    m_target = CreateWindowTarget(m_compositor);
    m_root = m_compositor.CreateSpriteVisual();
    m_root.RelativeSizeAdjustment({ 1, 1 });
    m_root.Brush(m_compositor.CreateColorBrush(winrt::Color{ 0, 0, 0, 0 }));
    m_snipVisual = m_compositor.CreateSpriteVisual();
    auto brush = m_compositor.CreateNineGridBrush();
    brush.SetInsets(BorderThickness);
    brush.IsCenterHollow(true);
    brush.Source(m_compositor.CreateColorBrush(winrt::Color{ 255, 255, 0, 0 }));
    m_snipVisual.Brush(brush);
    m_target.Root(m_root);
    m_snipVisual.IsVisible(false);
    m_root.Children().InsertAtTop(m_snipVisual);

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
        //RECT windowToThumbnailRect = {};
        //winrt::check_hresult(DwmGetWindowAttribute(m_windowToThumbnail, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&windowToThumbnailRect), sizeof(windowToThumbnailRect)));

        RECT clientRect = {};
        winrt::check_bool(GetClientRect(m_window, &clientRect));

        m_destRect = ComputeDestRect(clientRect, m_sourceRect);

        DWM_THUMBNAIL_PROPERTIES properties = {};
        properties.dwFlags = DWM_TNP_RECTDESTINATION;
        properties.rcDestination = m_destRect;
        winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));
    }
        break;
    case WM_LBUTTONDOWN:
    {
        auto xPos = GET_X_LPARAM(lparam);
        auto yPos = GET_Y_LPARAM(lparam);
        OnLeftButtonDown(xPos, yPos);
    }
    break;
    case WM_LBUTTONUP:
    {
        auto xPos = GET_X_LPARAM(lparam);
        auto yPos = GET_Y_LPARAM(lparam);
        OnLeftButtonUp(xPos, yPos);
    }
    break;
    case WM_MOUSEMOVE:
    {
        auto xPos = GET_X_LPARAM(lparam);
        auto yPos = GET_Y_LPARAM(lparam);
        OnMouseMove(xPos, yPos);
    }
    break;
    case WM_RBUTTONUP:
    {
        OnRightButtonUp();
    }
    break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
    }
    return 0;
}

void MainWindow::OnLeftButtonDown(int x, int y)
{
    if (m_snipStatus == SnipStatus::None)
    {
        m_snipStatus = SnipStatus::Ongoing;
        m_snipVisual.Offset({ x - BorderThickness, y - BorderThickness, 0 });
        m_snipVisual.Size({ 0, 0 });
        m_startPosition = { x, y };
        m_snipVisual.IsVisible(true);
    }
}

void MainWindow::OnLeftButtonUp(int x, int y)
{
    if (m_snipStatus == SnipStatus::Ongoing)
    {
        m_snipStatus = SnipStatus::Completed;
        m_snipVisual.IsVisible(false);

        // Compute our crop rect
        if (x < m_startPosition.x)
        {
            m_snipRect.left = x;
            m_snipRect.right = m_startPosition.x;
        }
        else
        {
            m_snipRect.left = m_startPosition.x;
            m_snipRect.right = x;
        }
        if (y < m_startPosition.y)
        {
            m_snipRect.top = y;
            m_snipRect.bottom = m_startPosition.y;
        }
        else
        {
            m_snipRect.top = m_startPosition.y;
            m_snipRect.bottom = y;
        }

        // Nothing to do if the rect is empty
        if (m_snipRect.right - m_snipRect.left == 0 || m_snipRect.bottom - m_snipRect.top == 0)
        {
            m_snipStatus = SnipStatus::None;
            return;
        }

        // Update the thumbnail
        
        // Calculate our source rect from the snip rect
        RECT newSourceRect = {};
        // First we need to intersect our snip rect with the current dest rect
        newSourceRect.left = std::max(m_snipRect.left, m_destRect.left);
        newSourceRect.top = std::max(m_snipRect.top, m_destRect.top);
        newSourceRect.right = std::min(m_snipRect.right, m_destRect.right);
        newSourceRect.bottom = std::min(m_snipRect.bottom, m_destRect.bottom);

        // Make our source rect relative to our current dest rect
        newSourceRect.left -= m_destRect.left;
        newSourceRect.top -= m_destRect.top;
        newSourceRect.right -= m_destRect.left;
        newSourceRect.bottom -= m_destRect.top;

        RECT clientRect = {};
        winrt::check_bool(GetClientRect(m_window, &clientRect));
        RECT windowToThumbnailRect = {};
        winrt::check_hresult(DwmGetWindowAttribute(m_windowToThumbnail, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&windowToThumbnailRect), sizeof(windowToThumbnailRect)));

        // The dest rect might be scaled, so undo that scale
        auto scale = ComputeScaleFactor(clientRect, windowToThumbnailRect);
        newSourceRect.left = static_cast<LONG>(static_cast<float>(newSourceRect.left) / scale);
        newSourceRect.top = static_cast<LONG>(static_cast<float>(newSourceRect.top) / scale);
        newSourceRect.right = static_cast<LONG>(static_cast<float>(newSourceRect.right) / scale);
        newSourceRect.bottom = static_cast<LONG>(static_cast<float>(newSourceRect.bottom) / scale);

        // Compute a new dest rect
        m_destRect = ComputeDestRect(clientRect, newSourceRect);
        m_sourceRect = newSourceRect;

        DWM_THUMBNAIL_PROPERTIES properties = {};
        properties.dwFlags = DWM_TNP_RECTSOURCE | DWM_TNP_RECTDESTINATION;
        properties.rcSource = newSourceRect;
        properties.rcDestination = m_destRect;
        winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));
    }
}

void MainWindow::OnMouseMove(int x, int y)
{
    if (m_snipStatus == SnipStatus::Ongoing)
    {
        auto offset = m_snipVisual.Offset();
        auto size = m_snipVisual.Size();

        if (x < m_startPosition.x)
        {
            offset.x = x - BorderThickness;
            size.x = (m_startPosition.x - x) + (2 * BorderThickness);
        }
        else
        {
            size.x = (x - m_startPosition.x) + (2 * BorderThickness);
        }

        if (y < m_startPosition.y)
        {
            offset.y = y - BorderThickness;
            size.y = (m_startPosition.y - y) + (2 * BorderThickness);
        }
        else
        {
            size.y = (y - m_startPosition.y) + (2 * BorderThickness);
        }

        m_snipVisual.Offset(offset);
        m_snipVisual.Size(size);
    }
}

void MainWindow::OnRightButtonUp()
{
    if (m_snipStatus == SnipStatus::Completed)
    {
        m_snipStatus = SnipStatus::None;

        RECT windowToThumbnailRect = {};
        winrt::check_hresult(DwmGetWindowAttribute(m_windowToThumbnail, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&windowToThumbnailRect), sizeof(windowToThumbnailRect)));
        RECT newSourceRect = {};
        newSourceRect.left = 0;
        newSourceRect.right = windowToThumbnailRect.right - windowToThumbnailRect.left;
        newSourceRect.top = 0;
        newSourceRect.bottom = windowToThumbnailRect.bottom - windowToThumbnailRect.top;

        RECT clientRect = {};
        winrt::check_bool(GetClientRect(m_window, &clientRect));

        m_destRect = ComputeDestRect(clientRect, windowToThumbnailRect);
        m_sourceRect = newSourceRect;

        DWM_THUMBNAIL_PROPERTIES properties = {};
        properties.dwFlags = DWM_TNP_RECTSOURCE | DWM_TNP_RECTDESTINATION;
        properties.rcSource = newSourceRect;
        properties.rcDestination = m_destRect;
        winrt::check_hresult(DwmUpdateThumbnailProperties(m_thumbnail, &properties));
    }
}