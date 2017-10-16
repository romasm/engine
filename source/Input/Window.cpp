#include "stdafx.h"
#include "Window.h"
#include "WindowsMgr.h"
#include "Render.h"
#include "RenderTarget.h"
#include "Hud.h"
#include "RenderTarget.h"
#include "MainLoop.h"
#include "WorldMgr.h"

#define wndClass L"MLE"

namespace EngineCore
{
//------------------------------------------------------------------
	Window::Window(void) :
		m_hwnd(0),
		m_isexit(false),
		m_active(true),
		m_minimized(false),
		m_maximized(false),
		m_isresize(false),
		m_RTmain(nullptr),
		m_pSwapChain(nullptr),
		b_main(false)
	{
		systemId = -1;

		rendertime = 0;
		m_timer = Timer::Get();

		m_finResize = false;
		m_begResize = true;
		b_hover = false;

		mouseX = 0;
		mouseY = 0;
		mouseMoved = false;

		m_ortho = XMMatrixIdentity();

		m_dropTarget = nullptr;

		is_tracking_mouse = false;
		is_press_captured = false;
		
		ZeroMemory(&presetParams, sizeof(presetParams));
	}

	bool Window::Create(int16_t id, bool main)
	{		
		crs_arrow = WindowsMgr::Get()->GetCursors(CURSOR_ARROW);

		LOG("Window Create");

		if(main)
		{
			b_main = true;

			WNDCLASSEX wnd;
			wnd.cbSize = sizeof(WNDCLASSEX);
			wnd.style = CS_HREDRAW | CS_VREDRAW;
			wnd.lpfnWndProc = StaticWndProc;
			wnd.cbClsExtra = 0;
			wnd.cbWndExtra = 0;
			wnd.hInstance = 0;
			wnd.hIcon = LoadIcon(0, IDI_WINLOGO);
			wnd.hIconSm = wnd.hIcon;
			wnd.hCursor = NULL;
			wnd.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
			wnd.lpszMenuName = 0;
			wnd.lpszClassName = wndClass;
			wnd.cbSize = sizeof(WNDCLASSEX);

			if( !RegisterClassEx( &wnd ) )
			{
				ERR("Registration of WNDCLASS failed");
				return false;
			}
		}

		if( !(m_hwnd = CreateWindowEx(0, wndClass, StringToWstring(m_desc.caption).c_str(), 
			WS_OVERLAPPED | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_EX_ACCEPTFILES,
			 m_desc.posx, m_desc.posy, m_desc.width, m_desc.height, 
			 main ? 0 : WindowsMgr::Get()->GetMainWindow()->m_hwnd, 0, 0, 0)) )
		{
			ERR("CreateWindowEx failed");
			return false;
		}

		// Windows 7?
		/*MARGINS borderless = {1,1,1,1};
		HRESULT hr = S_OK;
		hr = DwmExtendFrameIntoClientArea(m_hwnd, &borderless);
		if( FAILED(hr) )
			WRN("Не удалось вызвать DwmExtendFrameIntoClientArea");*/

		OleInitialize(nullptr);	
		m_dropTarget = new DropTarget(this);
		RegisterDragDrop(m_hwnd, m_dropTarget);

		systemId = id;

		// RAW INPUT
		RAWINPUTDEVICE Rid[3];
        
		Rid[0].usUsagePage = 0x01; 
		Rid[0].usUsage = 0x02; 
		Rid[0].dwFlags = 0;					// adds HID mouse
		Rid[0].hwndTarget = 0;

		Rid[1].usUsagePage = 0x01; 
		Rid[1].usUsage = 0x06; 
		Rid[1].dwFlags = 0;					// adds HID keyboard
		Rid[1].hwndTarget = 0;

		Rid[2].usUsagePage = 0x01; 
		Rid[2].usUsage = 0x05; 
		Rid[2].dwFlags = 0;                 // adds game pad
		Rid[2].hwndTarget = 0;

		if( RegisterRawInputDevices(Rid, 3, sizeof(Rid[0])) == FALSE ) 
		{
			ERR("Registration of RawInput devices failed");
			return false;
		}

		return true;
	}

	bool Window::CreateSwapChain()
	{
		DXGI_SWAP_CHAIN_DESC1 schd;
		ZeroMemory( &schd, sizeof( schd ) );
		schd.BufferCount = 1;
		schd.Width = m_desc.width;
		schd.Height = m_desc.height;
		schd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		schd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		schd.SampleDesc.Count = 1;
		schd.SampleDesc.Quality = 0;
		schd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		schd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	
		DXGI_SWAP_CHAIN_FULLSCREEN_DESC schdf;
		ZeroMemory( &schdf, sizeof( schdf ) );
		schdf.RefreshRate.Denominator = 1;
		schdf.RefreshRate.Numerator = 60;
		schdf.Scaling = DXGI_MODE_SCALING_CENTERED;
		schdf.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		schdf.Windowed = !CONFIG(bool, fullscreen);

		HRESULT hr = Render::Get()->m_pDxgiFactory->CreateSwapChainForHwnd( Render::Get()->m_pd3dDevice, m_hwnd, &schd, &schdf, nullptr, &m_pSwapChain );
		if( FAILED(hr) )
		{
			ERR("CreateSwapChainForHwnd failed");
			return false;
		}
		Render::Get()->m_pDxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

		// RENDER TARGET
		m_RTmain = new RenderTarget();
		if(!m_RTmain->Init(m_desc.width, m_desc.height))
		{
			ERR("Creation of main RenderTarget failed");
			return false;
		}
		m_ortho = XMMatrixOrthographicLH(float(m_desc.width), float(m_desc.height), 0.0f, 1.0f);

		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
		if( FAILED(hr) )
			return false;
		if(!m_RTmain->AddBackBufferRT(pBackBuffer))
		{
			ERR("Не удалось создать основной RenderTarget");
			return false;
		}		
		_RELEASE(pBackBuffer);

		return true;
	}

	void Window::SetRenderTarget()
	{
		m_RTmain->SetRenderTarget();
		Render::Get()->CurrentHudWindow = this;
	}

	void Window::ClearRenderTarget()
	{
		m_RTmain->ClearRenderTargets();
	}

	void Window::Swap()
	{
		/*if(EngineSettings::EngSets.vsync && EngineSettings::EngSets.fullscreen)
			m_pSwapChain->Present1(1, 0, &p); // todo: num of wait frames
		else */
			m_pSwapChain->Present1(0, 0, &presetParams);
	}

	void Window::AfterRunEvent()
	{
		if(m_isresize)
		{
			m_isresize = false;
			m_finResize = true;
			m_begResize = true;
		}

		mouseMoveProcess();
	}

	void Window::Close()
	{
		if( IsNull() )
			return;

		Hud::Get()->DestroyRoot(this);

		if (m_hwnd)
		{
			RevokeDragDrop(m_hwnd);

			DestroyWindow(m_hwnd);
		}

		if(m_dropTarget)
		{
			OleUninitialize();
			_RELEASE(m_dropTarget);
		}

		m_hwnd = 0;
		systemId = -1;

		LOG("Window Close");
	}

	bool Window::SetIcons(string icon_big, string icon_small)
	{
		HICON hIconBig = NULL;
		HICON hIconSmall = NULL;

		if(!icon_big.empty())
		{
			hIconBig = (HICON)LoadImage(NULL, StringToWstring(icon_big).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
			if(hIconBig)
				SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
			else
			{
				ERR("Unable to set big icon %s to window", icon_big.c_str());
				return false;
			}
		}
		else
		{
			SendMessage(m_hwnd, WM_SETICON, ICON_BIG, NULL);
		}

		if(!icon_small.empty())
		{
			hIconSmall = (HICON)LoadImage(NULL, StringToWstring(icon_small).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
			if(hIconSmall)
				SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
			else
			{
				ERR("Unable to set small icon %s to window", icon_small.c_str());
				return false;
			}
		}
		else
		{
			SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, NULL);
		}

		return true;
	}

	LRESULT Window::WndProc(HWND hwnd, uint32_t nMsg, WPARAM wParam, LPARAM lParam)
	{
		if(!Hud::Get())
			return DefWindowProcW( hwnd, nMsg, wParam, lParam);

		switch(nMsg)
		{
		case WM_CREATE:
			return 0;
		case WM_CLOSE:
			m_isexit = true;
			return 0;
		case WM_NCCALCSIZE:
			if(!m_desc.noWinBorder)
				break;
			if((BOOL)wParam)
				return 0;
			break;
		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE)
				m_active = true;
			else
			{
				m_active = false;
				Hud::Get()->DeactivateWindow(this);
			}
			return 0;
		case WM_MOVE:
			{
				int posX = LOWORD(lParam);
				int posY = HIWORD(lParam);
				if( (posX & 0x8000) > 0 )
					posX = -(0xffff - posX);
				if( (posY & 0x8000) > 0 )
					posY = -(0xffff - posY);
				m_desc.posx = posX;
				m_desc.posy = posY;

				ForceRedraw();

				return 0;
			}

		case WM_ERASEBKGND: 
			return 0;
		case WM_SIZE:
			if( wParam == SIZE_MINIMIZED )
			{
				m_active = false;
				m_minimized = true;
				m_maximized = false;
				m_begResize = false;
				return 0;
			}

			m_desc.width = LOWORD(lParam);
			m_desc.height = HIWORD(lParam);
			m_isresize = true;
			if( wParam == SIZE_MAXIMIZED )
			{
				m_active = true;
				m_minimized = false;
				m_maximized = true;
			}
			else if( wParam == SIZE_RESTORED )
			{
				if( m_minimized )
				{
					m_active = true;
					m_minimized = false;
				}
				else if( m_maximized )
				{
					m_active = true;
					m_maximized = false;
				}
			}
			resize();
			ForceRedraw();
			m_begResize = false;
			return 0;
		
		case WM_MOUSEMOVE:
			{
				if(!is_tracking_mouse)
				{
					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = m_hwnd;
					if(TrackMouseEvent(&tme))
						is_tracking_mouse = true;
				}

				if(!b_hover)
				{
					b_hover = true;
					SetCursor(crs_arrow);
				}
				mouseMoved = true;
				return 0;
			}
		case WM_MOUSELEAVE:
			b_hover = false;
			mouseMoved = true;
			is_tracking_mouse = false;
			return 0;

		case WM_MOUSEWHEEL: 
			Hud::Get()->MouseWheel(MouseEventWheel((short)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA, mouseX, mouseY), this);
			return 0;
		case WM_KEYDOWN: case WM_KEYUP:
			{
				eKeyCodes keyIndex = static_cast<eKeyCodes>(wParam);
				BYTE lpKeyState[256];
				wchar_t buffer;
				GetKeyboardState(lpKeyState);
				ToUnicode((uint32_t)wParam, HIWORD(lParam)&0xFF, lpKeyState, &buffer, 1, 0);

				Hud::Get()->KeyPressed(KeyEvent(buffer, keyIndex), nMsg == WM_KEYDOWN, this);
				return 0;
			}
		case WM_LBUTTONUP:
			if(is_press_captured)
				ReleaseCapture();
			is_press_captured = false;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_LEFT, mouseX, mouseY), false, this);
			return 0;
		case WM_RBUTTONUP:
			if(is_press_captured)
				ReleaseCapture();
			is_press_captured = false;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_RIGHT, mouseX, mouseY), false, this);
			return 0;
		case WM_MBUTTONUP:
			if(is_press_captured)
				ReleaseCapture();
			is_press_captured = false;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_MIDDLE, mouseX, mouseY), false, this);
			return 0;
		case WM_LBUTTONDOWN:
			SetCapture(m_hwnd);
			is_press_captured = true;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_LEFT, mouseX, mouseY), true, this);
			return 0;
		case WM_RBUTTONDOWN:
			SetCapture(m_hwnd);
			is_press_captured = true;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_RIGHT, mouseX, mouseY), true, this);
			return 0;
		case WM_MBUTTONDOWN:
			SetCapture(m_hwnd);
			is_press_captured = true;
			Hud::Get()->MousePressed(MouseEventClick(MOUSE_MIDDLE, mouseX, mouseY), true, this);
			return 0;

		case WM_NCHITTEST:
            {
				if(!m_desc.noWinBorder)
					break;

				RECT winrect;
                GetWindowRect(hwnd, &winrect);
                int x = LOWORD(lParam);
				if(x > 0x7fff)x = x - 0xffff - 1;
                int y = HIWORD(lParam);
				if(y > 0x7fff)y = y - 0xffff - 1;
				
                //bottom left corner
                if (x >= winrect.left && x < winrect.left + m_desc.borderWidth && y < winrect.bottom && y >= winrect.bottom - m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTBOTTOMLEFT;
				}
                //bottom right corner
                if (x < winrect.right && x >= winrect.right - m_desc.borderWidth && y < winrect.bottom && y >= winrect.bottom - m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTBOTTOMRIGHT;
				}
                //top left corner
                if (x >= winrect.left && x < winrect.left + m_desc.borderWidth && y >= winrect.top && y < winrect.top + m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTTOPLEFT;
				}
                //top right corner
                if (x < winrect.right && x >= winrect.right - m_desc.borderWidth && y >= winrect.top && y < winrect.top + m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTTOPRIGHT;
				}
                //left border
                if (x >= winrect.left && x < winrect.left + m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTLEFT;
				}
                //right border
                if (x < winrect.right && x >= winrect.right - m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTRIGHT;
				}
                //bottom border
                if (y < winrect.bottom && y >= winrect.bottom - m_desc.borderWidth)
				{
					if(m_maximized)return HTMAXBUTTON;
					return HTBOTTOM;
				}
                //top border
				int caption_right = m_desc.width - m_desc.captionRect.right;
				int caption_bottom = m_desc.captionRect.bottom;
				if(m_maximized)
					caption_bottom += SYSTEM_BORDER_SIZE;

                if (y >= winrect.top && y < winrect.top + m_desc.borderWidth)
				{
					if(m_maximized)
					{
						if (y >= winrect.top + m_desc.captionRect.top && y < winrect.top + caption_bottom && x >= winrect.left + m_desc.captionRect.left && x < winrect.left + caption_right )
							return HTCAPTION;
						else
							return HTMAXBUTTON;
					}
					return HTTOP;
				}

				// caption
				if (y >= winrect.top + m_desc.captionRect.top && y < winrect.top + caption_bottom && x >= winrect.left + m_desc.captionRect.left && x < winrect.left + caption_right )
					 return HTCAPTION;

				return HTCLIENT;
			}

		case WM_NCLBUTTONDOWN: case WM_NCLBUTTONDBLCLK:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_LBUTTONDOWN, wParam, lParam);
			break;
		case WM_NCRBUTTONDOWN: case WM_NCRBUTTONDBLCLK:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_RBUTTONDOWN, wParam, lParam);
			break;
		case WM_NCMBUTTONDOWN: case WM_NCMBUTTONDBLCLK:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_MBUTTONDOWN, wParam, lParam);
			break;

		case WM_NCLBUTTONUP:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_LBUTTONUP, wParam, lParam);
			break;
		case WM_NCRBUTTONUP:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_RBUTTONUP, wParam, lParam);
			break;
		case WM_NCMBUTTONUP:
			if(!m_desc.noWinBorder)
				break;
			if(wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTCLOSE || wParam == HTSYSMENU )
				return WndProc( hwnd, WM_MBUTTONUP, wParam, lParam);
			break;

		case WM_INPUT:
			{
				uint32_t dwSize;
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

				LPBYTE lpb = new BYTE[dwSize];
				if(!lpb) 
					return 0;

				if( GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize )
					 WRN("GetRawInputData does not return correct size!"); 

				RAWINPUT* raw = (RAWINPUT*)lpb;
				
				RawInputData rawData;
				switch (raw->header.dwType)
				{
				case RIM_TYPEKEYBOARD:
					rawData.type = USER_DEVICES::KEYBOARD;
					rawData.pressed = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0;
					rawData.key = (uint32_t)raw->data.keyboard.VKey;
					
					WorldMgr::Get()->RawInput(rawData);
					break;

				case RIM_TYPEMOUSE:
					rawData.type = USER_DEVICES::MOUSE;
					rawData.key = (uint32_t)raw->data.mouse.usButtonFlags;
					rawData.deltaX = (int32_t)raw->data.mouse.lLastX;
					rawData.deltaY = (int32_t)raw->data.mouse.lLastY;
					rawData.deltaZ = (SHORT)raw->data.mouse.usButtonData;
					
					WorldMgr::Get()->RawInput(rawData);
					break;

				case RIM_TYPEHID:
					rawData.type = USER_DEVICES::GAMEPAD;
					//LOG("Gamepad input TODO!");
					break;
				} 
				delete[] lpb; 

				return 0;
			} 
		}

		return DefWindowProcW( hwnd, nMsg, wParam, lParam);
	}

	void Window::mouseMoveProcess()
	{
		if(!mouseMoved)
			return;

		POINT Position;
		GetCursorPos(&Position);

		Hud::Get()->SetSystemCursorPos(Position);

		Position.x -= m_desc.posx;
		Position.y -=  m_desc.posy;

		if(mouseX == Position.x && mouseY == Position.y)
			return;

		mouseX = int16_t(Position.x);
		mouseY = int16_t(Position.y);

		Hud::Get()->MouseMove(MouseEvent(mouseX, mouseY, !IsHover()), this);
		
		mouseMoved = false;
	}

	void Window::ForceRedraw()
	{
		if(MainLoop::Get()->Succeeded())
			MainLoop::Get()->Frame(rendertime, true, true);
	}

	bool Window::resize()
	{
		_CLOSE(m_RTmain);

		HRESULT hr = m_pSwapChain->ResizeBuffers(0, m_desc.width, m_desc.height, DXGI_FORMAT_UNKNOWN, 0);
		if(hr != S_OK)
		{
			ERR("Resizing of Swap Chain failed");
			return false;
		}

		// RENDER TARGET
		m_RTmain = new RenderTarget();
		if(!m_RTmain->Init(m_desc.width, m_desc.height))
		{
			ERR("Creation of main RenderTarget failed");
			return false;
		}
		m_ortho = XMMatrixOrthographicLH(float(m_desc.width), float(m_desc.height), 0.0f, 1.0f);

		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
		if( FAILED(hr) )
			return false;
		if(!m_RTmain->AddBackBufferRT(pBackBuffer))
		{
			ERR("AddBackBufferRT failed");
			return false;
		}		
		_RELEASE(pBackBuffer);

		UpdateWindowState();

		return SUCCEEDED(hr);
	}

	void Window::UpdateWindowState()
	{
		Hud::Get()->UpdateEntities(this);
	}
	
	LRESULT CALLBACK EngineCore::StaticWndProc(HWND hwnd, uint32_t nMsg, WPARAM wParam, LPARAM lParam)
	{
		if(WindowsMgr::Get()->IsJustCreated(hwnd) && nMsg != WM_NCCREATE)
			return 0;

		Window* win = nullptr;
		if(!(win = WindowsMgr::Get()->GetWindowByHwnd(hwnd)))
			return 0;

		// remove
		Render::Get()->CurrentHudWindow = win;

		return win->WndProc( hwnd, nMsg, wParam, lParam );
	}
//------------------------------------------------------------------

	DropTarget::DropTarget(void* win)
	{
		refCount = 1;
		window = win;
	}

	DropTarget::~DropTarget()
	{
	}

	HRESULT DropTarget::QueryInterface(const IID& riid, void** ppvObject)
	{
		if (riid == IID_IUnknown ||
			riid == IID_IDropTarget)
		{
			*ppvObject = this;
			AddRef();
			return NOERROR;
		}
		*ppvObject = nullptr;
		return ResultFromScode(E_NOINTERFACE);
	}

	ULONG DropTarget::AddRef()
	{
		return ++refCount;
	}

	ULONG DropTarget::Release()
	{
		if (--refCount == 0)
		{
			delete this;
			return 0;
		}
		return refCount;
	}

	HRESULT DropTarget::DragEnter(IDataObject* dataObject, DWORD keyState, POINTL mousePos, DWORD* effect)
	{			
		Hud::Get()->ClearDropItems();

		FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgmed;

		if(dataObject->QueryGetData(&fmtetc) == S_OK)
		{
			if(dataObject->GetData(&fmtetc, &stgmed) == S_OK)
			{
				HDROP hDrop = (HDROP)GlobalLock(stgmed.hGlobal);

				uint32_t filesCount = DragQueryFile(hDrop, 0xffffffff, NULL, NULL);

				for(uint16_t i = 0; i < filesCount; i++)
				{
					TCHAR lpszFile[MAX_PATH] = {0};
					lpszFile[0] = '\0';

					if (DragQueryFile(hDrop, i, lpszFile, MAX_PATH))
					{
						string filename = WstringToString(wstring(lpszFile));
						Hud::Get()->AddDropItems(filename);
					}
				}

				GlobalUnlock(stgmed.hGlobal);
				ReleaseStgMedium(&stgmed);
			}
		}

		
		POINT dropPoint;
		dropPoint.x = mousePos.x - ((Window*)window)->m_desc.posx;
		dropPoint.y = mousePos.y - ((Window*)window)->m_desc.posy;

		bool allow = Hud::Get()->DragDropItems(GuiEvents::GE_ITEMS_DRAG_ENTER, dropPoint, (Window*)window);
		
		*effect = allow ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
		return NOERROR;
	}
	 
	HRESULT DropTarget::DragOver(DWORD keyState, POINTL mousePos, DWORD* effect)
	{
		POINT dropPoint;
		dropPoint.x = mousePos.x - ((Window*)window)->m_desc.posx;
		dropPoint.y = mousePos.y - ((Window*)window)->m_desc.posy;

		bool allow = Hud::Get()->DragDropItems(GuiEvents::GE_ITEMS_DRAG_MOVE, dropPoint, (Window*)window);
		
		*effect = allow ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
		return NOERROR;
	}
 
	HRESULT DropTarget::DragLeave()
	{
		POINT dropPoint;
		dropPoint.x = dropPoint.y = 0;

		bool allow = Hud::Get()->DragDropItems(GuiEvents::GE_ITEMS_DRAG_LEAVE, dropPoint, (Window*)window);
		Hud::Get()->ClearDropItems();
		
		return NOERROR;
	}
 
	HRESULT DropTarget::Drop(IDataObject* dataObject, DWORD keyState, POINTL mousePos, DWORD* effect)
	{
		POINT dropPoint;
		dropPoint.x = mousePos.x - ((Window*)window)->m_desc.posx;
		dropPoint.y = mousePos.y - ((Window*)window)->m_desc.posy;

		bool allow = Hud::Get()->FinishDropItems(dropPoint, (Window*)window);

		*effect = allow ? DROPEFFECT_MOVE : DROPEFFECT_NONE;
		return NOERROR;
	}

}