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
	int MemClkFreqVal;
	int FastMstateDis;
};

struct iGPUPStateInfo
{
	int Index;
	int Valid; // derived from StateValid in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	double Freq; // in MHz, derived from LclkDivider in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	int VID; // derived from VID in D0F0xBC_x3FD[8C:00:step14] LCLK DPM Control 0
	int NBPState;
};


class Info
{
public:

	int Family;
	int Model;
	int NumCores;
	int NumPStates; // derived from HwPstateMaxVal[2:0]
	int NumNBPStates; // derived from NbPstateMaxVal[1:0]
	int NumMemPStates; // derived from MemPstateCap
	int NBPStateHiCPU; // NB P-States used for CPU-only load, derived from NbPstateHi[1:0]
	int NBPStateLoCPU; // NB P-States used for CPU-only load, derived from NbPstateLo[1:0]
	int NBPStateHiGPU; // NB P-States used for GPU load
	int NBPStateLoGPU; // NB P-States used for GPU load
	
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
		, NumNBPStates(2) // we have at least 2 NB P-States (more for family 0x15)
		, NumMemPStates(1) // we have at least 1 Mem P-States (more for family 0x15)
		, NBPStateHiCPU(0)
		, NBPStateLoCPU(0)
		, NBPStateHiGPU(0)
		, NBPStateLoGPU(0)
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

	int GetCurrentPState() const;
	void SetCurrentPState(int index) const;

	double DecodeVID(int vid) const;
	int EncodeVID(double vid) const;

private:

	double DecodeMulti(int fid, int did) const;
	void EncodeMulti(double multi, int& fid, int& did) const;

};
