#pragma once
#include <robmikh.common/DesktopWindow.h>

struct MainWindow : robmikh::common::desktop::DesktopWindow<MainWindow>
{
	static const std::wstring ClassName;
	static void RegisterWindowClass();
	MainWindow(std::wstring const& titleString, HWND windowToThumbnail);
	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
	HWND m_windowToThumbnail = nullptr;
	HTHUMBNAIL m_thumbnail = nullptr;
};