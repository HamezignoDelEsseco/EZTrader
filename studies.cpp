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

SCSFExport scsf_LollipopSignal(SCStudyInterfaceRef sc) {
    SCInputRef VAPAlertAsk = sc.Input[0];
    SCInputRef InactivityTickOffset = sc.Input[1];


    SCSubgraphRef LollipopSignal = sc.Subgraph[0];
    SCSubgraphRef EZHigh = sc.Subgraph[1];
    SCSubgraphRef EZLow = sc.Subgraph[2];
    SCSubgraphRef ActiveEZIndex = sc.Subgraph[3];

    if (sc.SetDefaults) {
        sc.GraphName = "Lollipop signal";

        VAPAlertAsk.Name = "VAP alert at ask";
        VAPAlertAsk.SetStudyID(1);

        InactivityTickOffset.Name = "Tick offset for inactivity";
        InactivityTickOffset.SetIntLimits(0, 50);
        InactivityTickOffset.SetInt(0);

        LollipopSignal.Name = "Enter signal";
        EZHigh.Name = "EZ high";
        EZLow.Name = "EZ low";
        ActiveEZIndex.Name = "Active EZ index";
        return;
    }

    struct Lollipop
    {
        int startIndex = 0;
        float high = 0;
        float low = 0;
        bool active = false;
    };

    Lollipop* lollipop = reinterpret_cast<Lollipop*>(sc.GetPersistentPointer(0));

    if (lollipop == NULL) {
        lollipop = new Lollipop;
        sc.SetPersistentPointer(0, lollipop);
    }

    sc.DataStartIndex = 1; // We need to look at previous index to make the computation

    for (int idx = sc.DataStartIndex; idx < sc.ArraySize; idx++) {

        // Updating lollipop based on latest data
        if (lollipop->active && sc.Low[idx] - InactivityTickOffset.GetFloat()*sc.TickSize < lollipop->low) {
            lollipop->active = false;
        }

        std::vector<SCFloatArray> Zones(10);
        for (size_t i = 0; i < 5; ++i) {
            // Ask VAP: we need to collect both top and bottom to know whether we have consecutive alerts !!
            sc.GetStudyArrayUsingID(VAPAlertAsk.GetStudyID(), 48 + 2*i, Zones[2*i]);
            sc.GetStudyArrayUsingID(VAPAlertAsk.GetStudyID(), 48 + 2*i + 1, Zones[2*i + 1]);

            if (Zones[2*i][idx-1] != 0 && Zones[2*i + 1][idx-1] != 0) {
                if (Zones[2*i][idx-1] == sc.Low[idx-1]) {
                    lollipop->startIndex = idx-1;
                    lollipop->active = true;
                    lollipop->high = Zones[2*i + 1][idx-1];
                    lollipop->low = Zones[2*i][idx-1];
                    break;
                }
            }
        }
        if (lollipop->active) {
            EZHigh[idx] = lollipop->high;
            EZLow[idx] = lollipop->low;
            LollipopSignal[idx] = 1;
            ActiveEZIndex[idx] = lollipop->startIndex;
        }
    }
}