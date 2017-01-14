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
	
	class RenderTarget;
	
	struct luaHWND
	{
		luaHWND(HWND h){hwnd = h;}
		HWND hwnd;
	};

	struct DescWindow
	{
		DescWindow()
		{
			caption = L"[caption here]";
			width = 500;
			height = 500;
			posx = 0;
			posy = 0;
			resizing = true;
			borderWidth = 5;
			bg_color = &black_color;
			border_color = &black_color;
			border_focus_color = &black_color;
		}

		int posx;
		int posy;
		std::wstring caption;	
		int width;				
		int height;				
		bool resizing;
		RECT captionRect;
		int borderWidth;
		XMFLOAT4* bg_color;
		XMFLOAT4* border_color;
		XMFLOAT4* border_focus_color;
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
 
		DropTarget(void* window);
		virtual ~DropTarget();
 
	private:
		void*  m_window;
		ULONG m_refCount;
	};

	class Window
	{
	public:
		Window();
		~Window() { Close(); }

		bool Create(int16_t id, const DescWindow &desc, bool main = false);
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

		// Вернуть заголовок окна
		string GetCaption() const {return WstringToString(m_desc.caption);}
		void SetCaption(string str) 
		{
			m_desc.caption = StringToWstring(str);
			SetWindowText(m_hwnd, m_desc.caption.c_str());
		}

		//string _GetCaption() const {return WstringToString(GetCaption());}
		//void _SetCaption(string str) {SetCaption(StringToWstring(str));}

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

		void SetCaptionBorderSize(int32_t left, int32_t right, int32_t top, int32_t bottom, int32_t border)
		{
			m_desc.captionRect.left = left;
			m_desc.captionRect.top = top;
			m_desc.captionRect.right = right;
			m_desc.captionRect.bottom = bottom;
			m_desc.borderWidth = border;
		}

		void SetCaptionH(int32_t bottom) {m_desc.captionRect.bottom = bottom;}
		int32_t GetCaptionH() {return m_desc.captionRect.bottom;}

		void Show(bool visible = true)
		{
			ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
			UpdateWindow(m_hwnd);
			SetCursor(crs_arrow);
		}

		bool IsHover() const {return b_hover;}

		// обработка событий
		LRESULT WndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

		DescWindow m_desc;	// описание окна

	/*	void ResizeEnding()
		{
			//InvalidateRect(m_hwnd, NULL, TRUE);
			//UpdateWindow(m_hwnd);
			//RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_FRAME | RDW_INVALIDATE);
			//ShowWindow(m_hwnd, SW_HIDE);
			//ShowWindow(m_hwnd, SW_SHOW);
			//SetActiveWindow(m_hwnd);
			SetWindowPos(m_hwnd, HWND_TOP, m_desc.posx, m_desc.posy, m_desc.width, m_desc.height, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(m_hwnd, HWND_TOP, m_desc.posx, m_desc.posy, m_desc.width, m_desc.height, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
		}*/

		bool m_finResize;
		bool m_begResize;

		void SetColorBg(XMFLOAT4* color) {m_desc.bg_color = color;}
		void SetColorBorder(XMFLOAT4* color) {m_desc.border_color = color;}
		void SetColorBorderFocus(XMFLOAT4* color) {m_desc.border_focus_color = color;}

		inline XMFLOAT4* GetColorBg() const {return m_desc.bg_color;}
		inline XMFLOAT4* GetColorBorder() const {return m_desc.border_color;}
		inline XMFLOAT4* GetColorBorderFocus() const {return m_desc.border_focus_color;}

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

		static void RegLuaClass()
		{
			getGlobalNamespace(LSTATE)
				.beginClass<Window>("Window")
					.addFunction("Minimize", &Window::Minimize)
					.addFunction("Maximize", &Window::Maximize)
					.addFunction("RestoreSize", &Window::RestoreSize)
					.addFunction("UserClose", &Window::UserClose)

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
					.addFunction("SetCaptionBorderSize", &Window::SetCaptionBorderSize)
					.addFunction("GetCaptionH", &Window::GetCaptionH)
					.addFunction("SetCaptionH", &Window::SetCaptionH)

					.addProperty("caption_text", &Window::GetCaption, &Window::SetCaption)

					.addFunction("SetColorBgPtr", &Window::SetColorBg)
					.addFunction("SetColorBorderPtr", &Window::SetColorBorder)
					.addFunction("SetColorBorderFocusPtr", &Window::SetColorBorderFocus)

					.addFunction("SetAlpha", &Window::SetAlpha)

					.addFunction("Show", &Window::Show)
				.endClass()
				
				.beginClass<luaHWND>("HWND")
				.endClass();
		}	

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
		float fpslock;
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

	static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
}