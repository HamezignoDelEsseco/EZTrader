#include "sierrachart.h"

SCSFExport scsf_gcFairValueGap(SCStudyInterfaceRef sc)
{
	enum Subgraph
	{ FVGUpCount
	, FVGDnCount
	, FVGUp_01_H
	, FVGUp_01_L
	, FVGUp_02_H
	, FVGUp_02_L
	, FVGUp_03_H
	, FVGUp_03_L
	, FVGUp_04_H
	, FVGUp_04_L
	, FVGUp_05_H
	, FVGUp_05_L
	, FVGUp_06_H
	, FVGUp_06_L
	, FVGUp_07_H
	, FVGUp_07_L
	, FVGUp_08_H
	, FVGUp_08_L
	, FVGUp_09_H
	, FVGUp_09_L
	, FVGUp_10_H
	, FVGUp_10_L
	, FVGUp_11_H
	, FVGUp_11_L
	, FVGUp_12_H
	, FVGUp_12_L
	, FVGUp_13_H
	, FVGUp_13_L
	, FVGUp_14_H
	, FVGUp_14_L
	, FVGDn_01_H
	, FVGDn_01_L
	, FVGDn_02_H
	, FVGDn_02_L
	, FVGDn_03_H
	, FVGDn_03_L
	, FVGDn_04_H
	, FVGDn_04_L
	, FVGDn_05_H
	, FVGDn_05_L
	, FVGDn_06_H
	, FVGDn_06_L
	, FVGDn_07_H
	, FVGDn_07_L
	, FVGDn_08_H
	, FVGDn_08_L
	, FVGDn_09_H
	, FVGDn_09_L
	, FVGDn_10_H
	, FVGDn_10_L
	, FVGDn_11_H
	, FVGDn_11_L
	, FVGDn_12_H
	, FVGDn_12_L
	, FVGDn_13_H
	, FVGDn_13_L
	, FVGDn_14_H
	, FVGDn_14_L
	};

	// Input Index
	int SCInputIndex = 0;

	// FVG Up Settings
	SCInputRef Input_FVGUpEnabled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineWidth = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineStyle = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpLineColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpFillColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpTransparencyLevel = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpDrawMidline = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpExtendRight = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpHideWhenFilled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpAllowCopyToOtherCharts = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGUpMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// FVG Down Settings
	SCInputRef Input_FVGDnEnabled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineWidth = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineStyle = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnLineColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnFillColor = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnTransparencyLevel = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnDrawMidline = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnExtendRight = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnHideWhenFilled = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnAllowCopyToOtherCharts = sc.Input[SCInputIndex++];
	SCInputRef Input_FVGDnMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// General Settings
	SCInputRef Input_FVGMaxBarLookback = sc.Input[SCInputIndex++];
	SCInputRef Input_ToggleSubgraphs = sc.Input[SCInputIndex++];

	// Tick Size for Subgraphs - For trailing stops/targets/etc...
	SCInputRef Input_SubgraphFVGUpMinGapSizeInTicks = sc.Input[SCInputIndex++];
	SCInputRef Input_SubgraphFVGDnMinGapSizeInTicks = sc.Input[SCInputIndex++];

	// Input Graph used to feed 3rd candle of FVG to determine if FVG is drawn or not
	SCInputRef Input_UseAdditionalFVGCandleData = sc.Input[SCInputIndex++];
	SCInputRef Input_AdditionalFVGCandleData = sc.Input[SCInputIndex++];

	const int MIN_START_INDEX = 2; // Need at least 3 bars [0,1,2]

	struct FVGRectangle
	{
		int LineNumber;
		int ToolBeginIndex;
		float ToolBeginValue;
		int ToolEndIndex;
		float ToolEndValue;
		bool FVGEnded;
		bool FVGUp; // If not up, then it's down
		unsigned int AddAsUserDrawnDrawing;
	};

	struct HLForBarIndex
	{
		int Index;
		float High;
		float Low;
	};

	// Structs for FVG's and Bar High/Low/Index Data
	std::vector<FVGRectangle>* FVGRectangles = reinterpret_cast<std::vector<FVGRectangle>*>(sc.GetPersistentPointer(0));
	std::vector<HLForBarIndex> HLForBarIndexes;

	// Tool Line Unique Number Start Point
	int UniqueLineNumber = 8675309; // Jenny Jenny!

	if (sc.SetDefaults)
	{
		sc.GraphName = "GC: Fair Value Gap";
		SCString StudyDescription;
		sc.StudyDescription = StudyDescription.AppendFormat(
			"<br><br>This Study Draws ICT Fair Value Gaps"
		);
		sc.GraphRegion = 0;
		sc.AutoLoop = 0;

		sc.CalculationPrecedence = LOW_PREC_LEVEL; // Needed for addional study inputs...

		// FVG Up
		Input_FVGUpEnabled.Name = "FVG Up: Enabled";
		Input_FVGUpEnabled.SetDescription("Draw FVG Up Gaps");
		Input_FVGUpEnabled.SetYesNo(1);

		Input_FVGUpLineWidth.Name = "FVG Up: Line Width";
		Input_FVGUpLineWidth.SetDescription("Width of FVG Rectangle Border");
		Input_FVGUpLineWidth.SetInt(1);

		Input_FVGUpLineStyle.Name = "FVG Up: Line Style";
		Input_FVGUpLineStyle.SetDescription("Style of FVG Rectangle Border");
		Input_FVGUpLineStyle.SetCustomInputStrings("Solid; Dash; Dot; Dash-Dot; Dash-Dot-Dot; Alternate");
		Input_FVGUpLineStyle.SetCustomInputIndex(0);

		Input_FVGUpLineColor.Name = "FVG Up: Line Color";
		Input_FVGUpLineColor.SetColor(RGB(13, 166, 240));
		Input_FVGUpLineColor.SetDescription("Color of FVG Rectangle Border");

		Input_FVGUpFillColor.Name = "FVG Up: Fill Color";
		Input_FVGUpFillColor.SetDescription("Fill Color Used for FVG Rectangle");
		Input_FVGUpFillColor.SetColor(RGB(13, 166, 240));

		Input_FVGUpTransparencyLevel.Name = "FVG Up: Transparency Level";
		Input_FVGUpTransparencyLevel.SetDescription("Transparency Level for FVG Rectangle Fill");
		Input_FVGUpTransparencyLevel.SetInt(65);
		Input_FVGUpTransparencyLevel.SetIntLimits(0, 100);

		Input_FVGUpDrawMidline.Name = "FVG Up: Draw Midline (Set Line Width to 1 or Higher)";
		Input_FVGUpDrawMidline.SetDescription("Draw Midline for FVG Rectangle. Requires Line Width of 1 or Higher.");
		Input_FVGUpDrawMidline.SetYesNo(0);

		Input_FVGUpExtendRight.Name = "FVG Up: Extend Right";
		Input_FVGUpExtendRight.SetDescription("Extend FVG Rectangle to Right of Chart Until Filled");
		Input_FVGUpExtendRight.SetYesNo(1);

		Input_FVGUpHideWhenFilled.Name = "FVG Up: Hide When Filled";
		Input_FVGUpHideWhenFilled.SetDescription("Hide FVG Rectangle when Gap is Filled");
		Input_FVGUpHideWhenFilled.SetYesNo(1);

		Input_FVGUpAllowCopyToOtherCharts.Name = "FVG Up: Allow Copy To Other Charts";
		Input_FVGUpAllowCopyToOtherCharts.SetDescription("Allow the FVG Rectangles to be Copied to Other Charts");
		Input_FVGUpAllowCopyToOtherCharts.SetYesNo(1);

		Input_FVGUpMinGapSizeInTicks.Name = "FVG Up: Minimum Gap Size in Ticks";
		Input_FVGUpMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_FVGUpMinGapSizeInTicks.SetInt(1);
		Input_FVGUpMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		// FVG Down
		Input_FVGDnEnabled.Name = "FVG Down: Enabled";
		Input_FVGDnEnabled.SetDescription("Draw FVG Down Gaps");
		Input_FVGDnEnabled.SetYesNo(1);

		Input_FVGDnLineWidth.Name = "FVG Down: Line Width";
		Input_FVGDnLineWidth.SetDescription("Width of Rectangle Border");
		Input_FVGDnLineWidth.SetInt(1);

		Input_FVGDnLineStyle.Name = "FVG Down: Line Style";
		Input_FVGDnLineStyle.SetDescription("Style of FVG Rectangle Border");
		Input_FVGDnLineStyle.SetCustomInputStrings("Solid; Dash; Dot; Dash-Dot; Dash-Dot-Dot; Alternate");
		Input_FVGDnLineStyle.SetCustomInputIndex(0);

		Input_FVGDnLineColor.Name = "FVG Down: Line Color";
		Input_FVGDnLineColor.SetDescription("Color of Rectangle Border");
		Input_FVGDnLineColor.SetColor(RGB(255, 128, 128));

		Input_FVGDnFillColor.Name = "FVG Down: Fill Color";
		Input_FVGDnFillColor.SetDescription("Fill Color Used for Rectangle");
		Input_FVGDnFillColor.SetColor(RGB(255, 128, 128));

		Input_FVGDnTransparencyLevel.Name = "FVG Down: Transparency Level";
		Input_FVGDnTransparencyLevel.SetDescription("Transparency Level for Rectangle Fill");
		Input_FVGDnTransparencyLevel.SetInt(65);
		Input_FVGDnTransparencyLevel.SetIntLimits(0, 100);

		Input_FVGDnDrawMidline.Name = "FVG Down: Draw Midline (Set Line Width to 1 or Higher)";
		Input_FVGDnDrawMidline.SetDescription("Draw Midline for FVG Rectangle. Requires Line Width of 1 or Higher.");
		Input_FVGDnDrawMidline.SetYesNo(0);

		Input_FVGDnExtendRight.Name = "FVG Down: Extend Right";
		Input_FVGDnExtendRight.SetDescription("Extend FVG Rectangle to Right of Chart Until Filled");
		Input_FVGDnExtendRight.SetYesNo(1);

		Input_FVGDnHideWhenFilled.Name = "FVG Down: Hide When Filled";
		Input_FVGDnHideWhenFilled.SetDescription("Hide Rectangle when Gap is Filled");
		Input_FVGDnHideWhenFilled.SetYesNo(1);

		Input_FVGDnAllowCopyToOtherCharts.Name = "FVG Down: Allow Copy To Other Charts";
		Input_FVGDnAllowCopyToOtherCharts.SetDescription("Allow the FVG Rectangles to be Copied to Other Charts");
		Input_FVGDnAllowCopyToOtherCharts.SetYesNo(1);

		Input_FVGDnMinGapSizeInTicks.Name = "FVG Down: Minimum Gap Size in Ticks";
		Input_FVGDnMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_FVGDnMinGapSizeInTicks.SetInt(1);
		Input_FVGDnMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		// General settings
		Input_FVGMaxBarLookback.Name = "Maximum Bar Lookback (0 = ALL)";
		Input_FVGMaxBarLookback.SetDescription("This Sets the Maximum Number of Bars to Process");
		Input_FVGMaxBarLookback.SetInt(400);
		Input_FVGMaxBarLookback.SetIntLimits(0, MAX_STUDY_LENGTH);

		// Tick Size for Subgraphs - For trailing stops/targets/etc...
		Input_SubgraphFVGUpMinGapSizeInTicks.Name = "Subgraph FVG Up: Minimum Gap Size in Ticks";
		Input_SubgraphFVGUpMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_SubgraphFVGUpMinGapSizeInTicks.SetInt(1);
		Input_SubgraphFVGUpMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		Input_SubgraphFVGDnMinGapSizeInTicks.Name = "Subgraph FVG Down: Minimum Gap Size in Ticks";
		Input_SubgraphFVGDnMinGapSizeInTicks.SetDescription("Only Process Gaps if greater or equal to Specified Gap Size");
		Input_SubgraphFVGDnMinGapSizeInTicks.SetInt(1);
		Input_SubgraphFVGDnMinGapSizeInTicks.SetIntLimits(1, INT_MAX);

		Input_UseAdditionalFVGCandleData.Name = "Use Additional Study Data for FVG";
		Input_UseAdditionalFVGCandleData.SetDescription("When enabled this uses additional data to determine if FVG should be drawn");
		Input_UseAdditionalFVGCandleData.SetYesNo(0);

		Input_AdditionalFVGCandleData.Name = "Additional Study Data for FVG [True/False Compare]";
		Input_AdditionalFVGCandleData.SetDescription("This checks the result of the specified study at the same index as the candle that would create the FVG (middle candle). Non-Zero values are considered True. Zero is considered False. If True then the FVG is drawn. Otherwise it's ignored.");
		Input_AdditionalFVGCandleData.SetStudySubgraphValues(0, 0);

		// Subgraphs - FVG Counters
		sc.Subgraph[Subgraph::FVGUpCount].Name = "FVG Up Count";
		sc.Subgraph[Subgraph::FVGUpCount].PrimaryColor = RGB(13, 166, 240);
		sc.Subgraph[Subgraph::FVGUpCount].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUpCount].DrawZeros = false;

		sc.Subgraph[Subgraph::FVGDnCount].Name = "FVG Down Count";
		sc.Subgraph[Subgraph::FVGDnCount].PrimaryColor = RGB(255, 128, 128);
		sc.Subgraph[Subgraph::FVGDnCount].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDnCount].DrawZeros = false;

		// Subgraphs - FVG Ups
		#pragma region Subgraphs - FVG Ups
		sc.Subgraph[Subgraph::FVGUp_01_H].Name = "FVG Up 01 High";
		sc.Subgraph[Subgraph::FVGUp_01_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_01_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_01_H].PrimaryColor = RGB(13, 166, 240);
		sc.Subgraph[Subgraph::FVGUp_01_L].Name = "FVG Up 01 Low";
		sc.Subgraph[Subgraph::FVGUp_01_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_01_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_01_L].PrimaryColor = RGB(13, 166, 240);

		sc.Subgraph[Subgraph::FVGUp_02_H].Name = "FVG Up 02 High";
		sc.Subgraph[Subgraph::FVGUp_02_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_02_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_02_H].PrimaryColor = RGB(31, 172, 230);
		sc.Subgraph[Subgraph::FVGUp_02_L].Name = "FVG Up 02 Low";
		sc.Subgraph[Subgraph::FVGUp_02_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_02_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_02_L].PrimaryColor = RGB(31, 172, 230);

		sc.Subgraph[Subgraph::FVGUp_03_H].Name = "FVG Up 03 High";
		sc.Subgraph[Subgraph::FVGUp_03_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_03_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_03_H].PrimaryColor = RGB(49, 179, 220);
		sc.Subgraph[Subgraph::FVGUp_03_L].Name = "FVG Up 03 Low";
		sc.Subgraph[Subgraph::FVGUp_03_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_03_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_03_L].PrimaryColor = RGB(49, 179, 220);

		sc.Subgraph[Subgraph::FVGUp_04_H].Name = "FVG Up 04 High";
		sc.Subgraph[Subgraph::FVGUp_04_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_04_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_04_H].PrimaryColor = RGB(68, 185, 210);
		sc.Subgraph[Subgraph::FVGUp_04_L].Name = "FVG Up 04 Low";
		sc.Subgraph[Subgraph::FVGUp_04_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_04_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_04_L].PrimaryColor = RGB(68, 185, 210);

		sc.Subgraph[Subgraph::FVGUp_05_H].Name = "FVG Up 05 High";
		sc.Subgraph[Subgraph::FVGUp_05_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_05_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_05_H].PrimaryColor = RGB(86, 192, 200);
		sc.Subgraph[Subgraph::FVGUp_05_L].Name = "FVG Up 05 Low";
		sc.Subgraph[Subgraph::FVGUp_05_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_05_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_05_L].PrimaryColor = RGB(86, 192, 200);

		sc.Subgraph[Subgraph::FVGUp_06_H].Name = "FVG Up 06 High";
		sc.Subgraph[Subgraph::FVGUp_06_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_06_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_06_H].PrimaryColor = RGB(104, 198, 190);
		sc.Subgraph[Subgraph::FVGUp_06_L].Name = "FVG Up 06 Low";
		sc.Subgraph[Subgraph::FVGUp_06_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_06_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_06_L].PrimaryColor = RGB(104, 198, 190);

		sc.Subgraph[Subgraph::FVGUp_07_H].Name = "FVG Up 07 High";
		sc.Subgraph[Subgraph::FVGUp_07_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_07_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_07_H].PrimaryColor = RGB(122, 205, 180);
		sc.Subgraph[Subgraph::FVGUp_07_L].Name = "FVG Up 07 Low";
		sc.Subgraph[Subgraph::FVGUp_07_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_07_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_07_L].PrimaryColor = RGB(122, 205, 180);

		sc.Subgraph[Subgraph::FVGUp_08_H].Name = "FVG Up 08 High";
		sc.Subgraph[Subgraph::FVGUp_08_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_08_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_08_H].PrimaryColor = RGB(141, 211, 170);
		sc.Subgraph[Subgraph::FVGUp_08_L].Name = "FVG Up 08 Low";
		sc.Subgraph[Subgraph::FVGUp_08_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_08_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_08_L].PrimaryColor = RGB(141, 211, 170);

		sc.Subgraph[Subgraph::FVGUp_09_H].Name = "FVG Up 09 High";
		sc.Subgraph[Subgraph::FVGUp_09_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_09_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_09_H].PrimaryColor = RGB(159, 218, 160);
		sc.Subgraph[Subgraph::FVGUp_09_L].Name = "FVG Up 09 Low";
		sc.Subgraph[Subgraph::FVGUp_09_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_09_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_09_L].PrimaryColor = RGB(159, 218, 160);

		sc.Subgraph[Subgraph::FVGUp_10_H].Name = "FVG Up 10 High";
		sc.Subgraph[Subgraph::FVGUp_10_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_10_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_10_H].PrimaryColor = RGB(177, 224, 150);
		sc.Subgraph[Subgraph::FVGUp_10_L].Name = "FVG Up 10 Low";
		sc.Subgraph[Subgraph::FVGUp_10_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_10_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_10_L].PrimaryColor = RGB(177, 224, 150);

		sc.Subgraph[Subgraph::FVGUp_11_H].Name = "FVG Up 11 High";
		sc.Subgraph[Subgraph::FVGUp_11_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_11_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_11_H].PrimaryColor = RGB(195, 231, 140);
		sc.Subgraph[Subgraph::FVGUp_11_L].Name = "FVG Up 11 Low";
		sc.Subgraph[Subgraph::FVGUp_11_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_11_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_11_L].PrimaryColor = RGB(195, 231, 140);

		sc.Subgraph[Subgraph::FVGUp_12_H].Name = "FVG Up 12 High";
		sc.Subgraph[Subgraph::FVGUp_12_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_12_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_12_H].PrimaryColor = RGB(214, 237, 130);
		sc.Subgraph[Subgraph::FVGUp_12_L].Name = "FVG Up 12 Low";
		sc.Subgraph[Subgraph::FVGUp_12_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_12_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_12_L].PrimaryColor = RGB(214, 237, 130);

		sc.Subgraph[Subgraph::FVGUp_13_H].Name = "FVG Up 13 High";
		sc.Subgraph[Subgraph::FVGUp_13_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_13_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_13_H].PrimaryColor = RGB(232, 244, 120);
		sc.Subgraph[Subgraph::FVGUp_13_L].Name = "FVG Up 13 Low";
		sc.Subgraph[Subgraph::FVGUp_13_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_13_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_13_L].PrimaryColor = RGB(232, 244, 120);

		sc.Subgraph[Subgraph::FVGUp_14_H].Name = "FVG Up 14 High";
		sc.Subgraph[Subgraph::FVGUp_14_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_14_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_14_H].PrimaryColor = RGB(250, 250, 110);
		sc.Subgraph[Subgraph::FVGUp_14_L].Name = "FVG Up 14 Low";
		sc.Subgraph[Subgraph::FVGUp_14_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGUp_14_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGUp_14_L].PrimaryColor = RGB(250, 250, 110);
		#pragma endregion

		// Subgraphs - FVG Downs
		#pragma region Subgraphs - FVG Downs
		sc.Subgraph[Subgraph::FVGDn_01_H].Name = "FVG Down 01 High";
		sc.Subgraph[Subgraph::FVGDn_01_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_01_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_01_H].PrimaryColor = RGB(255, 128, 128);
		sc.Subgraph[Subgraph::FVGDn_01_L].Name = "FVG Down 01 Low";
		sc.Subgraph[Subgraph::FVGDn_01_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_01_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_01_L].PrimaryColor = RGB(255, 128, 128);

		sc.Subgraph[Subgraph::FVGDn_02_H].Name = "FVG Down 02 High";
		sc.Subgraph[Subgraph::FVGDn_02_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_02_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_02_H].PrimaryColor = RGB(255, 137, 127);
		sc.Subgraph[Subgraph::FVGDn_02_L].Name = "FVG Down 02 Low";
		sc.Subgraph[Subgraph::FVGDn_02_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_02_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_02_L].PrimaryColor = RGB(255, 137, 127);

		sc.Subgraph[Subgraph::FVGDn_03_H].Name = "FVG Down 03 High";
		sc.Subgraph[Subgraph::FVGDn_03_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_03_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_03_H].PrimaryColor = RGB(254, 147, 125);
		sc.Subgraph[Subgraph::FVGDn_03_L].Name = "FVG Down 03 Low";
		sc.Subgraph[Subgraph::FVGDn_03_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_03_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_03_L].PrimaryColor = RGB(254, 147, 125);

		sc.Subgraph[Subgraph::FVGDn_04_H].Name = "FVG Down 04 High";
		sc.Subgraph[Subgraph::FVGDn_04_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_04_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_04_H].PrimaryColor = RGB(254, 156, 124);
		sc.Subgraph[Subgraph::FVGDn_04_L].Name = "FVG Down 04 Low";
		sc.Subgraph[Subgraph::FVGDn_04_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_04_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_04_L].PrimaryColor = RGB(254, 156, 124);

		sc.Subgraph[Subgraph::FVGDn_05_H].Name = "FVG Down 05 High";
		sc.Subgraph[Subgraph::FVGDn_05_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_05_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_05_H].PrimaryColor = RGB(253, 166, 122);
		sc.Subgraph[Subgraph::FVGDn_05_L].Name = "FVG Down 05 Low";
		sc.Subgraph[Subgraph::FVGDn_05_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_05_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_05_L].PrimaryColor = RGB(253, 166, 122);

		sc.Subgraph[Subgraph::FVGDn_06_H].Name = "FVG Down 06 High";
		sc.Subgraph[Subgraph::FVGDn_06_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_06_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_06_H].PrimaryColor = RGB(253, 175, 121);
		sc.Subgraph[Subgraph::FVGDn_06_L].Name = "FVG Down 06 Low";
		sc.Subgraph[Subgraph::FVGDn_06_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_06_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_06_L].PrimaryColor = RGB(253, 175, 121);

		sc.Subgraph[Subgraph::FVGDn_07_H].Name = "FVG Down 07 High";
		sc.Subgraph[Subgraph::FVGDn_07_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_07_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_07_H].PrimaryColor = RGB(253, 184, 120);
		sc.Subgraph[Subgraph::FVGDn_07_L].Name = "FVG Down 07 Low";
		sc.Subgraph[Subgraph::FVGDn_07_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_07_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_07_L].PrimaryColor = RGB(253, 184, 120);

		sc.Subgraph[Subgraph::FVGDn_08_H].Name = "FVG Down 08 High";
		sc.Subgraph[Subgraph::FVGDn_08_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_08_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_08_H].PrimaryColor = RGB(252, 194, 118);
		sc.Subgraph[Subgraph::FVGDn_08_L].Name = "FVG Down 08 Low";
		sc.Subgraph[Subgraph::FVGDn_08_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_08_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_08_L].PrimaryColor = RGB(252, 194, 118);

		sc.Subgraph[Subgraph::FVGDn_09_H].Name = "FVG Down 09 High";
		sc.Subgraph[Subgraph::FVGDn_09_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_09_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_09_H].PrimaryColor = RGB(252, 203, 117);
		sc.Subgraph[Subgraph::FVGDn_09_L].Name = "FVG Down 09 Low";
		sc.Subgraph[Subgraph::FVGDn_09_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_09_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_09_L].PrimaryColor = RGB(252, 203, 117);

		sc.Subgraph[Subgraph::FVGDn_10_H].Name = "FVG Down 10 High";
		sc.Subgraph[Subgraph::FVGDn_10_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_10_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_10_H].PrimaryColor = RGB(252, 212, 116);
		sc.Subgraph[Subgraph::FVGDn_10_L].Name = "FVG Down 10 Low";
		sc.Subgraph[Subgraph::FVGDn_10_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_10_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_10_L].PrimaryColor = RGB(252, 212, 116);

		sc.Subgraph[Subgraph::FVGDn_11_H].Name = "FVG Down 11 High";
		sc.Subgraph[Subgraph::FVGDn_11_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_11_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_11_H].PrimaryColor = RGB(251, 222, 114);
		sc.Subgraph[Subgraph::FVGDn_11_L].Name = "FVG Down 11 Low";
		sc.Subgraph[Subgraph::FVGDn_11_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_11_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_11_L].PrimaryColor = RGB(251, 222, 114);

		sc.Subgraph[Subgraph::FVGDn_12_H].Name = "FVG Down 12 High";
		sc.Subgraph[Subgraph::FVGDn_12_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_12_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_12_H].PrimaryColor = RGB(251, 231, 113);
		sc.Subgraph[Subgraph::FVGDn_12_L].Name = "FVG Down 12 Low";
		sc.Subgraph[Subgraph::FVGDn_12_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_12_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_12_L].PrimaryColor = RGB(251, 231, 113);

		sc.Subgraph[Subgraph::FVGDn_13_H].Name = "FVG Down 13 High";
		sc.Subgraph[Subgraph::FVGDn_13_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_13_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_13_H].PrimaryColor = RGB(250, 241, 111);
		sc.Subgraph[Subgraph::FVGDn_13_L].Name = "FVG Down 13 Low";
		sc.Subgraph[Subgraph::FVGDn_13_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_13_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_13_L].PrimaryColor = RGB(250, 241, 111);

		sc.Subgraph[Subgraph::FVGDn_14_H].Name = "FVG Down 14 High";
		sc.Subgraph[Subgraph::FVGDn_14_H].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_14_H].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_14_H].PrimaryColor = RGB(250, 250, 110);
		sc.Subgraph[Subgraph::FVGDn_14_L].Name = "FVG Down 14 Low";
		sc.Subgraph[Subgraph::FVGDn_14_L].DrawStyle = DRAWSTYLE_IGNORE;
		sc.Subgraph[Subgraph::FVGDn_14_L].LineWidth = 1;
		sc.Subgraph[Subgraph::FVGDn_14_L].PrimaryColor = RGB(250, 250, 110);
		#pragma endregion

		return;
	}

	// See if we are capping max bars to check back
	if (Input_FVGMaxBarLookback.GetInt() == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - Input_FVGMaxBarLookback.GetInt() + MIN_START_INDEX;

	if (FVGRectangles == NULL) {
		// Array of FVGRectangle structs
		FVGRectangles = new std::vector<FVGRectangle>;
		sc.SetPersistentPointer(0, FVGRectangles);
	}

	// A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		// On a full recalculation non-user drawn advanced custom study drawings are automatically deleted
		// So need to manually remove the User type drawings. Same with hiding study, if user type, need to manually remove them
		for (int i = 0; i < FVGRectangles->size(); i++)
		{
			if (FVGRectangles->at(i).AddAsUserDrawnDrawing)
				sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, FVGRectangles->at(i).LineNumber);
		}
		// Drawings removed, now clear to avoid re-drawing them again
		FVGRectangles->clear();

		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}

	// Get line style for FVG types
	SubgraphLineStyles FVGUpLineStyle = (SubgraphLineStyles)Input_FVGUpLineStyle.GetIndex();
	SubgraphLineStyles FVGDnLineStyle = (SubgraphLineStyles)Input_FVGDnLineStyle.GetIndex();

	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (FVGRectangles != NULL)
		FVGRectangles->clear();

	// Min Gap Tick Size for FVG's
	float FVGUpMinTickSize = float(Input_FVGUpMinGapSizeInTicks.GetInt()) * sc.TickSize;
	float FVGDnMinTickSize = float(Input_FVGDnMinGapSizeInTicks.GetInt()) * sc.TickSize;

	// Min Gap Tick Size for Subgraphs
	float SubgraphFVGUpMinTickSize = float(Input_SubgraphFVGUpMinGapSizeInTicks.GetInt()) * sc.TickSize;
	float SubgraphFVGDnMinTickSize = float(Input_SubgraphFVGDnMinGapSizeInTicks.GetInt()) * sc.TickSize;

	// Additional Candle Data
	SCFloatArray AdditionalFVGCandleData;

	// Loop through bars and process FVG's
	// 1 is the current bar that is closed
	// 3 is the 3rd bar back from current bar
	for (int BarIndex = sc.DataStartIndex; BarIndex < sc.ArraySize - 1; BarIndex++)
	{
		// Store HL for each bar index. May need to revisit to see if better way
		HLForBarIndex TmpHLForBarIndex = {
			BarIndex,
			sc.High[BarIndex],
			sc.Low[BarIndex]
		};
		HLForBarIndexes.push_back(TmpHLForBarIndex);

		// Additional Data
		if (Input_UseAdditionalFVGCandleData.GetYesNo())
			sc.GetStudyArrayUsingID(Input_AdditionalFVGCandleData.GetStudyID(), Input_AdditionalFVGCandleData.GetSubgraphIndex(), AdditionalFVGCandleData);

		//
		// H1, H3, L1, L3 logic borrowed from this indicator
		// https://www.tradingview.com/script/u8mKo7pb-muh-gap-FAIR-VALUE-GAP-FINDER/
		float L1 = sc.Low[BarIndex];
		float H1 = sc.High[BarIndex];
		float L3 = sc.Low[BarIndex - 2];
		float H3 = sc.High[BarIndex - 2];

		bool FVGUp = (H3 < L1) && (L1 - H3 >= FVGUpMinTickSize);
		bool FVGDn = (L3 > H1) && (L3 - H1 >= FVGDnMinTickSize);

		if (Input_UseAdditionalFVGCandleData.GetYesNo())
		{
			FVGUp = FVGUp && AdditionalFVGCandleData[BarIndex - 1];
			FVGDn = FVGDn && AdditionalFVGCandleData[BarIndex - 1];
		}

		// Store the FVG Up
		if (FVGUp && Input_FVGUpEnabled.GetYesNo())
		{
			FVGRectangle TmpUpRect = {
				UniqueLineNumber + BarIndex, // Tool.LineNumber
				BarIndex - 2, // Tool.BeginIndex
				H3, // Tool.BeginValue
				BarIndex,// Tool.EndIndex
				L1, // Tool.EndValue
				false, // FVGEnded
				true, // FVGUp
				Input_FVGUpAllowCopyToOtherCharts.GetYesNo() // AddAsUserDrawnDrawing
			};
			FVGRectangles->insert(FVGRectangles->end(), TmpUpRect);
		}

		// Store the FVG Dn
		if (FVGDn && Input_FVGDnEnabled.GetYesNo())
		{
			FVGRectangle TmpDnRect = {
				UniqueLineNumber + BarIndex, // Tool.LineNumber
				BarIndex - 2, // Tool.BeginIndex
				L3, // Tool.BeginValue
				BarIndex,// Tool.EndIndex
				H1, // Tool.EndValue
				false, // FVGEnded
				false,  // FVGUp
				Input_FVGDnAllowCopyToOtherCharts.GetYesNo() // AddAsUserDrawnDrawing
			};
			FVGRectangles->insert(FVGRectangles->end(), TmpDnRect);
		}
	}

	// Count open FVG's for Up/Down
	int FVGUpCounter = 0;
	int FVGDnCounter = 0;

	// Subgraph offsets for counter
	int FVGUp_H_SubgraphOffset = 2;
	int FVGUp_L_SubgraphOffset = 3;
	int FVGDn_H_SubgraphOffset = 30;
	int FVGDn_L_SubgraphOffset = 31;

	for (int i = FVGRectangles->size() - 1; i >= 0; i--)
	{
		s_UseTool Tool;
		Tool.Clear();
		Tool.ChartNumber = sc.ChartNumber;
		Tool.LineNumber = FVGRectangles->at(i).LineNumber;
		Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		Tool.AddMethod = UTAM_ADD_OR_ADJUST;

		Tool.BeginIndex = FVGRectangles->at(i).ToolBeginIndex;
		Tool.BeginValue = FVGRectangles->at(i).ToolBeginValue;
		Tool.EndIndex = FVGRectangles->at(i).ToolEndIndex;
		Tool.EndValue = FVGRectangles->at(i).ToolEndValue;

		if (FVGRectangles->at(i).FVGUp)
		{
			// FVG Up
			Tool.Color = Input_FVGUpLineColor.GetColor();
			Tool.SecondaryColor = Input_FVGUpFillColor.GetColor();
			Tool.LineWidth = Input_FVGUpLineWidth.GetInt();
			Tool.LineStyle = FVGUpLineStyle;
			Tool.TransparencyLevel = Input_FVGUpTransparencyLevel.GetInt();
			Tool.DrawMidline = Input_FVGUpDrawMidline.GetYesNo();

			// If we want to allow this to show up on other charts, need to set it to user drawing
			Tool.AddAsUserDrawnDrawing = FVGRectangles->at(i).AddAsUserDrawnDrawing;
			Tool.AllowCopyToOtherCharts = FVGRectangles->at(i).AddAsUserDrawnDrawing;

			// In our High/Low Bar Index, check if we have an equal or lower Low than the FVG
			// If True, then this FVG will end at that index if extending Right
			auto it = find_if(
				HLForBarIndexes.begin(),
				HLForBarIndexes.end(),
				[=](HLForBarIndex& HLIndex)
				{
					return HLIndex.Index > FVGRectangles->at(i).ToolEndIndex && HLIndex.Low <= FVGRectangles->at(i).ToolBeginValue;
				}
			);
			// If true, we have an end index for this FVG and it hasn't yet been flagged as ended
			if (it != HLForBarIndexes.end() && !FVGRectangles->at(i).FVGEnded)
			{
				// FVG has ended, so then see if we want to show it or not...
				if (Input_FVGUpHideWhenFilled.GetYesNo())
					Tool.HideDrawing = 1;
				else
					Tool.HideDrawing = 0;

				// If here, we have an ending FVG that we want to show... Now need to see which EndIndex to use
				if (Input_FVGUpExtendRight.GetYesNo())
					Tool.EndIndex = it->Index; // Extending, so show it based on where we found the equal/lower bar low index

				// Flag this as ended now
				FVGRectangles->at(i).FVGEnded = true;
			}
			else
			{
				// If here, the FVG has not ended, so set to last closed bar if extending
				if (Input_FVGUpExtendRight.GetYesNo())
					Tool.EndIndex = sc.ArraySize - 1;
			}

			// Update Subgraphs
			bool IsSubgraphFVGUp = (FVGRectangles->at(i).ToolEndValue - FVGRectangles->at(i).ToolBeginValue) >= SubgraphFVGUpMinTickSize;
			bool FVGNotEnded = !FVGRectangles->at(i).FVGEnded;

			if (FVGNotEnded && IsSubgraphFVGUp)
			{
				FVGUpCounter++;
				sc.Subgraph[Subgraph::FVGUpCount][sc.ArraySize - 1] = (float)FVGUpCounter;

				if (FVGUpCounter == 1)
				{
					sc.Subgraph[Subgraph::FVGUp_01_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_01_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 2)
				{
					sc.Subgraph[Subgraph::FVGUp_02_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_02_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 3)
				{
					sc.Subgraph[Subgraph::FVGUp_03_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_03_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 4)
				{
					sc.Subgraph[Subgraph::FVGUp_04_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_04_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 5)
				{
					sc.Subgraph[Subgraph::FVGUp_05_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_05_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 6)
				{
					sc.Subgraph[Subgraph::FVGUp_06_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_06_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 7)
				{
					sc.Subgraph[Subgraph::FVGUp_07_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_07_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 8)
				{
					sc.Subgraph[Subgraph::FVGUp_08_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_08_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 9)
				{
					sc.Subgraph[Subgraph::FVGUp_09_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_09_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 10)
				{
					sc.Subgraph[Subgraph::FVGUp_10_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_10_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 11)
				{
					sc.Subgraph[Subgraph::FVGUp_11_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_11_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 12)
				{
					sc.Subgraph[Subgraph::FVGUp_12_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_12_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 13)
				{
					sc.Subgraph[Subgraph::FVGUp_13_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_13_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
				else if (FVGUpCounter == 14)
				{
					sc.Subgraph[Subgraph::FVGUp_14_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
					sc.Subgraph[Subgraph::FVGUp_14_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
				}
			}
		}
		else
		{
			// FVG Down
			Tool.Color = Input_FVGDnLineColor.GetColor();
			Tool.SecondaryColor = Input_FVGDnFillColor.GetColor();
			Tool.LineWidth = Input_FVGDnLineWidth.GetInt();
			Tool.LineStyle = FVGDnLineStyle;
			Tool.TransparencyLevel = Input_FVGDnTransparencyLevel.GetInt();
			Tool.AddMethod = UTAM_ADD_OR_ADJUST;
			Tool.DrawMidline = Input_FVGDnDrawMidline.GetYesNo();

			// If we want to allow this to show up on other charts, need to set it to user drawing
			Tool.AddAsUserDrawnDrawing = FVGRectangles->at(i).AddAsUserDrawnDrawing;
			Tool.AllowCopyToOtherCharts = FVGRectangles->at(i).AddAsUserDrawnDrawing;

			// In our High/Low Bar Index, check if we have an equal or higher High than the FVG
			// If True, then this FVG will end at that index if extending Right
			auto it = find_if(
				HLForBarIndexes.begin(),
				HLForBarIndexes.end(),
				[=](HLForBarIndex& HLIndex)
				{
					return HLIndex.Index > FVGRectangles->at(i).ToolEndIndex && HLIndex.High >= FVGRectangles->at(i).ToolBeginValue;
				}
			);
			// If true, we have an end index for this FVG and it hasn't yet been flagged as ended
			if (it != HLForBarIndexes.end() && !FVGRectangles->at(i).FVGEnded)
			{
				// FVG has ended, so then see if we want to show it or not...
				if (Input_FVGDnHideWhenFilled.GetYesNo())
					Tool.HideDrawing = 1;
				else
					Tool.HideDrawing = 0;

				// If here, we have an ending FVG that we want to show... Now need to see which EndIndex to use
				if (Input_FVGDnExtendRight.GetYesNo())
					Tool.EndIndex = it->Index; // Extending, so show it based on where we found the equal/lower bar low index

				// Flag this as ended now
				FVGRectangles->at(i).FVGEnded = true;
			}
			else
			{
				// If here, the FVG has not ended, so set to last closed bar if extending
				if (Input_FVGDnExtendRight.GetYesNo())
					Tool.EndIndex = sc.ArraySize - 1;
			}

			// Update Subgraphs
			bool IsSubgraphFVGDn = (FVGRectangles->at(i).ToolBeginValue - FVGRectangles->at(i).ToolEndValue) >= SubgraphFVGDnMinTickSize;
			bool FVGNotEnded = !FVGRectangles->at(i).FVGEnded;

			if (FVGNotEnded && IsSubgraphFVGDn)
			{
				FVGDnCounter++;
				sc.Subgraph[Subgraph::FVGDnCount][sc.ArraySize - 1] = (float)FVGDnCounter;

				if (FVGDnCounter == 1)
				{
					sc.Subgraph[Subgraph::FVGDn_01_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_01_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 2)
				{
					sc.Subgraph[Subgraph::FVGDn_02_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_02_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 3)
				{
					sc.Subgraph[Subgraph::FVGDn_03_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_03_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 4)
				{
					sc.Subgraph[Subgraph::FVGDn_04_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_04_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 5)
				{
					sc.Subgraph[Subgraph::FVGDn_05_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_05_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 6)
				{
					sc.Subgraph[Subgraph::FVGDn_06_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_06_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 7)
				{
					sc.Subgraph[Subgraph::FVGDn_07_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_07_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 8)
				{
					sc.Subgraph[Subgraph::FVGDn_08_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_08_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 9)
				{
					sc.Subgraph[Subgraph::FVGDn_09_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_09_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 10)
				{
					sc.Subgraph[Subgraph::FVGDn_10_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_10_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 11)
				{
					sc.Subgraph[Subgraph::FVGDn_11_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_11_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 12)
				{
					sc.Subgraph[Subgraph::FVGDn_12_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_12_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 13)
				{
					sc.Subgraph[Subgraph::FVGDn_13_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_13_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
				else if (FVGDnCounter == 14)
				{
					sc.Subgraph[Subgraph::FVGDn_14_H][sc.ArraySize - 1] = FVGRectangles->at(i).ToolBeginValue;
					sc.Subgraph[Subgraph::FVGDn_14_L][sc.ArraySize - 1] = FVGRectangles->at(i).ToolEndValue;
				}
			}
		}
		sc.UseTool(Tool);
	}
}