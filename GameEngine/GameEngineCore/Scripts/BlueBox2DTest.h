#pragma once
#include <windows.h>

// Exposed helpers for overlay use.
void InitBlueBoxText(const RECT& bounds);
void UpdateBlueBoxText(double dt, HDC hdc, const RECT& bounds);

// Retain the headless entry point for tests.
int RunBlueBox2DTest();
