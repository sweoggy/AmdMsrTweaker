/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#pragma once


struct PStateInfo
{
	int Index;    // hardware index
	double Multi; // internal one for 100 MHz reference
	int VID;
	int NBPState;
	int NBVID; // family 0x10 only
};

struct NBPStateInfo
{
	int Index;
	int Enabled;
	double Multi; // for 200 MHz reference
	int VID;
	int MemPState;
};

struct MemPStateInfo
{
	int Index;
	double MemClkFreq; // in MHz, derived from MemClkFreq[4:0]/M1MemClkFreq[4:0]
};

struct iGPUPStateInfo
{
	int Index;
	int StateValid; // derived from StateValid in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	int LclkDivider; // derived from LclkDivider in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	double Freq; // in MHz
	int VID; // derived from VID in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	int LowVoltageReqThreshold; // derived from LowVoltageReqThreshold in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
};


class Info
{
public:

	int Family;
	int Model;
	int NumCores;

	int NumPStates; // derived from HwPstateMaxVal[2:0]
	int PsiVidEn; // derived from PsiVidEn in D18F3xA0 Power Control Miscellaneous
	int PsiVid; // derived from PsiVid[6:0] and PsiVid[7] in D18F3xA0 Power Control Miscellaneous

	int NumNBPStates; // derived from NbPstateMaxVal[1:0]
	int NBPStateHi; // derived from NbPstateHi[1:0]
	int NBPStateLo; // derived from NbPstateLo[1:0]
	int NBPStateHiCPU; // NB P-States used for CPU-only load, derived from NbPstateHi[1:0]
	int NBPStateLoCPU; // NB P-States used for CPU-only load, derived from NbPstateLo[1:0]
	int NBPStateHiGPU; // NB P-States used for GPU load
	int NBPStateLoGPU; // NB P-States used for GPU load
	int NbPstateDis; // derived from NbPstateDis in MSRC001_0071 COFVID Status
	int NbPstateGnbSlowDis; // derived from NbPstateGnbSlowDis in D18F5x170 Northbridge P-state Control
	int StartupNbPstate; // derived from StartupNbPstate in D18F5x174 Northbridge P-state Status
	int NbPsi0VidEn; // derived from NbPsi0VidEn in D18F5x17C Miscellaneous Voltages
	int NbPsi0Vid; // derived from NbPsi0Vid in D18F5x17C Miscellaneous Voltages
	

	int NumMemPStates; // derived from MemPstateCap
	int MemClkFreqVal; // derived from MemClkFreqVal in D18F2x94_dct[3:0] DRAM Configuration High
	int FastMstateDis; // derived from FastMstateDis in D18F2x2E0_dct[3:0] Memory P-state Control and Status
	
	int GpuEnabled; // GpuEnabled = (D1F0x00!=FFFF_FFFFh)
	int SwGfxDis; // derived fromSwGfxDis in D18F5x178 Northbridge Fusion Configuration
	int ForceIntGfxDisable; // derived from ForceIntGfxDisable in D0F0x7C IOC Configuration Control

	int LclkDpmEn; // LclkDpmEn in D0F0xBC_x3FDC8 SMU_LCLK_DPM_CNTL
	int VoltageChgEn; // VoltageChgEn in D0F0xBC_x3FDC8 SMU_LCLK_DPM_CNTL
	int LclkDpmBootState; // LclkDpmBootState in D0F0xBC_x3FDC8 SMU_LCLK_DPM_CNTL
	
	double MinMulti, MaxMulti; // internal ones for 100 MHz reference
	double MaxSoftwareMulti; // for software (i.e., non-boost) P-states
	double MinVID, MaxVID;
	double VIDStep;
	double multiScaleFactor;

	bool IsBoostSupported;
	bool IsBoostEnabled;
	bool IsBoostLocked;
	bool IsDynMemPStateChgEnabled; // derived from MemPstateDis
	int NumBoostStates;

	int CurPState;
	int CurNBPState;
	int CurMemPState;

	Info()
		: Family(0)
		, Model(0)
		, NumCores(0)

		, NumPStates(0)
		, PsiVidEn(0)
		, PsiVid(0)

		, NumNBPStates(2) // we have at least 2 NB P-States (more for family 0x15)
		, NBPStateHi(0)
		, NBPStateLo(0)
		, NBPStateHiCPU(0)
		, NBPStateLoCPU(0)
		, NBPStateHiGPU(0)
		, NBPStateLoGPU(0)
		, NbPstateDis(0)
		, NbPstateGnbSlowDis(0)
		, StartupNbPstate(0)
		, NbPsi0VidEn(0)
		, NbPsi0Vid(0)

		, NumMemPStates(1) // we have at least 1 Mem P-States (more for family 0x15)
		, MemClkFreqVal(0)
		, FastMstateDis(0)

		, GpuEnabled(0)
		, SwGfxDis(0)

		, LclkDpmEn(0)
		, VoltageChgEn(0)
		, LclkDpmBootState(0)

		, MinMulti(0.0), MaxMulti(0.0)
		, MaxSoftwareMulti(0.0)
		, MinVID(0.0), MaxVID(0.0)
		, VIDStep(0.0125) //default step for pre SVI2 platforms
		, multiScaleFactor(1.0) //default for 100MHz REFCLK
		, IsBoostSupported(false)
		, IsBoostEnabled(false)
		, IsBoostLocked(false)
		, IsDynMemPStateChgEnabled(false)
		, NumBoostStates(0)
		, CurPState(0)
		, CurNBPState(0)
		, CurMemPState(0)
	{
	}

	bool Initialize();

	PStateInfo ReadPState(int index) const;
	void WritePState(const PStateInfo& info) const;

	NBPStateInfo ReadNBPState(int index) const;
	void WriteNBPState(const NBPStateInfo& info) const;

	MemPStateInfo ReadMemPState(int index) const;

	iGPUPStateInfo ReadiGPUPState(int index) const;

	void SetCPBDis(bool enabled) const;
	void SetBoostSource(bool enabled) const;
	void SetAPM(bool enabled) const;

	void WriteNbPsi0Vid(const int VID) const;

	int GetCurrentPState() const;
	void SetCurrentPState(int index) const;

	double DecodeVID(int vid) const;
	int EncodeVID(double vid) const;

private:

	double DecodeMulti(int fid, int did) const;
	void EncodeMulti(double multi, int& fid, int& did) const;

};
