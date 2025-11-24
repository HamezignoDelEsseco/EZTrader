#include "sierrachart.h"

SCSFExport scsf_FVGLatestATR(SCStudyInterfaceRef sc)
{
    // Inputs indices
    int inputIndex = 0;
    SCInputRef ATRStudy = sc.Input[inputIndex++];
    SCInputRef ATRMultiplier = sc.Input[inputIndex++];

    // Subgraphs
    SCSubgraphRef LatestActiveFVUpHigh = sc.Subgraph[0];
    SCSubgraphRef LatestActiveFVUpLow  = sc.Subgraph[1];
    SCSubgraphRef LatestActiveFVUpMid  = sc.Subgraph[2];
    SCSubgraphRef LatestActiveFVDnHigh = sc.Subgraph[3];
    SCSubgraphRef LatestActiveFVDnLow  = sc.Subgraph[4];
    SCSubgraphRef LatestActiveFVDnMid  = sc.Subgraph[5];
    SCSubgraphRef LatestActiveFVUpId  = sc.Subgraph[6];
    SCSubgraphRef LatestActiveFVDnId  = sc.Subgraph[7];

    // A simple FVG struct
    struct FVG
    {
        int startIndex = 0.0f;
        float low = 0.0f;
        float high = 0.0f;
        float mid = 0.0f;
        float id = 0;
    };

    auto* LastFVGUp = reinterpret_cast<FVG*>(sc.GetPersistentPointer(0));
    auto* LastFVGDn = reinterpret_cast<FVG*>(sc.GetPersistentPointer(1));

    // Persistent int to avoid double-processing the same bar in streaming mode
    int& lastProcessedBar = sc.GetPersistentInt(0);

    if (sc.SetDefaults)
    {
        sc.GraphName = "FVG latest ATR mult";

        sc.AutoLoop = 1; // let Sierra call the study each bar automatically
        ATRStudy.Name = "ATR study";
        ATRStudy.SetStudyID(1);

        ATRMultiplier.Name = "ATR multiplier";
        ATRMultiplier.SetFloatLimits(1, 200);
        ATRMultiplier.SetFloat(2);

        // Subgraphs visual setup
        LatestActiveFVUpHigh.Name = "UH";
        LatestActiveFVUpHigh.DrawStyle = DRAWSTYLE_DASH;
        LatestActiveFVUpLow.Name  = "UL";
        LatestActiveFVUpLow.DrawStyle = DRAWSTYLE_DASH;
        LatestActiveFVUpMid.Name  = "UM";
        LatestActiveFVUpMid.DrawStyle = DRAWSTYLE_DASH;

        LatestActiveFVDnHigh.Name = "DH";
        LatestActiveFVDnHigh.DrawStyle = DRAWSTYLE_DASH;
        LatestActiveFVDnLow.Name  = "DL";
        LatestActiveFVDnLow.DrawStyle = DRAWSTYLE_DASH;
        LatestActiveFVDnMid.Name  = "DM";
        LatestActiveFVDnMid.DrawStyle = DRAWSTYLE_DASH;
        LatestActiveFVUpId.Name = "UI";
        LatestActiveFVUpId.DrawStyle = DRAWSTYLE_HIDDEN;
        LatestActiveFVDnId.Name = "DI";
        LatestActiveFVDnId.DrawStyle = DRAWSTYLE_HIDDEN;

        sc.ScaleRangeType  = SCALE_SAMEASREGION;
        sc.GraphRegion = 0;

        return;
    }

    const int idx = sc.Index;
    float& fvgIdUp = sc.GetPersistentFloat(0);
    float& fvgIdDn = sc.GetPersistentFloat(1);

    if (lastProcessedBar == idx)
        return;

    // Allocate maps on first use
    if (LastFVGUp == nullptr)
    {
        LastFVGUp = new FVG;
        sc.SetPersistentPointer(0, LastFVGUp);
    }

    if (LastFVGDn == nullptr)
    {
        LastFVGDn = new FVG;
        sc.SetPersistentPointer(1, LastFVGDn);
    }

    // Reset at start of trading day or early bars
    if (sc.IsNewTradingDay(idx) || idx < 4)
    {
        fvgIdUp = 0;
        fvgIdDn = 0;
        LastFVGUp->high = 0.0f;
        LastFVGUp->low = 0.0f;
        LastFVGUp->mid = 0.0f;
        LastFVGUp->id = 0.0f;

        LastFVGDn->high = 0.0f;
        LastFVGDn->low = 0.0f;
        LastFVGDn->mid = 0.0f;
        LastFVGDn->id = 0.0f;

        // reset outputs for cleanliness
        LatestActiveFVUpHigh[idx] = 0.0f;
        LatestActiveFVUpLow[idx]  = 0.0f;
        LatestActiveFVUpMid[idx]  = 0.0f;
        LatestActiveFVDnHigh[idx] = 0.0f;
        LatestActiveFVDnLow[idx]  = 0.0f;
        LatestActiveFVDnMid[idx]  = 0.0f;
        LatestActiveFVDnId[idx] = 0.0f;
        LatestActiveFVUpId[idx] = 0.0f;

        lastProcessedBar = idx;

        return;
    }

    SCFloatArray ATR;
    sc.GetStudyArrayUsingID(ATRStudy.GetStudyID(), 0, ATR);

    // Convenience locals for bar prices
    const float L1 = sc.Low[idx - 1];
    const float H1 = sc.High[idx - 1];
    const float L3 = sc.Low[idx - 3];
    const float H3 = sc.High[idx - 3];

    // Determine FVG presence based on your original logic
    const bool FVGUp = H3 < L1 && L1 - H3 >= ATR[idx-1] * ATRMultiplier.GetFloat();
    const bool FVGDn = L3 > H1 && L3 - H1 >= ATR[idx-1] * ATRMultiplier.GetFloat();

    // Insert or update Up FVG keyed by its high (H1). store most recent per key.
    if (FVGUp)
    {
        fvgIdUp++;
        LastFVGUp->high = L1;
        LastFVGUp->mid = (H3 + L1) / 2.0f;
        LastFVGUp->low = H3;
        LastFVGUp->startIndex = idx - 1;
        LastFVGUp->id = fvgIdUp;
    }

    // Insert or update Down FVG keyed by its low (L3).
    if (FVGDn)
    {
        fvgIdDn++;
        LastFVGDn->high = L3;
        LastFVGDn->mid = (H1 + L3) / 2.0f;
        LastFVGDn->low = H1;
        LastFVGDn->startIndex = idx - 1;
        LastFVGDn->id = fvgIdDn;
    }

    // Up FVG is stale
    if (sc.Low[idx] <= LastFVGUp->low)
    {
        LastFVGUp->high = 0.0f;
        LastFVGUp->low = 0.0f;
        LastFVGUp->mid = 0.0f;
    }

    // Dn FVG is stale
    if (sc.High[idx] >= LastFVGDn->high)
    {
        LastFVGDn->high = 0.0f;
        LastFVGDn->low = 0.0f;
        LastFVGDn->mid = 0.0f;
    }

    LatestActiveFVUpHigh[idx] = LastFVGUp->high;
    LatestActiveFVUpLow[idx]  = LastFVGUp->low;
    LatestActiveFVUpMid[idx]  = LastFVGUp->mid;
    LatestActiveFVUpId[idx] = LastFVGUp->id;

    LatestActiveFVDnHigh[idx] = LastFVGDn->high;
    LatestActiveFVDnLow[idx]  = LastFVGDn->low;
    LatestActiveFVDnMid[idx]  = LastFVGDn->mid;
    LatestActiveFVDnId[idx] = LastFVGDn->id;

    lastProcessedBar = idx;
}