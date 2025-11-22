#include "sierrachart.h"

SCDLLName("eztraderMaster")

SCSFExport scsf_DummyStudy(SCStudyInterfaceRef sc) {

    SCSubgraphRef TradeSignal = sc.Subgraph[0];

    if (sc.SetDefaults) {
        sc.AutoLoop = 1;

        sc.GraphName = "Dummy study";
        TradeSignal.Name = "Enter signal";
    }
    TradeSignal[sc.Index] = 1;
}

SCSFExport scsf_LollipopSignalAsk(SCStudyInterfaceRef sc) {
    // Takes all lollipops at a local minimum
    // Signal stays active until a bar closes above the zone OR until there's a new signal
    int inputIndex = 0;
    SCInputRef VAPAlertAsk = sc.Input[inputIndex++];
    SCInputRef InactivityTickOffset = sc.Input[inputIndex++];
    SCInputRef LocalMinFilter = sc.Input[inputIndex++];
    SCInputRef MaxZoneSize = sc.Input[inputIndex++];


    SCSubgraphRef EZHigh = sc.Subgraph[0];
    SCSubgraphRef EZLow = sc.Subgraph[1];

    struct Lollipop
    {
        bool active = false;
        float startIndex = 0;
        float high = 0;
        float low = 0;
        float regionSizeInTicks = 0;
    };

    Lollipop* latestlollipop = reinterpret_cast<Lollipop*>(sc.GetPersistentPointer(0));

    if (sc.SetDefaults) {
        sc.GraphName = "Lollipop signal ask";

        sc.AutoLoop = 1;
        VAPAlertAsk.Name = "VAP alert at ask";
        VAPAlertAsk.SetStudyID(1);

        InactivityTickOffset.Name = "Tick offset for inactivity";
        InactivityTickOffset.SetIntLimits(0, 50);
        InactivityTickOffset.SetInt(0);

        LocalMinFilter.Name = "Min. of N bars (1 = not used)";
        LocalMinFilter.SetIntLimits(1, 50);
        LocalMinFilter.SetInt(1);

        MaxZoneSize.Name = "Max. zone size";
        MaxZoneSize.SetIntLimits(1, 50);
        MaxZoneSize.SetInt(1);

        EZHigh.Name = "EZ high";
        EZHigh.DrawStyle = DRAWSTYLE_DASH;

        EZLow.Name = "EZ low";
        EZLow.DrawStyle = DRAWSTYLE_DASH;

        sc.ScaleRangeType  = SCALE_SAMEASREGION;
        sc.GraphRegion = 0;
    }

    const int idx = sc.Index;
    const int idxPrev = sc.Index - 1;


    if (latestlollipop == NULL) {
        latestlollipop = new Lollipop;
        sc.SetPersistentPointer(0, latestlollipop);
    }


    if (idx == 0 || sc.IsNewTradingDay(idx))
    {
        // This will be hit in particular during full recalc -> reset all
        latestlollipop->active = false;
        latestlollipop->startIndex = 0;
        latestlollipop->high = 0;
        latestlollipop->low = 0;
        latestlollipop->regionSizeInTicks = 0;
        return;
    }

    // if (latestlollipop->active && sc.Close[idxPrev] > latestlollipop->high)
    // {
    //     // Shutting down the lollipop if a bar closes above its high
    //     latestlollipop->active = false;
    //     latestlollipop->startIndex = 0;
    //     latestlollipop->high = 0;
    //     latestlollipop->low = 0;
    //     latestlollipop->regionSizeInTicks = 0;
    // }

    EZHigh[idx] = latestlollipop->high;
    EZLow[idx] = latestlollipop->low;

    const float lowestOfNBars = sc.GetLowest(sc.BaseData[SC_LOW], idxPrev, LocalMinFilter.GetInt());

    if (sc.Low[idxPrev] != lowestOfNBars){return;} // Only updating the lollipops at local min / max


    std::vector<SCFloatArray> Zones(10);
    for (size_t i = 0; i < 5; ++i)
    {
        // Ask VAP: we need to collect both top and bottom to know whether we have consecutive alerts !!
        sc.GetStudyArrayUsingID(VAPAlertAsk.GetStudyID(), 48 + 2 * i, Zones[2 * i]);
        sc.GetStudyArrayUsingID(VAPAlertAsk.GetStudyID(), 48 + 2 * i + 1, Zones[2 * i + 1]);

        if (Zones[2 * i][idxPrev] != 0 && Zones[2 * i + 1][idxPrev] != 0)
        {
            if (Zones[2 * i][idxPrev] == sc.Low[idxPrev])
            {
                float zoneSize = (Zones[2 * i + 1][idxPrev] - Zones[2 * i][idxPrev]) / sc.TickSize;
                if (zoneSize > MaxZoneSize.GetFloat()){return;} // Filtering by maximum zone size
                latestlollipop->startIndex = idxPrev;
                latestlollipop->active = true;
                latestlollipop->high = Zones[2 * i + 1][idxPrev];
                latestlollipop->low = Zones[2 * i][idxPrev];
                latestlollipop->regionSizeInTicks = zoneSize;
                break;
            }
        }
    }

}