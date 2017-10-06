// TestPlugin.cpp : Defines the initialization routines for the DLL.
//

#define REAPERAPI_IMPLEMENT

using namespace std;

#include "TestPlugin.h"

#include "../reaper_plugin.h"
#include "../reaper_plugin_functions.h"
#include <fstream>

#define REG_FUNC(x,y) (*(void **)&x) = y->GetFunc(#x)

int fooCommandId;

static void customMenuHook(const char* menuString, HMENU menu, int flag)
{
	if (strcmp(menuString, "Track control panel context") == 0) {
		// the track context menu
		if (flag == 0) {
			// add our menu item
			int itemCount = GetMenuItemCount(menu);
			MENUITEMINFO itemInfo = {};
			itemInfo.cbSize = sizeof(itemInfo);
			itemInfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
			itemInfo.wID = fooCommandId;
			itemInfo.fType = MFT_STRING;
			itemInfo.dwTypeData = "Perform Foo On Tracks";
			InsertMenuItem(menu, itemCount++, true, &itemInfo);
		}
	}
}

static bool commandFilter(int commandId, int flag)
{
	if (commandId == fooCommandId) {
		// perform the action
		logNote("MyFunction");
		int trackCount = CountTracks(0);

		char buffer[255];
		for (int i = 0; i < trackCount; i++)
		{
			MediaTrack* track = GetTrack(0, i);
			GetTrackName(track, buffer, 255);
			int color = GetTrackColor(track);

			/*if (GetTrackReceiveName(track, 1, buffer, 255))
			{
				logNote("Receive Buffer: ");
				logNote(buffer);
			}*/
			logNote(buffer);
			
			int sendCount = GetTrackNumSends(track, -1);
			double val = GetMediaTrackInfo_Value(track, "I_RECINPUT");
			logNote("track done");
		}

		char strCount[2];
		_itoa_s(trackCount, strCount, 10);
		logNote(strCount);
		// return true to indicate that this was our command
		return true;
	}

	// not our command
	return false;
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
	// This function is visible to, and will be called by, REAPER
	// So let's do some checking...
	
	logNote("loading");
	if (rec)
	{
		// check the version numbers	
		if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
			return 0; // return an error if version numbers are different

					  // Get the address of each API function that will be used
					  /* Setting up your function pointers the ugly way */
		*((void **)&CSurf_TrackFromID) = rec->GetFunc("CSurf_TrackFromID");
		*((void **)&FreeHeapPtr) = rec->GetFunc("FreeHeapPtr");

		/* Macro way, which is much easier and neater! */
		REG_FUNC(GetSetTrackState, rec);
		REG_FUNC(plugin_register, rec);
		REG_FUNC(CountTracks, rec);
		REG_FUNC(GetTrack, rec);
		REG_FUNC(GetTrackName, rec);
		REG_FUNC(GetTrackColor, rec);
		REG_FUNC(GetTrackReceiveName, rec);
		REG_FUNC(GetTrackNumSends, rec);
		REG_FUNC(GetInputChannelName, rec);
		REG_FUNC(GetMediaTrackInfo_Value, rec);
		// Check that the address of each API function has been set, not every version
		// of the API running out there will have the functions you need.
		if (
			!CSurf_TrackFromID ||
			!GetSetTrackState ||
			!FreeHeapPtr
			) return 0; // return an error if any address isn't set

		logNote("success");

		fooCommandId = plugin_register("command_id", (void*)"SomeUniqueNameForTheActionZZZZ");
		gaccel_register_t accelerator;
		accelerator.accel.fVirt = FCONTROL | FALT | FVIRTKEY;
		accelerator.accel.key = 'D';
		accelerator.accel.cmd = fooCommandId;
		accelerator.desc = "Perform foo on the selected tracks";
		if (!rec->Register("gaccel", &accelerator)) {
			// registering the action failed ...
		}

		if (!rec->Register("hookcustommenu", &customMenuHook)) {
			// registering the hook function failed ...
		}

		if (!rec->Register("hookcommand", &commandFilter)) {
			// registering the hook function failed ...
		}
		return 1; // return success
	}
	else
	{
		// The plug-in is being unloaded. Do any cleanup necessary...
		logNote("failed");
		return 0;
	}
} // REAPER_PLUGIN_ENTRYPOINT




void logNote(char note[])
{
	ofstream logFile;
	logFile.open("K:\\Temp\\testPluginLog.txt", ios_base::app);
	logFile << note << endl;
	logFile.close();
}

