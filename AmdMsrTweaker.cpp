/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <iostream>
#include <conio.h>
#include "Info.h"
#include "Worker.h"
#include "WinRing0.h"

using std::cout;
using std::cerr;
using std::endl;


void PrintInfo(const Info& info);
void WaitForKey();


/// <summary>Entry point for the program.</summary>
int main(int argc, const char* argv[])
{
	// initialize WinRing0
	if (!InitializeOls() || GetDllStatus() != 0)
	{
		cerr << "ERROR: WinRing0 initialization failed" << endl;
		DeinitializeOls();

		return 1;
	}

	try
	{
		Info info;
		if (!info.Initialize())
		{
			cout << "ERROR: unsupported CPU" << endl;
			DeinitializeOls();
			WaitForKey();
			return 2;
		}

		if (argc > 1)
		{
			Worker worker(info);

			if (!worker.ParseParams(argc, argv))
			{
				DeinitializeOls();
				WaitForKey();
				return 3;
			}

			worker.ApplyChanges();
		}
		else
		{
			PrintInfo(info);
			WaitForKey();
		}
	}
	catch (const std::exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		DeinitializeOls();
		WaitForKey();
		return 10;
	}

	DeinitializeOls();

	return 0;
}


void PrintInfo(const Info& info)
{
	cout << endl;
	cout << "AmdMsrTweaker v2.0" << endl;
	cout << endl;

	cout << ".:. General" << endl << "---" << endl;
	cout << "  AMD family 0x" << std::hex << info.Family << ", model 0x" << info.Model << std::dec << " CPU, " << info.NumCores << " cores" << endl;
	cout << "  Default reference clock: " << info.multiScaleFactor * 100 << " MHz" << endl;
	cout << "  Available multipliers: " << (info.MinMulti / info.multiScaleFactor) << " .. " << (info.MaxSoftwareMulti / info.multiScaleFactor) << endl;
	cout << "  Available voltage IDs: " << info.MinVID << " .. " << info.MaxVID << " (" << info.VIDStep << " steps)" << endl;
	cout << endl;

	cout << ".:. Turbo" << endl << "---" << endl;
	if (!info.IsBoostSupported)
		cout << "  not supported" << endl;
	else
	{
		cout << "  " << (info.IsBoostEnabled ? "enabled" : "disabled") << endl;
		cout << "  " << (info.IsBoostLocked ? "locked" : "unlocked") << endl;

		if( info.Family == 0x12 )
		{
			cout << "  BoostEnAllCores: " << info.BoostEnAllCores << endl;
			cout << "  IgnoreBoostThresh: " << info.IgnoreBoostThresh << endl;
		}

		if (info.MaxMulti != info.MaxSoftwareMulti)
			cout << "  Max multiplier: " << (info.MaxMulti / info.multiScaleFactor) << endl;
	}
	cout << endl;

	cout << ".:. P-states" << endl << "---" << endl;
	cout << "  " << info.NumPStates << " of " << (info.Family == 0x10 ? 5 : 8) << " enabled (P0 .. P" << (info.NumPStates - 1) << ")" << endl;

	if (info.IsBoostSupported && info.NumBoostStates > 0)
	{
		cout << "  Turbo P-states:";
		for (int i = 0; i < info.NumBoostStates; i++)
			cout << " P" << i;
		cout << endl;
	}

	cout << "  ---" << endl;

	for (int i = 0; i < info.NumPStates; i++)
	{
		const PStateInfo pi = info.ReadPState(i);

		cout << "  P" << i << ": " << (pi.Multi / info.multiScaleFactor) << "x at " << info.DecodeVID(pi.VID) << "V" << endl;

		if (pi.NBPState >= 0)
		{
			cout << "      NorthBridge in NB_P" << pi.NBPState;
			if (pi.NBVID >= 0)
				cout << " at " << info.DecodeVID(pi.NBVID) << "V";
			cout << endl;
		}
	}

	cout << "  * PsiVidEn = " << info.PsiVidEn << ", PsiVid = " << info.DecodeVID(info.PsiVid) << " V" << endl;

	if (info.Family == 0x15)
	{
		cout << "  ---" << endl;

		int memPStateCnt[2] = { 0, 0 };

		for (int i = 0; i < info.NumNBPStates; i++)
		{
			const NBPStateInfo pi = info.ReadNBPState(i);
			if (pi.Enabled)
			{
				cout << "  NB_P" << i << ": " << pi.Multi << "x at " << info.DecodeVID(pi.VID) << "V";
				if (i == info.NBPStateHi)
					cout << " [NBPStateHi]";
				if (i == info.NBPStateLo)
					cout << " [NBPStateLo]";
				//if (i == info.NBPStateHiGPU)
				//	cout << " [GPU Hi]";
				//if (i == info.NBPStateLoGPU)
				//	cout << " [GPU Lo]";
				cout << endl;

				if (pi.MemPState >= 0)
				{
					cout << "         Memory in M" << pi.MemPState << endl;
					memPStateCnt[pi.MemPState]++;
				}
			}
		}

		cout << "  * NbPstateDis = " << info.NbPstateDis << ", NbPstateGnbSlowDis = " << info.NbPstateGnbSlowDis << ", StartupNbPstate = " << info.StartupNbPstate << endl;
		cout << "  * NbPsi0VidEn = " << info.NbPsi0VidEn << ", NbPsi0Vid = " << info.DecodeVID(info.NbPsi0Vid) << " V" << endl;
		cout << "  * NBPStateHiCPU = " << info.NBPStateHiCPU << ", NBPStateLoCPU = " << info.NBPStateLoCPU << endl;
		cout << "  * NBPStateHiGPU = " << info.NBPStateHiGPU << ", NBPStateLoGPU = " << info.NBPStateLoGPU << endl;

		cout << "  ---" << endl;

		for (int i = 0; i < info.NumMemPStates; i++)
		{
			if (memPStateCnt[i] > 0)
			{
				const MemPStateInfo pi = info.ReadMemPState(i);
				cout << "  M" << i << ": " << pi.MemClkFreq << " MHz" << endl;
			}
			if (!info.IsDynMemPStateChgEnabled)
				break;
		}

		cout << "  * MemClkFreqVal(M0) = " << info.MemClkFreqVal << ", FastMstateDis(M1) = " << info.FastMstateDis << endl;

		cout << "  ---" << endl;

		for (int i = 0; i < 8; i++)
		{
			const iGPUPStateInfo pi = info.ReadiGPUPState(i);
			cout << "  GPU_P" << i << ": StateValid = " << pi.StateValid << ", LclkDivider = " << pi.LclkDivider << ", VID = " << info.DecodeVID(pi.VID) << " V";
			if (pi.StateValid == 1)
				cout << " [VALID]";
			cout << endl;
		}

		cout << "  * GpuEnabled = " << info.GpuEnabled << ", SwGfxDis = " << info.SwGfxDis << ", ForceIntGfxDisable = " << info.ForceIntGfxDisable << endl;
		cout << "  * LclkDpmEn = " << info.LclkDpmEn << ", VoltageChgEn = " << info.VoltageChgEn << ", LclkDpmBootState = " << info.LclkDpmBootState << endl;
	}
	else if( info.Family == 0x12 )
	{
		cout << endl;

		cout << ".:. RAM" << endl << "---" << endl;

		DRAMInfo sticks[2];
		for( int i = 0; i <= 1; ++i )
		{
			sticks[i] = info.ReadDRAMInfo( i );
		}
		cout << "  Freq: " << sticks[0].Freq << "," << sticks[1].Freq << endl;
		cout << "  tCL:  " << sticks[0].tCL  << "," << sticks[1].tCL  << endl;
		cout << "  tRCD: " << sticks[0].tRCD << "," << sticks[1].tRCD << endl;
		cout << "  tRP:  " << sticks[0].tRP  << "," << sticks[1].tRP  << endl;
		cout << "  tRAS: " << sticks[0].tRAS << "," << sticks[1].tRAS << endl;
		cout << "  tRC:  " << sticks[0].tRC  << "," << sticks[1].tRC  << endl;
		cout << "  tRTP: " << sticks[0].tRTP << "," << sticks[1].tRTP << endl;
		cout << "  tRRD: " << sticks[0].tRRD << "," << sticks[1].tRRD << endl;
		cout << "  tWTR: " << sticks[0].tWTR << "," << sticks[1].tWTR << endl;
		cout << "  tWR:  " << sticks[0].tWR  << "," << sticks[1].tWR  << endl;
		cout << "  tCWL: " << sticks[0].tCWL << "," << sticks[1].tCWL << endl;
		cout << "  CR:   " << sticks[0].CR   << "," << sticks[1].CR   << endl;
	}
}


void WaitForKey()
{
	cout << endl << "Press any key to exit... ";
	_getch();
	cout << endl;
}
