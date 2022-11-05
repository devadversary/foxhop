#include "./include/prop_text.hpp"

PropText::PropText()
{
    Brush = 0;
    CurColor = {0,0,0,0};
    CurPos = { 0,0,0,0 };
    InitLen = 0;
    CurLen = 0;
    InitColor = { 0,0,0,0 };
    InitPos = { 0,0,0,0 };
    bActivate = 0;
    nStrLen = 0;
    pNext = 0;
    pPrev = 0;
    pStr = NULL;
    pTextFmt = NULL;
}

PropText::~PropText() {}

/**
    @brief  ������Ʈ�� �ʱ� �Ӽ� ����
    @remark ������ �ð����� ȭ�鿡 ������ �ʰ� �Ϸ��� StartColor�� ���İ��� 0���� ����.
*/
void PropText::Init(ID2D1RenderTarget* pRT, IDWriteTextFormat* pTexFmt, wchar_t* pText, int nTextLen, POSITION StartPos, D2D1_COLOR_F StartColor, int StartLen)
{
    pRT->CreateSolidColorBrush(StartColor, &Brush);
    pTextFmt = pTexFmt; /*��Ʈ ��� ���� ��ü���� ����*/
    InitPos = CurPos = StartPos;
    InitColor = CurColor = StartColor;
    InitLen = StartLen;
    CurLen = (float)StartLen;
    pStr = pText;
    nStrLen = nTextLen;
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

/**
    @brief ���ڿ��� �����ϰ� ���� ���� ����� �־��ش�
    @remark ����� �ؽ�Ʈ�� �ٷ� ���� �ϱ� ������ �������� ���� bCurrent ���ڰ� ����
*/
void PropText::SetText(MOTION_INFO MotionInfo, wchar_t* pText, int nStartTextLen, int nEndTextLen)
{
    ComMotionStrLen.clearChannel();
    addLenMotion(MotionInfo, FALSE, nStartTextLen, nEndTextLen);
}

void PropText::setRuntime(unsigned long time)
{
    CurPos = InitPos;
    CurColor = InitColor;
    CurLen = (float)InitLen;
    ComMotionMovement.setRuntime(time);
    ComMotionColor.setRuntime(time);
    ComMotionStrLen.setRuntime(time);
}

/**
    @brief ���� ������Ʈ
*/
BOOL PropText::update(unsigned long time)
{
    int nTrue = 0;
    /*��� ����*/
    nTrue += ComMotionMovement.update(time);
    nTrue += ComMotionColor.update(time);
    nTrue += ComMotionStrLen.update(time);
    if (!nTrue) return FALSE;
    return TRUE;
}

/**
    @brief ��� �����Ϳ� ���� ������
*/
void PropText::render(ID2D1RenderTarget* pRT)
{
    D2D1_RECT_F rect;

    Brush->SetColor(CurColor);
    rect = { (float)CurPos.x,
                (float)CurPos.y,
                (float)CurPos.x + CurPos.width,
                (float)CurPos.y + CurPos.height };
    pRT->DrawTextW(pStr, (UINT32)CurLen, pTextFmt, rect, Brush);
}