#include "Precompiled.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Suora/Core/NativeInput.h"
#include "Suora/Assets/SuoraProject.h"
#include "Suora/Assets/Texture2D.h"
#include "Suora/Events/ApplicationEvent.h"
#include "Suora/Events/MouseEvent.h"
#include "Suora/Events/KeyEvent.h"

#include "Suora/Renderer/RendererAPI.h"
#include "Suora/Renderer/Texture.h"

#include "Platform/OpenGL/OpenGLContext.h"

#include "Windows.h"
#include <stb_image.h>

#ifdef WIN32
	#include <dwmapi.h>
#endif

#define WINDOW_RESIZING_BORDER_WIDTH 8.0
#define WINDOW_DRAG_HEIGHT 36.0
#define WINDOW_MINIMUM_WIDTH 600.0
#define WINDOW_MINIMUM_HEIGHT 270.0


namespace GLFW
{
	void CenterWindow(GLFWwindow* window)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glfwSetWindowPos(window, mode->width/2 - width/2, mode->height/2 - height/2);
	}
}

namespace Suora 
{
	
	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		SUORA_ERROR(LogCategory::Rendering, "GLFW Error ({0}): {1}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
		: m_Props(props)
	{
		SUORA_PROFILE_FUNCTION();

		Window::CurrentFocusedWindow = this;
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		SUORA_PROFILE_FUNCTION();

		Window::AllWindows.Remove(this);
		Shutdown();
	}

	bool WindowsWindow::IsUndecorated() const
	{
		return !m_Props.isDecorated;
	}

	void WindowsWindow::Maximize()
	{
		int maximized = glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
		if (maximized == 1)
		{
			glfwRestoreWindow(m_Window);
		}
		else
		{
			glfwMaximizeWindow(m_Window);
		}
	}

	void WindowsWindow::Iconify()
	{
		glfwIconifyWindow(m_Window);
	}

	void WindowsWindow::CenterWindow()
	{
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		int width, height;
		glfwGetWindowSize(m_Window, &width, &height);
		glfwSetWindowPos(m_Window, mode->width / 2.0f - width / 2.0f, mode->height / 2.0f - height / 2.0f);
	}

	void WindowsWindow::SetFullscreen(bool b)
	{
		if (IsFullscreen() == b)
			return;

		static int _wndPos[2];
		static int _wndSize[2];

		if (b)
		{
			// backup window position and window size
			glfwGetWindowPos(m_Window, &_wndPos[0], &_wndPos[1]);
			glfwGetWindowSize(m_Window, &_wndSize[0], &_wndSize[1]);

			// get resolution of monitor
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

			// switch to full screen
			glfwWindowHint(GLFW_DECORATED, 1);
			glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
		}
		else
		{
			// restore last window size and position
			glfwWindowHint(GLFW_DECORATED, m_Props.isDecorated ? 1 : 0);
			glfwSetWindowMonitor(m_Window, nullptr, _wndPos[0], _wndPos[1], _wndSize[0], _wndSize[1], 0);
		}
	}

	bool WindowsWindow::IsFullscreen()
	{
		return glfwGetWindowMonitor(m_Window) != nullptr;
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		SUORA_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.window = this;

		SUORA_INFO(LogCategory::Rendering, "Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			SUORA_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			SUORA_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{
			SUORA_PROFILE_SCOPE("glfwCreateWindow");
		#if defined(SUORA_DEBUG)
			if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
		#endif
			glfwWindowHint(GLFW_DECORATED, m_Props.isDecorated ? 1 : 0);
			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, (Window::AllWindows.Size() > 0) ? (GLFWwindow*) Window::AllWindows[0]->GetNativeWindow() : nullptr);
			Window::AllWindows.Add(this);
			GLFW::CenterWindow(m_Window);
			++s_GLFWWindowCount;

		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* window, int focused)
		{
			if (focused)
			{
				// Iterate over all Windows until we find the focused one; // TODO: Remove this hack
				for (Window* w : Window::AllWindows.GetData())
				{
					if (static_cast<GLFWwindow*>(w->GetNativeWindow()) == window)
					{
						Window::CurrentFocusedWindow = w;
						break;
					}
				}
			}
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);

			NativeInput::s_CharInputCallback((char)(uint16_t)(KeyCode)keycode);

		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			NativeInput::s_GlobalScrollDelta += (float)yOffset;
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

		SetCursor(Cursor::Default);
		SetVSync(false);

	}

	void WindowsWindow::Shutdown()
	{
		SUORA_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		m_Window = nullptr;
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		SUORA_PROFILE_SCOPE("WindowsWindow::OnUpdate()");
		WindowData& data = *(WindowData*)glfwGetWindowUserPointer(m_Window);
		SUORA_PROFILE_FUNCTION();

		{
			SUORA_PROFILE_SCOPE("Events");
			glfwPollEvents();
		}
		{
			SUORA_PROFILE_SCOPE("Swap");
			m_Context->SwapBuffers();
		}

		if (IsUndecorated())
		{
			SUORA_PROFILE_SCOPE("WindowsWindow::OnUpdate() - IsUndecorated()");
			int xWin, yWin;
			glfwGetWindowPos(m_Window, &xWin, &yWin);
			double xPos, yPos;
			glfwGetCursorPos(m_Window, &xPos, & yPos);

			xPos += xWin;
			yPos += yWin;

			/*data.deltaX = xPos - data.lastX;
			data.deltaY = yPos - data.lastY;

			data.lastX = xPos;
			data.lastY = yPos;*/
			
		}


		// Updating the WindowIcon
		if (ProjectSettings::Get() && ProjectSettings::Get()->m_ProjectIconTexture != m_CurrentIconTexture)
		{
			if (ProjectSettings::Get()->m_ProjectIconTexture)
			{
				Texture* texture = ProjectSettings::Get()->m_ProjectIconTexture->GetTexture();
				if (texture && texture != Texture::GetOrCreateDefaultTexture())
				{
					m_CurrentIconTexture = ProjectSettings::Get()->m_ProjectIconTexture;
					m_CurrentIconTexture->m_Path = ProjectSettings::Get()->m_ProjectIconTexture->m_Path;
					GLFWimage images[1];
					int channels = 4;
					stbi_set_flip_vertically_on_load(0);
					images[0].pixels = stbi_load(m_CurrentIconTexture->GetTexturePath().c_str(), &images[0].width, &images[0].height, &channels, channels); //rgba channels 
					stbi_set_flip_vertically_on_load(1);
					images->width = texture->GetWidth();
					images->height = texture->GetHeight();
					glfwSetWindowIcon(m_Window, 1, images);
					stbi_image_free(images[0].pixels);
				}
			}
			else
			{
				glfwSetWindowIcon(m_Window, 0, nullptr);
			}
		}
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		SUORA_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_Window);

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	bool WindowsWindow::IsMaximized() const
	{
		int maximized = glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED);
		return maximized == 1;
	}

	void WindowsWindow::SetCursor(Cursor cursor)
	{
		if (cursor == currentCursorType) return;
		currentCursorType = cursor;

		if(m_Cursor) glfwDestroyCursor(m_Cursor);

		switch (cursor)
		{
		case Cursor::Default: m_Cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); break;
		case Cursor::Hand: m_Cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR); break;
		case Cursor::Crosshair: m_Cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR); break;
		case Cursor::IBeam: m_Cursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR); break;
		case Cursor::HorizontalResize: m_Cursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR); break;
		case Cursor::VerticalResize: m_Cursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR); break;

		default: m_Cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR); break;
		}

		glfwSetCursor(m_Window, m_Cursor);
	}
	Cursor WindowsWindow::GetCursor()
	{
		return currentCursorType;
	}

	void WindowsWindow::SetCursorLocked(bool locked)
	{
		NativeInput::ResetMouseDelta();
		glfwSetInputMode(m_Window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	bool WindowsWindow::IsWindowResizing()
	{
		return false;
	}

}