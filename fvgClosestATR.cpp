#include "sierrachart.h"

SCSFExport scsf_FVGClosestATR(SCStudyInterfaceRef sc)
{
    // Inputs indices
    int inputIndex = 0;
    SCInputRef ATRStudy = sc.Input[inputIndex++];
    SCInputRef ATRMultiplier = sc.Input[inputIndex++];
    SCInputRef LocalMinFilter = sc.Input[inputIndex++];
    SCInputRef MaxZoneSize = sc.Input[inputIndex++];
    SCInputRef RetrievalMode = sc.Input[inputIndex++]; // Closest | Above | Below

    // Subgraphs
    SCSubgraphRef ClosestActiveFVUpHigh = sc.Subgraph[0];
    SCSubgraphRef ClosestActiveFVUpLow  = sc.Subgraph[1];
    SCSubgraphRef ClosestActiveFVUpMid  = sc.Subgraph[2];
    SCSubgraphRef ClosestActiveFVDnHigh = sc.Subgraph[3];
    SCSubgraphRef ClosestActiveFVDnLow  = sc.Subgraph[4];
    SCSubgraphRef ClosestActiveFVDnMid  = sc.Subgraph[5];
    SCSubgraphRef NFVUp = sc.Subgraph[6];
    SCSubgraphRef NFVDn = sc.Subgraph[7];

    // A simple FVG struct
    struct FVG
    {
        int startIndex;
        float low;
        float high;
        float mid;
    };

    // Persistent pointers to maps
    auto* FVGUpArray = reinterpret_cast<std::map<float, FVG>*>(sc.GetPersistentPointer(0));
    auto* FVGDownArray = reinterpret_cast<std::map<float, FVG>*>(sc.GetPersistentPointer(1));

    // Persistent int to avoid double-processing the same bar in streaming mode
    int& lastProcessedBar = sc.GetPersistentInt(0);

    if (sc.SetDefaults)
    {
        sc.GraphName = "FVG closest ATR mult";

        sc.AutoLoop = 1; // let Sierra call the study each bar automatically
        ATRStudy.Name = "ATR study";
        ATRStudy.SetStudyID(1);

        ATRMultiplier.Name = "ATR multiplier";
        ATRMultiplier.SetFloatLimits(1, 200);
        ATRMultiplier.SetFloat(2);

        LocalMinFilter.Name = "Min. of N bars (1 = not used)";
        LocalMinFilter.SetIntLimits(1, 50);
        LocalMinFilter.SetInt(1);

        MaxZoneSize.Name = "Max. zone size";
        MaxZoneSize.SetIntLimits(1, 50);
        MaxZoneSize.SetInt(1);

        RetrievalMode.Name = "Nearest FVG search mode";
        RetrievalMode.SetCustomInputStrings("Closest;Above;Below");
        RetrievalMode.SetCustomInputIndex(0); // Default: Closest

        // Subgraphs visual setup
        ClosestActiveFVUpHigh.Name = "Up High";
        ClosestActiveFVUpHigh.DrawStyle = DRAWSTYLE_DASH;
        ClosestActiveFVUpLow.Name  = "Up Low";
        ClosestActiveFVUpLow.DrawStyle = DRAWSTYLE_DASH;
        ClosestActiveFVUpMid.Name  = "Up Mid";
        ClosestActiveFVUpMid.DrawStyle = DRAWSTYLE_DASH;

        ClosestActiveFVDnHigh.Name = "Dn High";
        ClosestActiveFVDnHigh.DrawStyle = DRAWSTYLE_DASH;
        ClosestActiveFVDnLow.Name  = "Dn Low";
        ClosestActiveFVDnLow.DrawStyle = DRAWSTYLE_DASH;
        ClosestActiveFVDnMid.Name  = "Dn Mid";
        ClosestActiveFVDnMid.DrawStyle = DRAWSTYLE_DASH;

        NFVUp.Name = "N FV up";
        NFVUp.DrawStyle = DRAWSTYLE_HIDDEN;
        NFVDn.Name = "N FV dn";
        NFVDn.DrawStyle = DRAWSTYLE_HIDDEN;

        sc.ScaleRangeType  = SCALE_SAMEASREGION;
        sc.GraphRegion = 0;

        // No further initialization needed here
        return;
    }

    // Avoid processing same index multiple times (streaming)
    const int idx = sc.Index;
    if (lastProcessedBar == idx)
        return;

    // Allocate maps on first use
    if (FVGUpArray == nullptr)
    {
        FVGUpArray = new std::map<float, FVG>();
        sc.SetPersistentPointer(0, FVGUpArray);
    }

    if (FVGDownArray == nullptr)
    {
        FVGDownArray = new std::map<float, FVG>();
        sc.SetPersistentPointer(1, FVGDownArray);
    }

    // Reset at start of trading day or early bars
    if (sc.IsNewTradingDay(idx) || idx < 4)
    {
        FVGUpArray->clear();
        FVGDownArray->clear();

        // reset outputs for cleanliness
        ClosestActiveFVUpHigh[idx] = 0.0f;
        ClosestActiveFVUpLow[idx]  = 0.0f;
        ClosestActiveFVUpMid[idx]  = 0.0f;
        ClosestActiveFVDnHigh[idx] = 0.0f;
        ClosestActiveFVDnLow[idx]  = 0.0f;
        ClosestActiveFVDnMid[idx]  = 0.0f;
        NFVUp[idx] = 0.0f;
        NFVDn[idx] = 0.0f;

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
        FVG up{};
        up.startIndex = idx - 1;
        up.low  = H3;
        up.high = L1;
        up.mid  = (H3 + L1) / 2.0f;
        FVGUpArray->insert_or_assign(H1, up);
    }

    // Insert or update Down FVG keyed by its low (L3).
    if (FVGDn)
    {
        FVG dn{};
        dn.startIndex = idx - 1;
        dn.low  = H1;
        dn.high = L3;
        dn.mid  = (H1 + L3) / 2.0f;
        FVGDownArray->insert_or_assign(L3, dn);
    }

    // --- Remove stale FVGs ---
    // Up is stale if any later low is lower than its low
    for (auto it = FVGUpArray->begin(); it != FVGUpArray->end();)
    {
        if (sc.Low[idx] <= it->second.low)
        {
            auto itNext = std::next(it);
            FVGUpArray->erase(it);
            it = itNext;
        }
        else
        {
            ++it;
        }
    }

    // Down is stale if any later high is higher than its high
    for (auto it = FVGDownArray->begin(); it != FVGDownArray->end(); )
    {
        if (sc.High[idx] >= it->second.high)
        {
            auto itNext = std::next(it);
            FVGDownArray->erase(it);
            it = itNext;
        }
        else
        {
            ++it;
        }
    }

    // --- Retrieve closest Up and Down relative to sc.Open[idx] ---
    const float price = sc.Open[idx];
    const int mode = RetrievalMode.GetIndex(); // 0=closest,1=above,2=below

    // Helper lambda to pick an iterator from above/below based on mode and absolute-close logic
    auto choose_iterator = [&](std::map<float, FVG>* m)->std::map<float, FVG>::iterator
    {
        if (m->empty())
            return m->end();

        auto itAbove = m->lower_bound(price); // first key >= price
        auto itBelow = (itAbove == m->begin()) ? m->end() : std::prev(itAbove);

        if (mode == 1) // Above
        {
            return (itAbove != m->end()) ? itAbove : m->end();
        }
        else if (mode == 2) // Below
        {
            return (itBelow != m->end()) ? itBelow : m->end();
        }
        else // mode == 0, Closest (absolute)
        {
            // If one side missing, pick the other
            if (itAbove == m->end() && itBelow == m->end())
                return m->end();
            if (itAbove == m->end())
                return itBelow;
            if (itBelow == m->end())
                return itAbove;

            // Both sides present: compare absolute distance of keys to price
            float diffA = std::fabs(itAbove->first - price);
            float diffB = std::fabs(itBelow->first - price);
            return (diffA < diffB) ? itAbove : itBelow;
        }
    };

    // Choose Up
    auto chosenUpIt = choose_iterator(FVGUpArray);
    if (chosenUpIt != FVGUpArray->end())
    {
        ClosestActiveFVUpHigh[idx] = chosenUpIt->second.high;
        ClosestActiveFVUpLow[idx]  = chosenUpIt->second.low;
        ClosestActiveFVUpMid[idx]  = chosenUpIt->second.mid;
    }
    else
    {
        ClosestActiveFVUpHigh[idx] = 0.0f;
        ClosestActiveFVUpLow[idx]  = 0.0f;
        ClosestActiveFVUpMid[idx]  = 0.0f;
    }

    // Choose Down
    auto chosenDnIt = choose_iterator(FVGDownArray);
    if (chosenDnIt != FVGDownArray->end())
    {
        ClosestActiveFVDnHigh[idx] = chosenDnIt->second.high;
        ClosestActiveFVDnLow[idx]  = chosenDnIt->second.low;
        ClosestActiveFVDnMid[idx]  = chosenDnIt->second.mid;
    }
    else
    {
        ClosestActiveFVDnHigh[idx] = 0.0f;
        ClosestActiveFVDnLow[idx]  = 0.0f;
        ClosestActiveFVDnMid[idx]  = 0.0f;
    }

    // Counts
    NFVUp[idx] = static_cast<float>(FVGUpArray->size());
    NFVDn[idx] = static_cast<float>(FVGDownArray->size());

    // Done for this bar
    lastProcessedBar = idx;
}