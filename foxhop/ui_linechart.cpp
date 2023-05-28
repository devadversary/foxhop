#include "./include/ui_linechart.hpp"
#include "./include/ui_system.hpp"
#include <cstdarg>

UI_LineChart::UI_LineChart(UISystem* pUISys, pfnUIHandler pfnCallback, POSITION Pos, IDWriteTextFormat* pTextFmt, unsigned long ChartPixelGap, float EnsureMinVal, float EnsureMaxVal, float BaseSplitVal, unsigned long nSplitCnt, unsigned long Delay, UI_LineChart_MotionParam MotionParam)
{
    D2D1_STROKE_STYLE_PROPERTIES style;

    uiSys = pUISys;
    pRenderTarget = pUISys->D2DA.pRenTarget;
    Focusable = FALSE;
    uiMotionState = eUIMotionState::eUMS_PlayingVisible;

    DefaultHandler = DefaultLineChartProc;
    MessageHandler = pfnCallback;
    uiPos = Pos;
    Motion = MotionParam;
    
    style.dashCap = D2D1_CAP_STYLE_FLAT;
    style.startCap = D2D1_CAP_STYLE_FLAT;
    style.endCap = D2D1_CAP_STYLE_FLAT;
    style.lineJoin = D2D1_LINE_JOIN_MITER;
    style.dashStyle = D2D1_DASH_STYLE_DASH;
    style.miterLimit = 0;
    style.dashOffset = 0;
    uiSys->D2DA.pFactory->CreateStrokeStyle(&style, NULL, 0, &pStroke);

    InitializeSRWLock(&lock);
    ViewDataWidth = ChartPixelGap;
    ViewDataCnt = (uiPos.x2 / ViewDataWidth) + 1;
    ValidViewDataCnt = 0;
    EnsureMin = EnsureMinVal;
    EnsureMax = EnsureMaxVal;
    IntervalMin = EnsureMinVal;
    IntervalMax = EnsureMaxVal;
    SplitCnt = nSplitCnt;
    BaseSplit = BaseSplitVal;
    ScrollComp = new ComponentMotion();
    for (int i = 0; i < ViewDataCnt; i++) {
        ChartObject* pObj = new ChartObject(this, pStroke, ViewDataWidth, uiPos.y2);
        ViewData.emplace_back(pObj);
        PropLines.emplace_back(new PropLine(pRenderTarget));
    }
    PropLines.emplace_back(new PropLine(pRenderTarget)); /*선 갯수는 하나 더 많아야 함.*/

    pBoxBg = new PropBox(pRenderTarget);
    pBoxFrame = new PropBox(pRenderTarget);
    resume(Delay);
}

UI_LineChart::~UI_LineChart()
{

}

void UI_LineChart::DefaultLineChartProc(UI* pUI, UINT Message, WPARAM wParam, LPARAM lParam)
{
    UI_LineChart* pChart = (UI_LineChart*)pUI;

    switch (Message) {
        case WM_MOUSEWHEEL: {
            long long ScrollPx;
            AcquireSRWLockExclusive(&pChart->lock);
            ScrollPx = pChart->ScrollPixel - (GET_WHEEL_DELTA_WPARAM(wParam) / 2);
            ReleaseSRWLockExclusive(&pChart->lock);
            pChart->SetScroll(ScrollPx);
            break;
        }
    }
}

void UI_LineChart::SetScroll(long long TargetScrollPx)
{
    MOTION_PATTERN patt;
    MOTION_INFO    mi;
    unsigned long long DataCnt;
    long long MaxScroll;

    /*초기화 / 소멸 모션 진행중엔 스크롤 X*/
    if (uiMotionState != eUIMotionState::eUMS_Visible) return;

    AcquireSRWLockExclusive(&lock);

    /*스크롤 경계 관리*/
    DataCnt = MainData.size();
    MaxScroll = (DataCnt * ViewDataWidth) - uiPos.x2;
    if (MaxScroll < 0) MaxScroll = 0;
    if (TargetScrollPx < 0) TargetScrollPx = 0;
    else if (TargetScrollPx > MaxScroll) TargetScrollPx = MaxScroll;
    ScrollPixel = TargetScrollPx;

    /*스크롤 모션 진행 재시작*/
    ScrollComp->clearChannel();
    mi = InitMotionInfo(eMotionForm::eMotion_x3_2, 0, Motion.PitchScroll);
    patt = InitMotionPattern(mi, NULL);
    AddChain(&patt, &CurrentScroll, CurrentScroll, (float)ScrollPixel);
    ScrollComp->addChannel(patt);

    ReleaseSRWLockExclusive(&lock);
}

void UI_LineChart::AddValue(float Val, wchar_t* pLabel, BOOL bAutoScroll)
{
    unsigned long long DataCnt;
    CHART_DATA data;

    AcquireSRWLockExclusive(&lock);

    data.Selected = FALSE;
    data.MotionPlayed = FALSE;
    data.Value = Val;
    wcscpy_s(data.Label, MAX_CHART_LABEL_LEN, pLabel);

    MainData.emplace_back(data);
    DataCnt = MainData.size();
    ValidViewDataCnt = ViewDataCnt > DataCnt ? DataCnt : ViewDataCnt;
    ReleaseSRWLockExclusive(&lock);

    if (uiMotionState != eUIMotionState::eUMS_Visible) return;
    if (bAutoScroll) SetScroll(DataCnt * ViewDataWidth);
}

void UI_LineChart::EditValue(unsigned long long DataIdx, float NewVal, wchar_t* NewLabel)
{
    CHART_DATA data;

    AcquireSRWLockExclusive(&lock);
    data = MainData[DataIdx];
    data.Value = NewVal;
    wcscpy_s(data.Label, MAX_CHART_LABEL_LEN, NewLabel);
    MainData[DataIdx].Value = NewVal;
    ReleaseSRWLockExclusive(&lock);
}

void UI_LineChart::ResumeFrame(unsigned long Delay)
{
    MOTION_INFO mi;

    switch (Motion.MotionInitFrame) {
        case eLineChartMotionPattern::eInitFrame_Default: {
            pBoxFrame->Init(uiPos, ALL_ZERO, FALSE);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Motion.DelayInitFrame, Motion.PitchInitFrame);
            pBoxFrame->SetColor(mi, TRUE, ALL_ZERO, Motion.ColorFrame);
            break;
        }
    }
}

void UI_LineChart::PauseFrame(unsigned long Delay)
{
    MOTION_INFO mi;

    switch (Motion.MotionPauseFrame) {
        case eLineChartMotionPattern::ePauseFrame_Default: {
            mi = InitMotionInfo(eMotionForm::eMotion_None, Motion.DelayPauseFrame, Motion.PitchPauseFrame);
            pBoxFrame->SetColor(mi, TRUE, ALL_ZERO, ALL_ZERO);
            break;
        }
    }
}

void UI_LineChart::ResumeBg(unsigned long Delay)
{
    MOTION_INFO mi;

    switch (Motion.MotionInitBg) {
        case eLineChartMotionPattern::eInitBg_Default: {
            pBoxBg->Init(uiPos, ALL_ZERO);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, Motion.PitchInitFrame);
            pBoxBg->SetColor(mi, TRUE, ALL_ZERO, Motion.ColorBg);
            break;
        }
    }
}

void UI_LineChart::PauseBg(unsigned long Delay)
{
    MOTION_INFO mi;

    switch (Motion.MotionPauseBg) {
        case eLineChartMotionPattern::ePauseBg_Default: {
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, Motion.PitchPauseBg);
            pBoxBg->SetColor(mi, TRUE, ALL_ZERO, ALL_ZERO);
            break;
        }
    }
}

void UI_LineChart::ResumeDataOrder(unsigned long Delay)
{
    switch (Motion.MotionInitDataOrder){
        case eLineChartMotionPattern::eInitDataOrder_Default: {
            for (int i = 0; i < ValidViewDataCnt; i++) ViewData[i]->resume(Delay);
            break;
        }
    }
}

void UI_LineChart::PauseDataOrder(unsigned long Delay)
{
    switch (Motion.MotionPauseDataOrder) {
        case eLineChartMotionPattern::ePauseDataOrder_Default: {
            for (int i = 0; i < ValidViewDataCnt; i++) ViewData[i]->pause(Delay);
            break;
        }
    }
}

void UI_LineChart::resume(int nDelay)
{
    uiMotionState = eUIMotionState::eUMS_PlayingVisible;
    ResumeFrame(nDelay + Motion.DelayInitFrame);
    ResumeBg(nDelay + Motion.DelayInitBg);
    //ResumeLabel(nDelay + Motion.DelayInitText);
    ResumeDataOrder(nDelay + Motion.DelayInitFrame);
}

void UI_LineChart::pause(int nDelay)
{
    uiMotionState = eUIMotionState::eUMS_PlayingHide;
    AcquireSRWLockExclusive(&lock);
    PinCount = ValidViewDataCnt; /*현재의 유효 뷰 갯수 (Resume시엔 PinCount를 따르지 않음)*/
    ScrollComp->clearChannel(); /*스크롤 모션 정지*/
    ReleaseSRWLockExclusive(&lock);
    PauseFrame(nDelay + Motion.DelayPauseFrame);
    PauseBg(nDelay + Motion.DelayPauseBg);
    //PauseLabel(nDelay + Motion.DelayPauseText);
    PauseDataOrder(nDelay + Motion.DelayPauseFrame);
}

BOOL UI_LineChart::update(unsigned long time)
{
    BOOL bUpdated;
    BOOL bNeedRangeUpdate = FALSE;
    long long ModIdx;
    float IntervalMin, IntervalMax; /*구간 최대최솟값*/
    size_t MainDataCnt;

    if (uiMotionState == eUIMotionState::eUMS_Hide) return FALSE;

    AcquireSRWLockExclusive(&lock);
    bUpdated = ScrollComp->update(time);
    bUpdated |= pBoxFrame->update(time);
    bUpdated |= pBoxBg->update(time);
    //bUpdated |= p->update(time);

    ViewStartIdx = (long long)CurrentScroll / ViewDataWidth;
    ModIdx = ViewStartIdx % ViewDataCnt;

    /*데이터 바인딩*/
    MainDataCnt = MainData.size();
    for (int i = 0; i < ValidViewDataCnt; i++) {
        unsigned long long BindTargetIdx = ViewStartIdx + i;
        long long TmpIdx = (ModIdx+i) % ViewDataCnt;
        ChartObject* pObj = ViewData[TmpIdx];

        if (pObj->MainDataIdx == BindTargetIdx) continue; /*이미 바인드된 데이터*/
        if (MainDataCnt <= BindTargetIdx) break;/*딱뎀상황에선 자투리 접근 X*/
        /*구간 최대최소값 갱신*/
        IntervalMax = max(EnsureMax, pObj->Value);
        IntervalMin = min(EnsureMin, pObj->Value);
        bNeedRangeUpdate = TRUE; /*좀 더 최적화 필요. 범위 벗어난게 있어야 어쩌구*/

        pObj->MainDataIdx = BindTargetIdx;
        pObj->Value = MainData[BindTargetIdx].Value; /*모션처리 X. 값만 갱신*/
        wcscpy_s(pObj->LabelText, ARRAYSIZE(pObj->LabelText), MainData[BindTargetIdx].Label);
    }

    /*바인딩이 일어났다면 최대최소값 갱신*/
    if (bNeedRangeUpdate) {
        for (int k = 0; k < ValidViewDataCnt; k++) {
            ViewData[k]->SetValue(TRUE, ViewData[k]->Value, IntervalMax, IntervalMin, ViewData[k]->LabelText);
            ViewData[k]->ResumeGuideLine(TRUE, 0);
            ViewData[k]->ResumePoint(TRUE, 0);
        }
    }


    /*뷰 행 업데이트*/
    if (uiMotionState == eUIMotionState::eUMS_PlayingHide)
        for (int i = 0; i < PinCount; i++) ViewData[i]->update(time);
    else
        for (int i = 0; i < ValidViewDataCnt; i++) ViewData[i]->update(time);

    ReleaseSRWLockExclusive(&lock);

    if (!bUpdated) {
        if (uiMotionState == eUIMotionState::eUMS_PlayingHide)
            uiMotionState = eUIMotionState::eUMS_Hide;

        else if (uiMotionState == eUIMotionState::eUMS_PlayingVisible)
            uiMotionState = eUIMotionState::eUMS_Visible;
    }
    return bUpdated;
}

void UI_LineChart::render()
{
    D2D_MATRIX_3X2_F OldMat;
    D2D_MATRIX_3X2_F TmpMat;
    D2D1_RECT_F ClipRect;
    unsigned long ModIdx;
    unsigned long ModScroll;
    unsigned long long MainDataCnt;

    AcquireSRWLockShared(&lock);
    pBoxBg->render(pRenderTarget);
    
    ClipRect.left = uiPos.x;
    ClipRect.top = uiPos.y;
    ClipRect.right = uiPos.x + uiPos.x2;
    ClipRect.bottom = uiPos.y + uiPos.y2;

    ModScroll = (unsigned long)CurrentScroll % ViewDataWidth;
    ModIdx = ViewStartIdx % ViewDataCnt;

    pRenderTarget->GetTransform(&OldMat); /*기존 행렬 백업*/
    pRenderTarget->PushAxisAlignedClip(ClipRect, D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    TmpMat = OldMat;
    TmpMat.dx += uiPos.x;
    TmpMat.dy += uiPos.y; /*UI 위치에 맞게 행렬 조정*/
    TmpMat.dx -= (float)ModScroll; /*스크롤된 오프셋 적용 준비*/
    MainDataCnt = MainData.size();
    for (int i = 0; i < ValidViewDataCnt; i++) {
        unsigned long TmpIdx = (ModIdx + i) % ValidViewDataCnt;
        if (MainDataCnt <= ViewStartIdx + i) break; /*딱뎀시 자투리 X*/
        pRenderTarget->SetTransform(TmpMat);
        ViewData[TmpIdx]->render();
        TmpMat.dx += (float)ViewDataWidth;
    }
    pRenderTarget->PopAxisAlignedClip();
    pRenderTarget->SetTransform(OldMat);
    
    /*
    for() text 렌더링
    */

    pBoxFrame->render(pRenderTarget);
    ReleaseSRWLockShared(&lock);
}
































ChartObject::ChartObject(UI_LineChart* pParent, ID2D1StrokeStyle* Stroke, unsigned long width, unsigned long height)
{
    pChart = pParent;
    Pos = { 0.f, 0.f, (float)width, (float)height};
    MainDataIdx = -1;
    Value = 0;
    ValueMax = 0;
    ValueMin = 0;
    pStroke = Stroke;
    Width = width;
    Height = height;
    pGuideLine = new PropLine(pChart->pRenderTarget, pStroke);
    pBoxPoint = new PropBox(pChart->pRenderTarget);
    pCircle = new PropCircle(pChart->pRenderTarget);
    pLabel = new PropText(pChart->pRenderTarget, 32);
}

ChartObject::~ChartObject()
{

}


void ChartObject::SetValue(BOOL bMotion, float Value, float Max, float Min, wchar_t* Label)
{
    MOTION_INFO mi;
    POSITION PointPos;
    unsigned long yHeight;
    unsigned long PointSize;
    /*부모의 AddValue로 데이터가 추가되면 update의 bind 작업에의해 무조건 SetValue 가 호출됨*/

    ValueMax = Max;
    ValueMin = Min;
    wcscpy_s(LabelText, MAX_CHART_LABEL_LEN, Label);
    /*Min, Max, Value에 따른 y좌표 구하기*/
    yHeight = Pos.y2 * ((Value - ValueMin) / (ValueMax - ValueMin));
    PointSize = pChart->Motion.PointSize;
    PointPos.x = (float)((Width<<1) - (PointSize<<1));
    PointPos.y = (float)(yHeight - (PointSize<<1));
    PointPos.x2 = (float)PointSize;
    PointPos.y2 = (float)PointSize;

    switch (pChart->Motion.MotionAdjustData) {
    case eLineChartMotionPattern::eAdjustData_Default:
        mi = InitMotionInfo(eMotionForm::eMotion_None, 0, pChart->Motion.PitchAdjustData);
        pBoxPoint->SetPos(mi, TRUE, ALL_ZERO, PointPos);
        break;
    }
}

BOOL ChartObject::update(unsigned long time)
{
    BOOL bUpdated;
    
    bUpdated = pGuideLine->update(time);
    bUpdated |= pBoxPoint->update(time);
    bUpdated |= pCircle->update(time);
    bUpdated |= pLabel->update(time);
    return bUpdated;
}

void ChartObject::ResumeGuideLine(BOOL bMotion, unsigned long Delay)
{
    MOTION_INFO mi;
    eLineChartMotionPattern patt;
    POSITION LinePos;

    if (bMotion) patt = pChart->Motion.MotionInitChartGuideLine;
    else patt = eLineChartMotionPattern::eInitChartGuideLine_Default;

    LinePos.x = Width / 2;
    LinePos.y = 0;
    LinePos.x2 = Width / 2;
    LinePos.y2 = Height;

    switch (patt) {
        case eLineChartMotionPattern::eInitChartGuideLine_Default: {
            pGuideLine->Init(LinePos, ALL_ZERO);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, pChart->Motion.PitchInitChartGuideLine);
            pGuideLine->SetColor(mi, TRUE, ALL_ZERO, pChart->Motion.ColorGuideLine);
            break;
        }

        case eLineChartMotionPattern::eInitChartGuideLine_Expend: {
            POSITION StartPos = LinePos;
            StartPos.y += Height / 2;
            StartPos.y2 = 0;
            pGuideLine->Init(StartPos, ALL_ZERO);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, 0);
            pGuideLine->SetColor(mi, TRUE, ALL_ZERO, pChart->Motion.ColorGuideLine);
            mi = InitMotionInfo(eMotionForm::eMotion_x3_2, Delay, pChart->Motion.PitchInitChartGuideLine);
            pGuideLine->SetPos(mi, TRUE, ALL_ZERO, LinePos);
            break;
        }
    }
}

void ChartObject::PauseGuideLine(unsigned long Delay)
{
    MOTION_INFO mi;
    D2D1_COLOR_F TmpColor;
    POSITION LinePos;

    LinePos.x = Width / 2;
    LinePos.y = 0;
    LinePos.x2 = Width / 2;
    LinePos.y2 = Height;

    switch (pChart->Motion.MotionPauseChartGuideLine) {
        case eLineChartMotionPattern::ePauseChartGuideLine_Default: {
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, pChart->Motion.PitchPauseChartGuideLine);
            TmpColor = pChart->Motion.ColorGuideLine;
            TmpColor.a = 0;
            pGuideLine->SetColor(mi, TRUE, ALL_ZERO, TmpColor);
            break;
        }

        case eLineChartMotionPattern::ePauseChartGuideLine_Collapse: {
            POSITION EndPos = LinePos;
            EndPos.y += Height / 2;
            EndPos.y2 = 0;
            mi = InitMotionInfo(eMotionForm::eMotion_x3_2, Delay, pChart->Motion.PitchInitChartGuideLine);
            pGuideLine->SetPos(mi, TRUE, ALL_ZERO, EndPos);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay+ pChart->Motion.PitchInitChartGuideLine, 0);
            pGuideLine->SetColor(mi, TRUE, ALL_ZERO, ALL_ZERO);
            break;
        }
    }
}

void ChartObject::ResumePoint(BOOL bMotion, unsigned long Delay)
{
    MOTION_INFO mi;
    eLineChartMotionPattern patt;
    unsigned long yHeight;
    unsigned long PointSize;
    POSITION PointPos;

    PointSize = pChart->Motion.PointSize;
    yHeight = Pos.y2 * ((Value - ValueMin) / (ValueMax - ValueMin));
    PointPos.x = (float)((Width >> 1) - (PointSize >> 1));
    PointPos.y = (float)((Height - yHeight) - (PointSize >> 1));
    PointPos.width = (float)PointSize;
    PointPos.height = (float)PointSize;

    if (bMotion) patt = pChart->Motion.MotionInitChartPoint;
    else patt = eLineChartMotionPattern::eInitChartPoint_Default;

    switch (patt) {
        case eLineChartMotionPattern::eInitChartPoint_Default: {
            pBoxPoint->Init(PointPos, ALL_ZERO);
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, pChart->Motion.PitchInitChartPoint);
            pBoxPoint->SetColor(mi, TRUE, ALL_ZERO, pChart->Motion.ColorChartPoint);
            break;
        }
    }
}

void ChartObject::PausePoint(unsigned long Delay)
{
    MOTION_INFO mi;
    D2D1_COLOR_F TmpColor;

    switch (pChart->Motion.MotionPauseChartPoint) {
        case eLineChartMotionPattern::ePauseChartPoint_Default: {
            mi = InitMotionInfo(eMotionForm::eMotion_None, Delay, pChart->Motion.PitchPauseChartPoint);
            TmpColor = pChart->Motion.ColorChartPoint;
            TmpColor.a = 0;
            pBoxPoint->SetColor(mi, TRUE, ALL_ZERO, TmpColor);
            break;
        }
    }
}

void ChartObject::ResumeLabel(BOOL bMotion, unsigned long Delay)
{
    eLineChartMotionPattern patt;

    if (bMotion) patt = pChart->Motion.MotionInitChartLabel;
    else patt = eLineChartMotionPattern::eInitChartLabel_Default;

    switch (patt) {
        case eLineChartMotionPattern::eInitChartLabel_Default: {
            break;
        }
    }
}

void ChartObject::PauseLabel(unsigned long Delay)
{
    switch (pChart->Motion.MotionPauseChartLabel) {
        case eLineChartMotionPattern::ePauseChartLabel_Default: {
            break;
        }
    }
}

void ChartObject::render()
{
    ID2D1RenderTarget* pRT = pChart->pRenderTarget;

    //pCircle->render(pRT);
    pGuideLine->render(pRT);
    pBoxPoint->render(pRT);
    //pLabel->render(pRT);
}

void ChartObject::pause(unsigned long Delay)
{
    PauseGuideLine(Delay + pChart->Motion.DelayPauseChartGuideLine);
    PausePoint(Delay + pChart->Motion.DelayPauseChartPoint);
}

void ChartObject::resume(unsigned long Delay)
{
    ResumeGuideLine(TRUE, Delay + pChart->Motion.DelayInitChartGuideLine);
    ResumePoint(TRUE, Delay + pChart->Motion.DelayInitChartPoint);
}








