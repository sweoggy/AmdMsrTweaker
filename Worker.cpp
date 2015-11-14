/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <locale>
#include <thread>
#include "Worker.h"
#include "StringUtils.h"
#include "WinRing0.h"

using std::cerr;
using std::endl;
using std::min;
using std::max;
using std::string;
using std::tolower;
using std::vector;

static void SplitPair(string& left, string& right, const string& str, char delimiter)
{
	const size_t i = str.find(delimiter);

	left = str.substr(0, i);

	if (i == string::npos)
		right.clear();
	else
		right = str.substr(i + 1);

}

bool Worker::ParseParams(int argc, const char* argv[])
{
	const Info& info = *_info;

	PStateInfo psi;
	psi.Multi = psi.VID = psi.NBVID = -1;
	psi.NBPState = -1;

	NBPStateInfo nbpsi;
	nbpsi.Multi = 1.0;
	nbpsi.VID = -1;

	for (int i = 0; i < info.NumPStates; i++)
	{
		_pStates.push_back(psi);
		_pStates.back().Index = i;
	}
	for (int i = 0; i < info.NumNBPStates; i++)
	{
		_nbPStates.push_back(nbpsi);
		_nbPStates.back().Index = i;
	}

	for (int i = 1; i < argc; i++)
	{
		const string param(argv[i]);

		string key, value;
		SplitPair(key, value, param, '=');

		if (value.empty())
		{
			if (param.length() >= 2 && tolower(param[0]) == 'p')
			{
				const int index = atoi(param.c_str() + 1);
				if (index >= 0 && index < info.NumPStates)
				{
					_pState = index;
					continue;
				}
			}
		}
		else
		{
			if (key.length() >= 2 && tolower(key[0]) == 'p')
			{
				const int index = atoi(key.c_str() + 1);
				if (index >= 0 && index < info.NumPStates)
				{
					string multi, vid;
					SplitPair(multi, vid, value, '@');

					if (!multi.empty())
						_pStates[index].Multi = info.multiScaleFactor * atof(multi.c_str());
					if (!vid.empty())
						_pStates[index].VID = info.EncodeVID(atof(vid.c_str()));

					continue;
				}
			}

			if (key.length() >= 5 && _strnicmp(key.c_str(), "NB_P", 4) == 0)
			{
				const int index = atoi(key.c_str() + 4);
				if (index >= 0 && index < info.NumNBPStates)
				{
					string multi, vid;
					SplitPair(multi, vid, value, '@');

					if (!multi.empty())
						_nbPStates[index].Multi = atof(multi.c_str());
					if (!vid.empty())
						_nbPStates[index].VID = info.EncodeVID(atof(vid.c_str()));

					continue;
				}
			}

			if (_stricmp(key.c_str(), "NB_low") == 0)
			{
				const int index = atoi(value.c_str());

				int j = 0;
				for (; j < min(index, info.NumPStates); j++)
					_pStates[j].NBPState = 0;
				for (; j < info.NumPStates; j++)
					_pStates[j].NBPState = 1;

				continue;
			}

			if (_stricmp(key.c_str(), "Turbo") == 0)
			{
				const int flag = atoi(value.c_str());
				if (flag == 0 || flag == 1)
				{
					_turbo = flag;
					continue;
				}
			}

			if( _stricmp( key.c_str(), "BoostEnAllCores" ) == 0 )
			{
				const int flag = atoi( value.c_str() );
				if( flag == 0 || flag == 1 )
				{
					_boostEnAllCores = flag;
					continue;
				}
			}

			if( _stricmp( key.c_str(), "IgnoreBoostThresh" ) == 0 )
			{
				const int flag = atoi( value.c_str() );
				if( flag == 0 || flag == 1 )
				{
					_ignoreBoostThresh = flag;
					continue;
				}
			}

			if (_stricmp(key.c_str(), "APM") == 0)
			{
				const int flag = atoi(value.c_str());
				if (flag == 0 || flag == 1)
				{
					_apm = flag;
					continue;
				}
			}

			if (_stricmp(key.c_str(), "NbPsi0Vid") == 0)
			{
				if (!value.empty())
					_NbPsi0Vid_VID = info.EncodeVID(atof(value.c_str()));

				continue;
			}
		}

		cerr << "ERROR: invalid parameter " << param.c_str() << endl;
		return false;
	}

	return true;
}


static bool ContainsChanges(const PStateInfo& info)
{
	return (info.Multi >= 0 || info.VID >= 0 || info.NBVID >= 0 || info.NBPState >= 0);
}
static bool ContainsChanges(const NBPStateInfo& info)
{
	return (info.Multi >= 0 || info.VID >= 0);
}

static void SwitchTo(int logicalCPUIndex)
{
	const HANDLE hThread = GetCurrentThread();
	SetThreadAffinityMask(hThread, (DWORD_PTR)1 << logicalCPUIndex);
}

void Worker::ApplyChanges()
{
	const Info& info = *_info;
#ifdef _DEBUG
	unsigned short sleepDelay = 0;
	string sleepText = ", waiting for " + std::to_string(sleepDelay) + "seconds.";
#endif

	// Apply NB P-states
#ifdef _DEBUG
	cerr << "Applying NB P-states" << sleepText << endl;
	std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
#endif
	//Changing NB P-states causes system hang on Carrizo (Model == 0x60), disable for now
	if (info.Family == 0x15 && info.Model != 0x60)
	{
		for (int i = 0; i < _nbPStates.size(); i++)
		{
			const NBPStateInfo& nbpsi = _nbPStates[i];
			if (ContainsChanges(nbpsi))
				info.WriteNBPState(nbpsi);
		}
	}
	else if (info.Family == 0x10 && (_nbPStates[0].VID >= 0 || _nbPStates[1].VID >= 0))
	{
		for (int i = 0; i < _pStates.size(); i++)
		{
			PStateInfo& psi = _pStates[i];

			const int nbPState = (psi.NBPState >= 0 ? psi.NBPState : info.ReadPState(i).NBPState);
			const NBPStateInfo& nbpsi = _nbPStates[nbPState];

			if (nbpsi.VID >= 0)
				psi.NBVID = nbpsi.VID;
		}
	}
#ifdef _DEBUG
	if (_nbPStates.size() > 0 && (info.Family == 0x15 && info.Model != 0x60))
	{
		cerr << "NB P-states successfully applied" << endl;
	}
	else if (info.Family == 0x15 && info.Model == 0x60)
	{
		cerr << "Modifying P-states on Carrizo is disabled for now (causes system hang)" << endl;
	}
	else
	{
		cerr << "No P-states applied (non were specified)" << endl;
	}
#endif

	// Applying turbo
#ifdef _DEBUG
	cerr << "Configuring turbo and APM (if supported)" << sleepText << endl;
	std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
#endif
	if (_turbo >= 0 && info.IsBoostSupported)
	{
		info.SetBoostSource(_turbo == 1);
	}
	if (_boostEnAllCores >= 0 && info.BoostEnAllCores != -1)
	{
		info.SetBoostEnAllCores(_boostEnAllCores);
	}
	if (_ignoreBoostThresh >= 0 && info.IgnoreBoostThresh != -1)
	{
		info.SetIgnoreBoostThresh(_ignoreBoostThresh);
	}
	if (_apm >= 0 && info.Family == 0x15)
	{
		info.SetAPM(_apm == 1);
	}

	if (_NbPsi0Vid_VID >= 0 && info.Family == 0x15)
	{
#ifdef _DEBUG
		cerr << "Writing NbPsi0Vid" << sleepText << endl;
		std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
#endif
		info.WriteNbPsi0Vid(_NbPsi0Vid_VID);
	}

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	const int numLogicalCPUs = sysInfo.dwNumberOfProcessors;

	// switch to the highest thread priority (we do not want to get interrupted often)
	const HANDLE hProcess = GetCurrentProcess();
	const HANDLE hThread = GetCurrentThread();
	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
	SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);

	// Write P-states, perform one iteration in each logical core
#ifdef _DEBUG
	if (_pStates.size() > 0)
	{
		cerr << "Writing P-states" << sleepText << endl;
		std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
	}
#endif
	for (int j = 0; j < numLogicalCPUs; j++)
	{
		SwitchTo(j);

		for (int i = 0; i < _pStates.size(); i++)
		{
			const PStateInfo& psi = _pStates[i];
			if (ContainsChanges(psi))
				info.WritePState(psi);
		}

		if (_turbo >= 0 && info.IsBoostSupported)
			info.SetCPBDis(_turbo == 1);
	}

	// Set P-states, perform one iteration in each logical core
#ifdef _DEBUG
	if (ContainsChanges(_pStates[info.GetCurrentPState()]))
	{
		cerr << "Settings P-states" << sleepText << endl;
		std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
	}
#endif
	for (int j = 0; j < numLogicalCPUs; j++)
	{
		SwitchTo(j);

		const int currentPState = info.GetCurrentPState();
		const int newPState = (_pState >= 0 ? _pState : currentPState);

		if (newPState != currentPState)
			info.SetCurrentPState(newPState);
		else
		{
			if (ContainsChanges(_pStates[currentPState]))
			{
				const int tempPState = (currentPState == info.NumPStates - 1 ? 0 : info.NumPStates - 1);
				info.SetCurrentPState(tempPState);
				Sleep(1);
				info.SetCurrentPState(currentPState);
			}
		}
	}

	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
	SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);

#ifdef _DEBUG
	cerr << "Successfully executed all steps, exiting in 5 seconds" << endl;
	std::this_thread::sleep_for(std::chrono::seconds(5));
#endif
}
