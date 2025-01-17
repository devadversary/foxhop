#pragma once

#include <windows.h>
#include <dwmapi.h>
#include <versionhelpers.h>
#pragma comment (lib, "dwmapi.lib")

#define WINDOWMODE_TRANSPARENT 0
#define WINDOWMODE_BLURBEHIND 1

#define WINDOWMODE_ERROR_LIBRARY_FAIL -1
#define WINDOWMODE_ERROR_CODE_FAIL -2
#define WINDOWMODE_ERROR_EXCUTE_FAIL -3
#define WINDOWMODE_ERROR_VERSION_UNAVAILABLE -4

#define WIN_MAJOR_MAX	    10    // Windows 10
#define WIN_MAJOR_MIN	    5    // Windows 2000 or XP

typedef enum _WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_LAST = 23
}WINDOWCOMPOSITIONATTRIB;

typedef enum ACCENT_STATE
{
	ACCENT_DISABLED = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_INVALID_STATE = 4,
	_ACCENT_STATE_SIZE = 0xFFFFFFFF
}ACCENT_STATE;

typedef struct WINCOMPATTRDATA
{
	DWORD attribute; //속성 지정자
	PVOID pData; //속성 설정 데이터
	ULONG dataSize; //속성 사이즈
}WINCOMPATTRDATA;

typedef struct ACCENTPOLICY
{
	ACCENT_STATE nAccentState;
	DWORD nFlags;
	DWORD nColor;
	DWORD nAnimationId;
}ACCENTPOLICY;

typedef BOOL(WINAPI* SetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

/**
	@brief  투명/반투명한 윈도우로 변환시켜준다.
	@remark 팝업윈도우에서만 잘 동작한다.
*/
int AlphaWindow(HWND hWnd, int mode);