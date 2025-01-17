#include "./include/prop_text.hpp"

PropText::PropText(ID2D1RenderTarget* pRT, int MaxLength, IDWriteTextFormat* pTexFmt)
{
    pRT->CreateSolidColorBrush({ 0,0,0,0 }, &Brush);
    CurColor = { 0,0,0,0 };
    CurPos = { 0,0,0,0 };
    CurLen = 0;
    InitColor = { 0,0,0,0 };
    InitPos = { 0,0,0,0 };
    nStrLen = 0;
    MaxLen = MaxLength;
    pStr = (wchar_t*)malloc(sizeof(wchar_t) * (MaxLength+1));
    pStr[0] = 0;
    pTextFmt = pTexFmt;
}

PropText::~PropText()
{
    if (Brush) Brush->Release();
}

/**
    @brief  오브젝트의 초기 속성 셋팅
    @remark 딜레이 시간동안 화면에 나가지 않게 하려면 StartColor의 알파값을 0으로 주자.
*/
void PropText::Init(wchar_t* pText, int nTextLen, POSITION StartPos, D2D1_COLOR_F StartColor, int StartLen)
{
    InitPos = CurPos = StartPos;
    InitColor = CurColor = StartColor;
    CurLen = (float)StartLen;
    nStrLen = wcslen(pText);
    wcscpy_s(pStr, MaxLen, pText);
    //pStr = pText;
    ComMotionColor.clearChannel();
    ComMotionMovement.clearChannel();
    ComMotionStrLen.clearChannel();
}

void PropText::addMovementMotion(MOTION_INFO MotionInfo, BOOL bAppend, POSITION StartPos, POSITION EndPos)
{
    MOTION_PATTERN mc;

    mc = InitMotionPattern(MotionInfo, NULL);
    AddChain(&mc, &CurPos.x, StartPos.x, EndPos.x);
    AddChain(&mc, &CurPos.y, StartPos.y, EndPos.y);
    AddChain(&mc, &CurPos.width, StartPos.width, EndPos.width);
    AddChain(&mc, &CurPos.height, StartPos.height, EndPos.height);
    if (bAppend) ComMotionMovement.appendChannel(mc);
    else ComMotionMovement.addChannel(mc);
}

void PropText::addColorMotion(MOTION_INFO MotionInfo, BOOL bAppend, D2D1_COLOR_F StartColor, D2D1_COLOR_F EndColor)
{
    MOTION_PATTERN mc;

    mc = InitMotionPattern(MotionInfo, NULL);
    AddChain(&mc, &CurColor.r, StartColor.r, EndColor.r);
    AddChain(&mc, &CurColor.g, StartColor.g, EndColor.g);
    AddChain(&mc, &CurColor.b, StartColor.b, EndColor.b);
    AddChain(&mc, &CurColor.a, StartColor.a, EndColor.a);
    if (bAppend) ComMotionColor.appendChannel(mc);
    else ComMotionColor.addChannel(mc);
}

void PropText::addLenMotion(MOTION_INFO MotionInfo, BOOL bAppend, int nStartLen, int nEndLen)
{
    MOTION_PATTERN mc;

    mc = InitMotionPattern(MotionInfo, NULL);
    AddChain(&mc, &CurLen, (float)nStartLen, (float)nEndLen);
    if (bAppend) ComMotionStrLen.appendChannel(mc);
    else ComMotionStrLen.addChannel(mc);
}

void PropText::SetPos(MOTION_INFO MotionInfo, BOOL bCurrent, POSITION StartPos, POSITION EndPos)
{
    POSITION TmpPos;

    ComMotionMovement.clearChannel();
    if (bCurrent) TmpPos = CurPos;
    else TmpPos = StartPos;
    addMovementMotion(MotionInfo, FALSE, TmpPos, EndPos);
}

void PropText::SetColor(MOTION_INFO MotionInfo, BOOL bCurrent, D2D1_COLOR_F StartColor, D2D1_COLOR_F EndColor)
{
    D2D1_COLOR_F TmpColor;

    ComMotionColor.clearChannel();
    if (bCurrent) TmpColor = CurColor;
    else TmpColor = StartColor;
    addColorMotion(MotionInfo, FALSE, TmpColor, EndColor);
}

void PropText::SetLen(MOTION_INFO MotionInfo, BOOL bCurrent, unsigned long StartLen, unsigned long EndLen)
{
    unsigned long TmpLen;

    ComMotionStrLen.clearChannel();
    if (bCurrent) TmpLen = CurLen;
    else TmpLen = StartLen;
    addLenMotion(MotionInfo, FALSE, TmpLen, EndLen);
}

void PropText::SetText(wchar_t* Str)
{
    ComMotionStrLen.clearChannel();
    wcscpy_s(pStr, MaxLen, Str);
    CurLen = wcslen(pStr);
}

/**
    @brief 상태 업데이트
*/
BOOL PropText::update(unsigned long time)
{
    BOOL bUpdated = FALSE;
    /*모션 보정*/
    bUpdated |= ComMotionMovement.update(time);
    bUpdated |= ComMotionColor.update(time);
    bUpdated |= ComMotionStrLen.update(time);
    if (!bUpdated) return FALSE;
    return TRUE;
}

/**
    @brief 모션 데이터에 따라 렌더링
*/
void PropText::render(ID2D1RenderTarget* pRT)
{
    D2D1_RECT_F rect;
    Brush->SetColor(CurColor);
    rect = { (float)CurPos.x,
                (float)CurPos.y,
                (float)CurPos.x + CurPos.width,
                (float)CurPos.y + CurPos.height };
    if(pStr)
        pRT->DrawTextW(pStr, (UINT32)CurLen, pTextFmt, rect, Brush);
}