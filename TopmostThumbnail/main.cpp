#include "pch.h"
#include "MainWindow.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::desktop;
}

util::WindowInfo GetWindowToCapture(std::vector<util::WindowInfo> const& windows);

int wmain(int argc, wchar_t* argv[])
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // Initialize COM
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Args
    std::vector<std::wstring> args(argv + 1, argv + argc);
    if (args.size() <= 0)
    {
        throw std::runtime_error("Invalid input!");
    }

    // Get the window to thumbnail
    auto windowQuery = args[0];
    wprintf(L"Looking for \"%s\"...\n", windowQuery.c_str());

    auto windows = util::FindTopLevelWindowsByTitle(windowQuery);
    if (windows.size() == 0)
    {
        wprintf(L"No windows found!\n");
        return 1;
    }

    auto foundWindow = GetWindowToCapture(windows);
    wprintf(L"Using window \"%s\"\n", foundWindow.Title.c_str());
    auto windowToThumbnail = foundWindow.WindowHandle;

    // Create the DispatcherQueue that the compositor needs to run
    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    // Create our Compositor
    auto compositor = winrt::Compositor();

    // Create our thumbnail window
    auto window = MainWindow(compositor, L"TopmostThumbnail", windowToThumbnail);

    // Message pump
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

util::WindowInfo GetWindowToCapture(std::vector<util::WindowInfo> const& windows)
{
    if (windows.empty())
    {
        throw winrt::hresult_invalid_argument();
    }

    auto numWindowsFound = windows.size();
    auto foundWindow = windows[0];
    if (numWindowsFound > 1)
    {
        wprintf(L"Found %I64u windows that match:\n", numWindowsFound);
        wprintf(L"    Num    PID       Window Title\n");
        auto count = 0;
        for (auto const& window : windows)
        {
            DWORD pid = 0;
            auto ignored = GetWindowThreadProcessId(window.WindowHandle, &pid);
            wprintf(L"    %3i    %06u    %s\n", count, pid, window.Title.c_str());
            count++;
        }

        do
        {
            wprintf(L"Please make a selection (q to quit): ");
            std::wstring selectionString;
            std::getline(std::wcin, selectionString);
            if (selectionString.rfind(L"q", 0) == 0 ||
                selectionString.rfind(L"Q", 0) == 0)
            {
                return 0;
            }
            auto selection = std::stoi(selectionString);
            if (selection >= 0 && selection < windows.size())
            {
                foundWindow = windows[selection];
                break;
            }
            else
            {
                wprintf(L"Invalid input, '%s'!\n", selectionString.c_str());
            }
        } while (true);
    }

    return foundWindow;
}