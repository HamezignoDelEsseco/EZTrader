#include "sierrachart.h"

SCSFExport scsf_FVGClosestManual(SCStudyInterfaceRef sc)
{
    // Inputs indices
    int inputIndex = 0;
    SCInputRef GapSizeInTicks = sc.Input[inputIndex++];
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

    const int MIN_START_INDEX = 3; // Need at least 4 bars [0,1,2,3] to check bar idx-3

    // Persistent pointers to maps
    auto* FVGUpArray = reinterpret_cast<std::map<float, FVG>*>(sc.GetPersistentPointer(0));
    auto* FVGDownArray = reinterpret_cast<std::map<float, FVG>*>(sc.GetPersistentPointer(1));

    if (sc.SetDefaults)
    {
        sc.GraphName = "FVG closest manual";

        sc.AutoLoop = 0; // Manual looping for better performance

        GapSizeInTicks.Name = "Gap size in ticks";
        GapSizeInTicks.SetIntLimits(1, 200);
        GapSizeInTicks.SetInt(10);

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

        return;
    }

    // Set data start index - need at least MIN_START_INDEX bars
    sc.DataStartIndex = MIN_START_INDEX;

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

    // Handle full recalculation, study removal, or hiding
    // A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
    // another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
    // Or under other conditions which can cause a full recalculation.
    if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
    {
        // Clear maps on full recalculation
        if (sc.IsFullRecalculation)
        {
            FVGUpArray->clear();
            FVGDownArray->clear();
        }

        // Clear subgraphs for all bars
        for (int idx = 0; idx < sc.ArraySize; idx++)
        {
            ClosestActiveFVUpHigh[idx] = 0.0f;
            ClosestActiveFVUpLow[idx]  = 0.0f;
            ClosestActiveFVUpMid[idx]  = 0.0f;
            ClosestActiveFVDnHigh[idx] = 0.0f;
            ClosestActiveFVDnLow[idx]  = 0.0f;
            ClosestActiveFVDnMid[idx]  = 0.0f;
            NFVUp[idx] = 0.0f;
            NFVDn[idx] = 0.0f;
        }

        // Study is being removed or hidden, nothing more to do
        if (sc.LastCallToFunction || sc.HideStudy)
            return;
    }

    // Calculate gap size in price units once (optimization)
    const float gapTicks = GapSizeInTicks.GetFloat() * sc.TickSize;
    const int mode = RetrievalMode.GetIndex(); // 0=closest,1=above,2=below

    // Helper lambda to pick an iterator from above/below based on mode and absolute-close logic
    // Defined outside loop for efficiency (captures mode and price by reference)
    auto choose_iterator = [&](std::map<float, FVG>* m, float price)->std::map<float, FVG>::iterator
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

    // Process all closed bars from start index to last closed bar
    // Note: sc.ArraySize - 1 is the last closed bar, sc.ArraySize is the current forming bar
    // Loop condition idx < sc.ArraySize ensures we only process closed bars (idx up to ArraySize-1)
    for (int idx = sc.DataStartIndex; idx < sc.ArraySize; idx++)
    {
        // Reset FVG maps at start of new trading day
        if (sc.IsNewTradingDay(idx))
        {
            FVGUpArray->clear();
            FVGDownArray->clear();
        }

        // Convenience locals for bar prices (using idx-1 and idx-3 as per original logic)
        const float L1 = sc.Low[idx - 1];
        const float H1 = sc.High[idx - 1];
        const float L3 = sc.Low[idx - 3];
        const float H3 = sc.High[idx - 3];

        // Determine FVG presence
        const bool FVGUp = H3 < L1 && L1 - H3 >= gapTicks;
        const bool FVGDn = L3 > H1 && L3 - H1 >= gapTicks;

        // Insert or update Up FVG keyed by its high (H1). Store most recent per key.
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

        // Remove stale FVGs - Up is stale if current bar's low is lower than its low
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

        // Remove stale FVGs - Down is stale if current bar's high is higher than its high
        for (auto it = FVGDownArray->begin(); it != FVGDownArray->end();)
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

        // Retrieve closest Up and Down relative to current bar's open
        const float price = sc.Open[idx];

        // Choose Up FVG
        auto chosenUpIt = choose_iterator(FVGUpArray, price);
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

        // Choose Down FVG
        auto chosenDnIt = choose_iterator(FVGDownArray, price);
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
    }
}

