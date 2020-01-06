
#include "HISSToolsFreeze.h"

#include "IPlug_include_in_plug_src.h"
#include "IControls.h"


class IVMenuControl : public IControl, public IVectorBase
{
public:
    
    IVMenuControl(const IRECT& bounds, int paramIdx) : IControl(bounds, paramIdx)
    {
        AttachIControl(this, "");
        DisablePrompt(false);
    }
    
    void OnInit() override
    {
        const IParam* pParam = GetParam();
        
        if(pParam)
            mLabelStr.Set(pParam->GetNameForHost());
    }
    
    void Draw(IGraphics& g) override
    {
        DrawBackGround(g, mRECT);
        DrawLabel(g);
        DrawValue(g, false);
    }
    
    void OnResize() override
    {
        SetTargetRECT(MakeRects(mRECT, true));
        IControl::SetDirty(false);
    }
    
    void OnMouseDown(float x, float y, const IMouseMod& mod) override
    {
        PromptUserInput(IRECT(mRECT.L, mRECT.B, mRECT.L, mRECT.B), GetValIdxForPos(x, y));
    }
    
    void SetDirty(bool push, int valIdx) override
    {
        IControl::SetDirty(push);
        
        const IParam* pParam = GetParam();
        
        if(pParam)
            pParam->GetDisplayForHostWithLabel(mValueStr);
    }
};

HISSToolsFreeze::HISSToolsFreeze(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms)), mProxy(new FromPlugProxy()), mDSP(mProxy)
{
    GetParam(kFFTSize)->InitEnum("FFT Size", 3, 8);
    GetParam(kFFTSize)->SetDisplayText(0, "512");
    GetParam(kFFTSize)->SetDisplayText(1, "1024");
    GetParam(kFFTSize)->SetDisplayText(2, "2048");
    GetParam(kFFTSize)->SetDisplayText(3, "4096");
    GetParam(kFFTSize)->SetDisplayText(4, "8192");
    GetParam(kFFTSize)->SetDisplayText(5, "16384");
    GetParam(kFFTSize)->SetDisplayText(6, "32768");
    GetParam(kFFTSize)->SetDisplayText(7, "65536");
    
    GetParam(kOverlap)->InitEnum("Overlap", 2, 3);
    GetParam(kOverlap)->SetDisplayText(0, "2");
    GetParam(kOverlap)->SetDisplayText(1, "4");
    GetParam(kOverlap)->SetDisplayText(2, "8");

    GetParam(kBlur)->InitDouble("Blur", 0., 0., 2000.0, 0.1, "ms");

    GetParam(kTime)->InitDouble("Time", 0., 0., 10000.0, 0.1, "ms");
    
    mMakeGraphicsFunc = [&]() {
        return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
    };
    
    mLayoutFunc = [&](IGraphics* pGraphics) {
        pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
        pGraphics->AttachPanelBackground(COLOR_GRAY);
        pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
        const IRECT b = pGraphics->GetBounds();
        pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50).GetVShifted(200), "HISSTools Freeze", IText(50)));
        pGraphics->AttachControl(new IVMenuControl(b.GetCentredInside(100, 40).GetVShifted(-160), kFFTSize));
        pGraphics->AttachControl(new IVMenuControl(b.GetCentredInside(100, 40).GetVShifted(-110), kOverlap));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-20), kBlur));
        pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(100), kTime));
    };
}

//const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, float lo = 0., float hi = 1.f, float defaultVal = 0., uint32_t bufferSize = 100

void HISSToolsFreeze::OnReset()
{
    mDSP.reset(GetSampleRate(), GetBlockSize());
    
    
}

void HISSToolsFreeze::OnTimeChange()
{
    int FFT = 1 << (GetParam(kFFTSize)->Int() + 9);
    int overlap = 1 << (GetParam(kOverlap)->Int() + 1);

    double blurTime = GetParam(kBlur)->Value();
    double hopTime = 1000 * FFT / (overlap * GetSampleRate());
    double frames = std::round(blurTime / hopTime) + 1;
    
    mProxy->sendFromHost(2, &frames, 1);
}

void HISSToolsFreeze::OnParamChange(int paramIdx, EParamSource source, int sampleOffset)
{
    switch (paramIdx)
    {
        case kFFTSize:
        {
            double FFT = 1 << (GetParam(kFFTSize)->Int() + 9);
            mProxy->sendFromHost(1, &FFT, 1);
            OnTimeChange();
        }
        
        case kOverlap:
        {
            double overlap = 1 << (GetParam(kOverlap)->Int() + 1);
            mProxy->sendFromHost(0, &overlap, 1);
            OnTimeChange();
        }
            
        case kBlur:
        {
            OnTimeChange();
        }
            
        case kTime:
        {
            double time = GetParam(kTime)->Value();
            mProxy->sendFromHost(3, &time, 1);
        }
        
        default:
            break;
    }
}

void HISSToolsFreeze::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    sample triggers[nFrames];
    sample* allInputs[3];
    
    allInputs[0] = inputs[0];
    allInputs[1] = inputs[1];
    allInputs[2] = triggers;

    for (int i = 0; i < nFrames; i++)
        triggers[i] = (std::rand() / (RAND_MAX + 1.0)) > 0.9999;
    
    mDSP.process(allInputs, outputs, nFrames);
}


