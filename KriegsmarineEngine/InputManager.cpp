#include "InputManager.h"
#include "hidsdi.h"

void InputManager::Initialize(HWND hwnd)
{
    _hWnd = hwnd;
    RegisterRawInputDevices();
}

HWND InputManager::GetHwnd() const
{
    return _hWnd;
}

void InputManager::Update()
{
    std::copy(std::begin(_keyState), std::end(_keyState), std::begin(_prevKeyState));
    std::copy(std::begin(_mouseState), std::end(_mouseState), std::begin(_prevMouseState));

    // _mouseDelta = { 0, 0 };
}

bool InputManager::IsKeyDown(unsigned int key) const
{
    return key < KEY_COUNT && _keyState[key];
}

bool InputManager::IsKeyPressed(unsigned int key) const
{
    return key < KEY_COUNT && _keyState[key] && !_prevKeyState[key];
}

bool InputManager::IsKeyReleased(unsigned int key) const
{
    return key < KEY_COUNT && !_keyState[key] && _prevKeyState[key];
}

bool InputManager::IsMouseButtonDown(MouseKey button) const
{
    int buttonCasted = static_cast<int>(button);

    return buttonCasted < MOUSE_BUTTON_COUNT && _mouseState[buttonCasted];
}

bool InputManager::IsMouseButtonPressed(MouseKey button) const
{
    int buttonCasted = static_cast<int>(button);

    return buttonCasted < MOUSE_BUTTON_COUNT &&
        _mouseState[buttonCasted] && !_prevMouseState[buttonCasted];
}

bool InputManager::IsMouseButtonReleased(MouseKey button) const
{
    int buttonCasted = static_cast<int>(button);

    return buttonCasted < MOUSE_BUTTON_COUNT &&
        !_mouseState[buttonCasted] && _prevMouseState[buttonCasted];
}

void InputManager::SetMousePos(POINT pos)
{
    _mousePos = pos;
}

POINT InputManager::GetMousePos() const
{
    return _mousePos;
}

POINT InputManager::GetMouseDelta() const
{
    return _mouseDelta;
}

void InputManager::SetCursorPos(int x, int y)
{
    ::SetCursorPos(x, y);
}

void InputManager::GetCursorPos(LPPOINT lpPoint)
{
    ::GetCursorPos(lpPoint);
}

void InputManager::HideCursor()
{
    if (!_isCursorHidden)
    {
        while (::ShowCursor(FALSE) >= 0);  // Keep hiding cursor until it's fully hidden
        _isCursorHidden = true;
    }
}

void InputManager::ShowCursor()
{
    if (_isCursorHidden)
    {
        while (::ShowCursor(TRUE) < 0);  // Keep showing cursor until it's fully shown
        _isCursorHidden = false;
    }
}

void InputManager::ResetMouseDelta()
{
    _mouseDelta = { 0, 0 };
    _mouseWheelDelta = 0;
}

short InputManager::GetMouseWheelDelta() const
{
    return _mouseWheelDelta;
}

void InputManager::ProcessRawInput(LPARAM lParam)
{
    UINT dwSize = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

    // std::vector<BYTE> 사용
    std::vector<BYTE> lpb(dwSize);

    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
    {
        return;
    }

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb.data());

    if (raw->header.dwType == RIM_TYPEKEYBOARD)
    {
        // 키보드 입력 처리
        _keyState[raw->data.keyboard.VKey] = (raw->data.keyboard.Flags & RI_KEY_BREAK) ? 0 : 1;
    }

    if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        // 마우스 이동 처리
        _mousePos.x += raw->data.mouse.lLastX;
        _mousePos.y += raw->data.mouse.lLastY;

        // 마우스 버튼 처리
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            _mouseState[0] = 1;  // Left button down
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            _mouseState[0] = 0;  // Left button up
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            _mouseState[1] = 1;  // Right button down
        }
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            _mouseState[1] = 0;  // Right button up
        }

        // 마우스 휠 처리
        if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
        {
            _mouseWheelDelta = static_cast<SHORT>(raw->data.mouse.usButtonData);
        }

        _mouseDelta.x = _mousePos.x - _prevMousePos.x;
        _mouseDelta.y = _mousePos.y - _prevMousePos.y;

    }
}

void InputManager::RegisterRawInputDevices()
{
    _rawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    _rawInputDevices[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    _rawInputDevices[0].hwndTarget = _hWnd;

    _rawInputDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    _rawInputDevices[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    _rawInputDevices[1].hwndTarget = _hWnd;

    if (!::RegisterRawInputDevices(_rawInputDevices, 2, sizeof(RAWINPUTDEVICE)))
    {
        throw std::exception("Failed to register raw input devices.");
    }
}
