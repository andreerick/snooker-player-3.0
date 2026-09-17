#include "UiUtils.h"

#include <QWidget>

#ifdef Q_OS_WIN
// NOMINMAX : sans ca, windows.h (inclus par dwmapi.h) definit des macros
// min/max qui cassent tout appel a std::min/std::max ailleurs dans le
// programme (erreur de compilation cryptique "jeton non conforme").
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

void applyDarkTitleBar(QWidget* window)
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    BOOL enabled = TRUE;
    const DWORD kDwmwaUseImmersiveDarkMode = 20;
    ::DwmSetWindowAttribute(hwnd, kDwmwaUseImmersiveDarkMode, &enabled, sizeof(enabled));
#else
    Q_UNUSED(window);
#endif
}
