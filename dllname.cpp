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