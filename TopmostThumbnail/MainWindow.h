#pragma once
#include <robmikh.common/DesktopWindow.h>

struct MainWindow : robmikh::common::desktop::DesktopWindow<MainWindow>
{
	static const std::wstring ClassName;
	MainWindow(winrt::Windows::UI::Composition::Compositor const& compositor, std::wstring const& titleString, HWND windowToThumbnail);
	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
	enum class SnipStatus
	{
		None,
		Ongoing,
		Completed,
	};

	static const float BorderThickness;
	static void RegisterWindowClass();

	void OnLeftButtonDown(int x, int y);
	void OnLeftButtonUp(int x, int y);
	void OnMouseMove(int x, int y);
	void OnRightButtonUp();

private:
	winrt::Windows::System::DispatcherQueue m_mainThread{ nullptr };
	winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
	winrt::Windows::UI::Composition::CompositionTarget m_target{ nullptr };
	winrt::Windows::UI::Composition::SpriteVisual m_root{ nullptr };
	winrt::Windows::UI::Composition::SpriteVisual m_snipVisual{ nullptr };

	HWND m_windowToThumbnail = nullptr;
	HTHUMBNAIL m_thumbnail = nullptr;
	SnipStatus m_snipStatus = SnipStatus::None;
	POINT m_startPosition = {};
	RECT m_snipRect = {};
	RECT m_destRect = {};
	RECT m_sourceRect = {};
};