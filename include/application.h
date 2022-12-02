#ifndef APPLICATION_H
#define APPLICATION_H

#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>

#define KEYBOARD_BUTTON_COUNT 1024
#define MOUSE_BUTTON_COUNT 32

class Application
{
public:
    template <class TApp>
    static int Run(TApp &t);
};

enum KeyboardButtons
{
    KeySpace = 32,
    KeyApostrophe = 39, /* ' */
    KeyComma = 44,      /* , */
    KeyMinus = 45,      /* - */
    KeyPeriod = 46,     /* . */
    KeySlash = 47,      /* / */
    Key0 = 48,
    Key1 = 49,
    Key2 = 50,
    Key3 = 51,
    Key4 = 52,
    Key5 = 53,
    Key6 = 54,
    Key7 = 55,
    Key8 = 56,
    Key9 = 57,
    KeySemicolon = 59, /* ; */
    KeyEqual = 61,     /* = */
    KeyA = 65,
    KeyB = 66,
    KeyC = 67,
    KeyD = 68,
    KeyE = 69,
    KeyF = 70,
    KeyG = 71,
    KeyH = 72,
    KeyI = 73,
    KeyJ = 74,
    KeyK = 75,
    KeyL = 76,
    KeyM = 77,
    KeyN = 78,
    KeyO = 79,
    KeyP = 80,
    KeyQ = 81,
    KeyR = 82,
    KeyS = 83,
    KeyT = 84,
    KeyU = 85,
    KeyV = 86,
    KeyW = 87,
    KeyX = 88,
    KeyY = 89,
    KeyZ = 90,
    KeyLeftBracket = 91,  /* [ */
    KeyBackslash = 92,    /* \ */
    KeyRightBracket = 93, /* ] */
    KeyGraveAccent = 96,  /* ` */
    KeyWorld1 = 161,      /*non-us #1 */
    KeyWorld2 = 162,      /*non-us #2 */
    KeyEscape = 256,
    KeyEnter = 257,
    KeyTab = 258,
    KeyBackspace = 259,
    KeyInsert = 260,
    KeyDelete = 261,
    KeyRight = 262,
    KeyLeft = 263,
    KeyDown = 264,
    KeyUp = 265,
    KeyPageUp = 266,
    KeyPageDown = 267,
    KeyHome = 268,
    KeyEnd = 269,
    KeyCapsLock = 280,
    KeyScrollLock = 281,
    KeyNumLock = 282,
    KeyPrintScreen = 283,
    KeyPause = 284,
    KeyF1 = 290,
    KeyF2 = 291,
    KeyF3 = 292,
    KeyF4 = 293,
    KeyF5 = 294,
    KeyF6 = 295,
    KeyF7 = 296,
    KeyF8 = 297,
    KeyF9 = 298,
    KeyF10 = 299,
    KeyF11 = 300,
    KeyF12 = 301,
    KeyF13 = 302,
    KeyF14 = 303,
    KeyF15 = 304,
    KeyF16 = 305,
    KeyF17 = 306,
    KeyF18 = 307,
    KeyF19 = 308,
    KeyF20 = 309,
    KeyF21 = 310,
    KeyF22 = 311,
    KeyF23 = 312,
    KeyF24 = 313,
    KeyF25 = 314,
    KeyKp0 = 320,
    KeyKp1 = 321,
    KeyKp2 = 322,
    KeyKp3 = 323,
    KeyKp4 = 324,
    KeyKp5 = 325,
    KeyKp6 = 326,
    KeyKp7 = 327,
    KeyKp8 = 328,
    KeyKp9 = 329,
    KeyKpDecimal = 330,
    KeyKpDivide = 331,
    KeyKpMultiply = 332,
    KeyKpSubtract = 333,
    KeyKpAdd = 334,
    KeyKpEnter = 335,
    KeyKpEqual = 336,
    KeyLeftShift = 340,
    KeyLeftControl = 341,
    KeyLeftAlt = 342,
    KeyLeftSuper = 343,
    KeyRightShift = 344,
    KeyRightControl = 345,
    KeyRightAlt = 346,
    KeyRightSuper = 347,
    KeyMenu = 348,
    KeyboardButtonsCount,
};

enum MouseButtons
{
    LeftButton,
    RightButton,
    MiddleButton,
    MouseButtonsCount,
};

struct InputState
{
    bool KeyboardButtonStates[KeyboardButtons::KeyboardButtonsCount] = {false};
    bool MouseButtonStates[MouseButtons::MouseButtonsCount] = {false};
    int MousePointerPosition[2] = {0, 0};
    InputState *PreviousState = nullptr;

    void OnKeyboardButtonDown(
        KeyboardButtons button,
        std::function<void()> func);

    void OnKeyboardButtonUp(
        KeyboardButtons button,
        std::function<void()> func);

    void OnMouseButtonDown(
        MouseButtons button,
        std::function<void()> func);

    void OnMouseButtonUp(
        MouseButtons button,
        std::function<void()> func);
};

#endif // APPLICATION_H

#ifdef APPLICATION_IMPLEMENTATION

#include <glad/glad.h>

#ifdef _WIN32

#include <GL/wglext.h>
#include <windows.h>

#define EXAMPLE_NAME "genmap"

typedef HGLRC(WINAPI *PFNGLXCREATECONTEXTATTRIBS)(HDC hDC, HGLRC hShareContext, const int *attribList);

class Win32Application
{
public:
    bool Startup(
        std::function<bool()> intialize,
        std::function<void(int width, int height)> resize,
        std::function<void()> destroy);

    int Run(
        std::function<bool(std::chrono::milliseconds::rep time, const struct InputState &inputState)> tick);

private:
    std::function<void(int width, int height)> _resize;
    std::function<void()> _destroy;
    HINSTANCE _hInstance;
    HWND _hWnd;
    HDC _hDC;
    HGLRC _hRC;
    PFNGLXCREATECONTEXTATTRIBS _pfnGlxCreateContext;
    InputState _inputState;
    InputState _previousInputState;

    virtual void Destroy(
        const char *errorMessage = nullptr);

    LRESULT CALLBACK objectProc(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

    static LRESULT CALLBACK staticProc(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);
};

bool Win32Application::Startup(
    std::function<bool()> intialize,
    std::function<void(int width, int height)> resize,
    std::function<void()> destroy)
{
    _resize = resize;
    _destroy = destroy;
    _hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc;

    if (GetClassInfo(_hInstance, EXAMPLE_NAME, &wc) == FALSE)
    {
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = (WNDPROC)Win32Application::staticProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = _hInstance;
        wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = EXAMPLE_NAME;

        if (RegisterClass(&wc) == FALSE)
        {
            Destroy("Failed to register window class");
            return false;
        }
    }

    _hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        EXAMPLE_NAME, EXAMPLE_NAME,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr,
        _hInstance, (VOID *)this);

    if (_hWnd == 0)
    {
        Destroy("Failed to create window");
        return false;
    }

    _hDC = GetDC(_hWnd);

    if (_hDC == 0)
    {
        Destroy("Failed to get device context");
        return false;
    }

    static PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
            1,                             // Version Number
            PFD_DRAW_TO_WINDOW |           // Format Must Support Window
                PFD_SUPPORT_OPENGL |       // Format Must Support CodeGL
                PFD_DOUBLEBUFFER,          // Must Support Double Buffering
            PFD_TYPE_RGBA,                 // Request An RGBA Format
            32,                            // Select Our Color Depth
            0, 0, 0, 0, 0, 0,              // Color Bits Ignored
            0,                             // No Alpha Buffer
            0,                             // Shift Bit Ignored
            0,                             // No Accumulation Buffer
            0, 0, 0, 0,                    // Accumulation Bits Ignored
            24,                            // 24Bit Z-Buffer (Depth Buffer)
            0,                             // No Stencil Buffer
            0,                             // No Auxiliary Buffer
            PFD_MAIN_PLANE,                // Main Drawing Layer
            0,                             // Reserved
            0, 0, 0                        // Layer Masks Ignored
        };

    auto pixelFormat = ChoosePixelFormat(_hDC, &pfd);
    if (pixelFormat == false)
    {
        Destroy("Failed to choose pixel format");
        return false;
    }

    if (SetPixelFormat(_hDC, pixelFormat, &pfd) == FALSE)
    {
        Destroy("Failed to set pixel format");
        return false;
    }

    _pfnGlxCreateContext = (PFNGLXCREATECONTEXTATTRIBS)wglGetProcAddress("wglCreateContextAttribsARB");
    if (_pfnGlxCreateContext == nullptr)
    {
        _hRC = wglCreateContext(_hDC);

        if (_hRC == 0)
        {
            Destroy("Failed to create clasic opengl context (v1.0)");
            return false;
        }
    }
    else
    {
        GLint attribList[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB,
                4,
                WGL_CONTEXT_MINOR_VERSION_ARB,
                6,
                WGL_CONTEXT_FLAGS_ARB,
                0,
                0,
            };

        _hRC = _pfnGlxCreateContext(_hDC, 0, attribList);
        if (_hRC == 0)
        {
            Destroy("Failed to create modern opengl context (v4.6)");
            return false;
        }
    }

    wglMakeCurrent(_hDC, _hRC);

    gladLoadGL();

    spdlog::debug("GL_VERSION                  : {0}", glGetString(GL_VERSION));
    spdlog::debug("GL_SHADING_LANGUAGE_VERSION : {0}", glGetString(GL_SHADING_LANGUAGE_VERSION));
    spdlog::debug("GL_RENDERER                 : {0}", glGetString(GL_RENDERER));
    spdlog::debug("GL_VENDOR                   : {0}", glGetString(GL_VENDOR));

    if (!intialize())
    {
        Destroy("Initialize failed");
        return false;
    }

    ShowWindow(_hWnd, SW_SHOW);
    SetForegroundWindow(_hWnd);
    SetFocus(_hWnd);

    return true;
}
std::chrono::milliseconds::rep CurrentTime()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();

    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

int Win32Application::Run(
    std::function<bool(std::chrono::milliseconds::rep time, const struct InputState &inputState)> tick)
{
    bool running = true;

    _inputState.PreviousState = &_previousInputState;

    while (running)
    {
        memcpy(&_previousInputState, &_inputState, sizeof(InputState));

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (IsWindowVisible(_hWnd) == FALSE)
        {
            break;
        }

        running = tick(CurrentTime(), _inputState);

        SwapBuffers(_hDC);
    }

    _destroy();

    return 0;
}

void Win32Application::Destroy(
    const char *errorMessage)
{
    if (errorMessage != nullptr)
    {
        spdlog::error(errorMessage);
    }

    _destroy();

    UnregisterClass(EXAMPLE_NAME, _hInstance);

    if (_hDC != 0)
    {
        ReleaseDC(_hWnd, _hDC);
        _hDC = 0;
    }

    if (_hWnd != 0)
    {
        DestroyWindow(_hWnd);
        _hWnd = 0;
    }
}

LRESULT CALLBACK Win32Application::objectProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);

            this->_resize(width, height);

            break;
        }
        case WM_LBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::LeftButton] = true;
            break;
        }
        case WM_LBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::LeftButton] = false;
            break;
        }
        case WM_RBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::RightButton] = true;
            break;
        }
        case WM_RBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::RightButton] = false;
            break;
        }
        case WM_MBUTTONDOWN:
        {
            _inputState.MouseButtonStates[MouseButtons::MiddleButton] = true;
            break;
        }
        case WM_MBUTTONUP:
        {
            _inputState.MouseButtonStates[MouseButtons::MiddleButton] = false;
            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            auto scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
            if (!scancode)
            {
                // NOTE: Some synthetic key messages have a scancode of zero
                // HACK: Map the virtual key back to a usable scancode
                scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
            }

            switch (scancode)
            {
                case 0x00B:
                    _inputState.KeyboardButtonStates[Key0] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x002:
                    _inputState.KeyboardButtonStates[Key1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x003:
                    _inputState.KeyboardButtonStates[Key2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x004:
                    _inputState.KeyboardButtonStates[Key3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x005:
                    _inputState.KeyboardButtonStates[Key4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x006:
                    _inputState.KeyboardButtonStates[Key5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x007:
                    _inputState.KeyboardButtonStates[Key6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x008:
                    _inputState.KeyboardButtonStates[Key7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x009:
                    _inputState.KeyboardButtonStates[Key8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00A:
                    _inputState.KeyboardButtonStates[Key9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01E:
                    _inputState.KeyboardButtonStates[KeyA] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x030:
                    _inputState.KeyboardButtonStates[KeyB] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02E:
                    _inputState.KeyboardButtonStates[KeyC] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x020:
                    _inputState.KeyboardButtonStates[KeyD] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x012:
                    _inputState.KeyboardButtonStates[KeyE] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x021:
                    _inputState.KeyboardButtonStates[KeyF] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x022:
                    _inputState.KeyboardButtonStates[KeyG] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x023:
                    _inputState.KeyboardButtonStates[KeyH] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x017:
                    _inputState.KeyboardButtonStates[KeyI] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x024:
                    _inputState.KeyboardButtonStates[KeyJ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x025:
                    _inputState.KeyboardButtonStates[KeyK] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x026:
                    _inputState.KeyboardButtonStates[KeyL] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x032:
                    _inputState.KeyboardButtonStates[KeyM] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x031:
                    _inputState.KeyboardButtonStates[KeyN] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x018:
                    _inputState.KeyboardButtonStates[KeyO] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x019:
                    _inputState.KeyboardButtonStates[KeyP] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x010:
                    _inputState.KeyboardButtonStates[KeyQ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x013:
                    _inputState.KeyboardButtonStates[KeyR] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01F:
                    _inputState.KeyboardButtonStates[KeyS] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x014:
                    _inputState.KeyboardButtonStates[KeyT] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x016:
                    _inputState.KeyboardButtonStates[KeyU] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02F:
                    _inputState.KeyboardButtonStates[KeyV] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x011:
                    _inputState.KeyboardButtonStates[KeyW] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02D:
                    _inputState.KeyboardButtonStates[KeyX] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x015:
                    _inputState.KeyboardButtonStates[KeyY] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02C:
                    _inputState.KeyboardButtonStates[KeyZ] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x028:
                    _inputState.KeyboardButtonStates[KeyApostrophe] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02B:
                    _inputState.KeyboardButtonStates[KeyBackslash] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x033:
                    _inputState.KeyboardButtonStates[KeyComma] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00D:
                    _inputState.KeyboardButtonStates[KeyEqual] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x029:
                    _inputState.KeyboardButtonStates[KeyGraveAccent] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01A:
                    _inputState.KeyboardButtonStates[KeyLeftBracket] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00C:
                    _inputState.KeyboardButtonStates[KeyMinus] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x034:
                    _inputState.KeyboardButtonStates[KeyPeriod] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01B:
                    _inputState.KeyboardButtonStates[KeyRightBracket] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x027:
                    _inputState.KeyboardButtonStates[KeySemicolon] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x035:
                    _inputState.KeyboardButtonStates[KeySlash] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x056:
                    _inputState.KeyboardButtonStates[KeyWorld2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x00E:
                    _inputState.KeyboardButtonStates[KeyBackspace] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x153:
                    _inputState.KeyboardButtonStates[KeyDelete] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14F:
                    _inputState.KeyboardButtonStates[KeyEnd] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01C:
                    _inputState.KeyboardButtonStates[KeyEnter] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x001:
                    _inputState.KeyboardButtonStates[KeyEscape] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x147:
                    _inputState.KeyboardButtonStates[KeyHome] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x152:
                    _inputState.KeyboardButtonStates[KeyInsert] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15D:
                    _inputState.KeyboardButtonStates[KeyMenu] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x151:
                    _inputState.KeyboardButtonStates[KeyPageDown] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x149:
                    _inputState.KeyboardButtonStates[KeyPageUp] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x045:
                    _inputState.KeyboardButtonStates[KeyPause] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x146:
                    _inputState.KeyboardButtonStates[KeyPause] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x039:
                    _inputState.KeyboardButtonStates[KeySpace] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x00F:
                    _inputState.KeyboardButtonStates[KeyTab] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03A:
                    _inputState.KeyboardButtonStates[KeyCapsLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x145:
                    _inputState.KeyboardButtonStates[KeyNumLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x046:
                    _inputState.KeyboardButtonStates[KeyScrollLock] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03B:
                    _inputState.KeyboardButtonStates[KeyF1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03C:
                    _inputState.KeyboardButtonStates[KeyF2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03D:
                    _inputState.KeyboardButtonStates[KeyF3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03E:
                    _inputState.KeyboardButtonStates[KeyF4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x03F:
                    _inputState.KeyboardButtonStates[KeyF5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x040:
                    _inputState.KeyboardButtonStates[KeyF6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x041:
                    _inputState.KeyboardButtonStates[KeyF7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x042:
                    _inputState.KeyboardButtonStates[KeyF8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x043:
                    _inputState.KeyboardButtonStates[KeyF9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x044:
                    _inputState.KeyboardButtonStates[KeyF10] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x057:
                    _inputState.KeyboardButtonStates[KeyF11] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x058:
                    _inputState.KeyboardButtonStates[KeyF12] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x064:
                    _inputState.KeyboardButtonStates[KeyF13] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x065:
                    _inputState.KeyboardButtonStates[KeyF14] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x066:
                    _inputState.KeyboardButtonStates[KeyF15] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x067:
                    _inputState.KeyboardButtonStates[KeyF16] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x068:
                    _inputState.KeyboardButtonStates[KeyF17] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x069:
                    _inputState.KeyboardButtonStates[KeyF18] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06A:
                    _inputState.KeyboardButtonStates[KeyF19] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06B:
                    _inputState.KeyboardButtonStates[KeyF20] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06C:
                    _inputState.KeyboardButtonStates[KeyF21] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06D:
                    _inputState.KeyboardButtonStates[KeyF22] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x06E:
                    _inputState.KeyboardButtonStates[KeyF23] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x076:
                    _inputState.KeyboardButtonStates[KeyF24] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x038:
                    _inputState.KeyboardButtonStates[KeyLeftAlt] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x01D:
                    _inputState.KeyboardButtonStates[KeyLeftControl] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x02A:
                    _inputState.KeyboardButtonStates[KeyLeftShift] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15B:
                    _inputState.KeyboardButtonStates[KeyLeftSuper] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x137:
                    _inputState.KeyboardButtonStates[KeyPrintScreen] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x138:
                    _inputState.KeyboardButtonStates[KeyRightAlt] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x11D:
                    _inputState.KeyboardButtonStates[KeyRightControl] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x036:
                    _inputState.KeyboardButtonStates[KeyRightShift] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x15C:
                    _inputState.KeyboardButtonStates[KeyRightSuper] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x150:
                    _inputState.KeyboardButtonStates[KeyDown] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14B:
                    _inputState.KeyboardButtonStates[KeyLeft] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x14D:
                    _inputState.KeyboardButtonStates[KeyRight] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x148:
                    _inputState.KeyboardButtonStates[KeyUp] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;

                case 0x052:
                    _inputState.KeyboardButtonStates[KeyKp0] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04F:
                    _inputState.KeyboardButtonStates[KeyKp1] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x050:
                    _inputState.KeyboardButtonStates[KeyKp2] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x051:
                    _inputState.KeyboardButtonStates[KeyKp3] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04B:
                    _inputState.KeyboardButtonStates[KeyKp4] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04C:
                    _inputState.KeyboardButtonStates[KeyKp5] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04D:
                    _inputState.KeyboardButtonStates[KeyKp6] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x047:
                    _inputState.KeyboardButtonStates[KeyKp7] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x048:
                    _inputState.KeyboardButtonStates[KeyKp8] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x049:
                    _inputState.KeyboardButtonStates[KeyKp9] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04E:
                    _inputState.KeyboardButtonStates[KeyKpAdd] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x053:
                    _inputState.KeyboardButtonStates[KeyKpDecimal] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x135:
                    _inputState.KeyboardButtonStates[KeyKpDivide] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x11C:
                    _inputState.KeyboardButtonStates[KeyKpEnter] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x059:
                    _inputState.KeyboardButtonStates[KeyKpEqual] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x037:
                    _inputState.KeyboardButtonStates[KeyKpMultiply] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
                case 0x04A:
                    _inputState.KeyboardButtonStates[KeyKpSubtract] = (HIWORD(lParam) & KF_UP ? false : true);
                    break;
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            _inputState.MousePointerPosition[0] = LOWORD(lParam);
            _inputState.MousePointerPosition[1] = HIWORD(lParam);
            break;
        }
    }
    return DefWindowProc(this->_hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Win32Application::staticProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    Win32Application *app = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        app = reinterpret_cast<Win32Application *>(((LPCREATESTRUCT)lParam)->lpCreateParams);

        if (app != nullptr)
        {
            app->_hWnd = hWnd;

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<long long>(app));

            return app->objectProc(uMsg, wParam, lParam);
        }
    }
    else
    {
        app = reinterpret_cast<Win32Application *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        if (app != nullptr)
        {
            return app->objectProc(uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Win32Application *CreateApplication(
    std::function<bool()> initialize,
    std::function<void(int width, int height)> resize,
    std::function<void()> destroy)
{
    static Win32Application app;

    if (app.Startup(initialize, resize, destroy))
    {
        return &app;
    }

    spdlog::error("Create application failed");

    exit(0);

    return nullptr;
}
#endif // _WIN32

#ifdef __linux__
#endif // __linux__

void InputState::OnKeyboardButtonDown(
    KeyboardButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (KeyboardButtonStates[button] && KeyboardButtonStates[button] != PreviousState->KeyboardButtonStates[button])
    {
        func();
    }
}

void InputState::OnKeyboardButtonUp(
    KeyboardButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (!KeyboardButtonStates[button] && KeyboardButtonStates[button] != PreviousState->KeyboardButtonStates[button])
    {
        func();
    }
}

void InputState::OnMouseButtonDown(
    MouseButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (MouseButtonStates[button] && MouseButtonStates[button] != PreviousState->MouseButtonStates[button])
    {
        func();
    }
}

void InputState::OnMouseButtonUp(
    MouseButtons button,
    std::function<void()> func)
{
    if (PreviousState == nullptr)
    {
        return;
    }

    if (!MouseButtonStates[button] && MouseButtonStates[button] != PreviousState->MouseButtonStates[button])
    {
        func();
    }
}

template <class TApp>
int Application::Run(TApp &t)
{
    auto app = CreateApplication(
        [&]() { return t.Startup(); },
        [&](int w, int h) { t.Resize(w, h); },
        [&]() { t.Destroy(); });

    return app->Run([&](std::chrono::milliseconds::rep time, const struct InputState &inputState) { return t.Tick(time, inputState); });
}

#endif //APPLICATION_IMPLEMENTATION
