#pragma once

#include "EngineSettings.h"
#include "DataTypes.h"
#include "Log.h"
#include "Timer.h"
#include "Util.h"
#include "LuaVM.h"
#include "InputCodes.h"
#include "GlobalColor.h"

namespace EngineCore
{
//------------------------------------------------------------------
	
#define SYSTEM_BORDER_SIZE 7

	class RenderTarget;
	
	struct luaHWND
	{
		luaHWND(HWND h){hwnd = h;}
		luaHWND(){hwnd = NULL;}
		HWND hwnd;
	};

	struct DescWindow
	{
		DescWindow()
		{
			noWinBorder = false;
			caption = "[caption here]";
			width = 500;
			height = 500;
			posx = 0;
			posy = 0;
			borderWidth = 0;

			captionRect.bottom = 0;
			captionRect.top = 0;
			captionRect.left = 0;
			captionRect.right = 0;

			bg_color = &black_color;
			border_color = &black_color;
			border_focus_color = &black_color;
		}

		bool noWinBorder;
		int posx;
		int posy;
		string caption;	
		int width;				
		int height;		
		RECT captionRect;
		int borderWidth;
		Vector4* bg_color;
		Vector4* border_color;
		Vector4* border_focus_color;
	};
		
	class DropTarget : public IDropTarget
	{
	public:
		HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
		ULONG   __stdcall AddRef (void);
		ULONG   __stdcall Release (void);
 
		HRESULT __stdcall DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
		HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
		HRESULT __stdcall DragLeave(void);
		HRESULT __stdcall Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
 
		DropTarget(void* win);
		virtual ~DropTarget();
 
	private:
		void* window;
		ULONG refCount;
	};

	class Window
	{
	public:
		Window();
		~Window() { Close(); }

		bool Create(int16_t id, bool main = false);
		bool CreateSwapChain();

		inline bool IsNull() {return m_hwnd == 0;}
		inline int16_t GetSystemId() {return systemId;}

		void SetRenderTarget();
		void ClearRenderTarget();
		void Swap();

		void AfterRunEvent();

		void Close();

		void Minimize()
		{
			ShowWindow(m_hwnd, SW_MINIMIZE);
		}
		void Maximize()
		{
			ShowWindow(m_hwnd, SW_MAXIMIZE);
		}
		void RestoreSize()
		{
			ShowWindow(m_hwnd, SW_RESTORE);
		}
		void UserClose()
		{
			SendMessage(m_hwnd, WM_CLOSE, 0, 0);
		}

		void SetActive()
		{
			SetActiveWindow(m_hwnd);
		}

		bool IsMaximized(){return m_maximized;}
		bool IsMinimized(){return m_minimized;}

		inline void UpdateWindowState();
		
		inline HWND GetHWND() const {return m_hwnd;}
		inline luaHWND _GetHWND() const {return luaHWND(m_hwnd);}
		inline int GetLeft() const {return m_desc.posx;}
		inline int GetTop() const {return m_desc.posy;}
		inline int GetWidth() const {return m_desc.width;}
		inline int GetHeight() const {return m_desc.height;}
		
		XMMATRIX* Window::GetOrtho() {return &m_ortho;}

		string GetCaption() const {return m_desc.caption;}
		void SetCaption(string str) 
		{
			m_desc.caption = str;
			SetWindowText(m_hwnd, StringToWstring(m_desc.caption).c_str());
		}
		
		bool IsExit() const {return m_isexit;}
		bool IsActive() const {return m_active;}
		bool IsResize() const {return m_isresize;}
		bool IsStartResize() const {return m_isresize && m_begResize;}
		bool IsEndResize() const {return m_finResize;}

		bool IsMain()  const {return b_main;}

		void SetSize(int width, int height)
		{
			m_desc.width = width;
			m_desc.height = height;
			UpdateWindowSizePos();
		}

		MLRECT GetPosSize()
		{
			return MLRECT(m_desc.posx, m_desc.posy, m_desc.width, m_desc.height);
		}

		void SetPos(int x, int y)
		{
			m_desc.posx = x;
			m_desc.posy = y;
			UpdateWindowSizePos();
		}

		void SetPosSize(int x, int y, int width, int height)
		{
			m_desc.posx = x;
			m_desc.posy = y;
			m_desc.width = width;
			m_desc.height = height;
			UpdateWindowSizePos();
		}

		void SetBorderSize(int32_t border)
		{
			if(m_desc.noWinBorder)
				m_desc.borderWidth = border;
		}
		int32_t GetBorderSize(int32_t border) const
		{
			return m_desc.borderWidth;
		}

		void SetCaptionRect(int32_t left, int32_t right, int32_t top, int32_t bottom)
		{
			if(!m_desc.noWinBorder)
				return;
			m_desc.captionRect.left = left;
			m_desc.captionRect.top = top;
			m_desc.captionRect.right = right;
			m_desc.captionRect.bottom = bottom;
		}
		RECT GetCaptionRect() const
		{
			RECT res;
			res.left = m_desc.captionRect.left;
			res.top = m_desc.captionRect.top;
			res.right = m_desc.captionRect.right;
			res.bottom = m_desc.captionRect.bottom;
			return res;
		}

		void SetResizable(bool allow)
		{
			LONG_PTR style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
			if(allow)
				style = style | WS_THICKFRAME;
			else
				style = style & ~WS_THICKFRAME;

			SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
			SetWindowPos(m_hwnd, 0,0,0,0,0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
		}
		void SetMinMaxBox(bool allow)
		{
			LONG_PTR style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
			if(allow)
				style = style | (WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			else
				style = style & ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
			SetWindowPos(m_hwnd, 0,0,0,0,0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
		}
		void SetSysMenu(bool allow)
		{
			LONG_PTR style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
			if(allow)
				style = style | WS_SYSMENU;
			else
				style = style & ~WS_SYSMENU;

			SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
			SetWindowPos(m_hwnd, 0,0,0,0,0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
		}

		void SetPopup(bool is_popup)
		{
			LONG_PTR style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
			if(is_popup)
			{
				style = style & ~WS_OVERLAPPEDWINDOW;
				style = style | WS_POPUP;
			}
			else
			{
				style = style & ~WS_POPUP;
				style = style | WS_OVERLAPPEDWINDOW;
			}

			SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
			SetWindowPos(m_hwnd, 0,0,0,0,0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DRAWFRAME);
		}

		void Show(bool visible = true)
		{
			ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
			UpdateWindow(m_hwnd);
			SetCursor(crs_arrow);
		}

		bool IsHover() const {return b_hover;}

		LRESULT WndProc(HWND hwnd, uint32_t nMsg, WPARAM wParam, LPARAM lParam);
		
		void SetColorBg(Vector4* color) {m_desc.bg_color = color;}
		void SetColorBorder(Vector4* color) {m_desc.border_color = color;}
		void SetColorBorderFocus(Vector4* color) {m_desc.border_focus_color = color;}

		inline Vector4* GetColorBg() const {return m_desc.bg_color;}
		inline Vector4* GetColorBorder() const {return m_desc.border_color;}
		inline Vector4* GetColorBorderFocus() const {return m_desc.border_focus_color;}

		void SetAlpha(float alpha)
		{
			if(alpha < 1.0f)
			{
				SetWindowLong(m_hwnd, GWL_EXSTYLE, GetWindowLong(m_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
				SetLayeredWindowAttributes(m_hwnd, 0, (BYTE) (alpha * 255), LWA_ALPHA);
			}
			else
				SetWindowLong(m_hwnd, GWL_EXSTYLE, GetWindowLong(m_hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		}

		// Call before resize
		void HideWinBorder(bool hide)
		{
			m_desc.noWinBorder = hide;
		}
		bool IsWinBorderHided() const
		{
			return m_desc.noWinBorder;
		}

		bool SetIcons(string icon_big, string icon_small);

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<Window>("Window")
					.addFunction("Minimize", &Window::Minimize)
					.addFunction("Maximize", &Window::Maximize)
					.addFunction("RestoreSize", &Window::RestoreSize)
					.addFunction("UserClose", &Window::UserClose)
					.addFunction("SetActive", &Window::SetActive)

					.addFunction("IsMaximized", &Window::IsMaximized)
					.addFunction("IsMinimized", &Window::IsMinimized)

					.addFunction("GetHWND", &Window::_GetHWND)

					.addFunction("GetLeft", &Window::GetLeft)
					.addFunction("GetTop", &Window::GetTop)
					.addFunction("GetWidth", &Window::GetWidth)
					.addFunction("GetHeight", &Window::GetHeight)

					.addFunction("IsActive", &Window::IsActive)
					.addFunction("IsResize", &Window::IsResize)
					.addFunction("IsStartResize", &Window::IsStartResize)
					.addFunction("IsEndResize", &Window::IsEndResize)
					.addFunction("IsExit", &Window::IsExit)
					.addFunction("IsMain", &Window::IsMain)
					.addFunction("IsHover", &Window::IsHover)

					.addFunction("SetSize", &Window::SetSize)
					.addFunction("GetPosSize", &Window::GetPosSize)
					.addFunction("SetPos", &Window::SetPos)
					.addFunction("SetPosSize", &Window::SetPosSize)

					.addFunction("SetCaptionRect", &Window::SetCaptionRect)
					.addFunction("GetCaptionRect", &Window::GetCaptionRect)
					.addFunction("SetBorderSize", &Window::SetBorderSize)
					.addFunction("GetBorderSize", &Window::GetBorderSize)

					.addFunction("SetResizable", &Window::SetResizable)
					.addFunction("SetMinMaxBox", &Window::SetMinMaxBox)
					.addFunction("SetSysMenu", &Window::SetSysMenu)

					.addFunction("SetPopup", &Window::SetPopup)

					.addFunction("HideWinBorder", &Window::HideWinBorder)
					.addFunction("IsWinBorderHided", &Window::IsWinBorderHided)

					.addProperty("caption_text", &Window::GetCaption, &Window::SetCaption)

					.addFunction("SetColorBgPtr", &Window::SetColorBg)
					.addFunction("SetColorBorderPtr", &Window::SetColorBorder)
					.addFunction("SetColorBorderFocusPtr", &Window::SetColorBorderFocus)

					.addFunction("SetAlpha", &Window::SetAlpha)

					.addFunction("SetIcons", &Window::SetIcons)

					.addFunction("Show", &Window::Show)
				.endClass()
				
				.beginClass<luaHWND>("HWND")
					.addConstructor<void (*)()>() 
				.endClass();
		}	


		DescWindow m_desc;

		bool m_finResize;
		bool m_begResize;

		ALIGNED_ALLOCATION

	private:

		inline void ForceRedraw();

		void UpdateWindowSizePos()
		{
			BOOL r;
			r = SetWindowPos(m_hwnd, m_hwnd, m_desc.posx, m_desc.posy, m_desc.width, m_desc.height, SWP_NOZORDER);
			//m_isresize = true;
			resize();
		}

		bool resize();
		
		int16_t systemId;
		HWND m_hwnd;
		bool b_main;

		bool m_isexit;
		bool m_active;
		bool m_minimized;
		bool m_maximized;
		bool m_isresize;

		XMMATRIX m_ortho;

		IDXGISwapChain1 *m_pSwapChain;
		RenderTarget* m_RTmain;
		
		DXGI_PRESENT_PARAMETERS presetParams;

		double rendertime;
		Timer* m_timer;

		bool b_hover;
		HCURSOR crs_arrow;

		DropTarget* m_dropTarget;

		bool mouseMoved;
		void mouseMoveProcess();

		int16_t mouseX, mouseY;

		bool is_tracking_mouse;
		bool is_press_captured;
	};

	static LRESULT CALLBACK StaticWndProc(HWND hwnd, uint32_t nMsg, WPARAM wParam, LPARAM lParam);
}