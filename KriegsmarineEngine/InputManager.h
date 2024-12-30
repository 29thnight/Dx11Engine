#pragma once
#include "Core.Definition.h"

constexpr int KEY_COUNT = 256;
constexpr int MOUSE_BUTTON_COUNT = 3;

class InputManager : public Singleton<InputManager>
{
private:
	friend class Singleton;

private:
	InputManager() = default;
	~InputManager() = default;

public:
	void Initialize(HWND hwnd);
	HWND GetHwnd() const;
	void Update();

public:
	bool IsKeyDown(unsigned int key) const;
	bool IsKeyPressed(unsigned int key) const;
	bool IsKeyReleased(unsigned int key) const;

public:
	bool IsMouseButtonDown(MouseKey button) const;
	bool IsMouseButtonPressed(MouseKey button) const;
	bool IsMouseButtonReleased(MouseKey button) const;

public:
	void SetMousePos(POINT pos);
	POINT GetMousePos() const;
	POINT GetMouseDelta() const;
	void SetCursorPos(int x, int y);
	void GetCursorPos(LPPOINT lpPoint);
	void HideCursor();
	void ShowCursor();
	void ResetMouseDelta();
	short GetMouseWheelDelta() const;

public:
	void ProcessRawInput(LPARAM lParam);
	void RegisterRawInputDevices();

private:
	HWND _hWnd{ nullptr };
	RAWINPUTDEVICE _rawInputDevices[2]{};

private:
	bool _keyState[KEY_COUNT]{};
	bool _prevKeyState[KEY_COUNT]{};

private:
	bool _mouseState[MOUSE_BUTTON_COUNT]{};
	bool _prevMouseState[MOUSE_BUTTON_COUNT]{};

private:
	POINT _prevMousePos{};
	POINT _mousePos{};
	POINT _mouseDelta{};
	short _mouseWheelDelta{};

private:
	bool _isCursorHidden{ false };
};

inline static auto& InputManagement = InputManager::GetInstance();
