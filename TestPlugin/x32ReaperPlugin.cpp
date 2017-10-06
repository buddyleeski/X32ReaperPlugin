// TestPlugin.cpp : Defines the initialization routines for the DLL.
//

#define REAPERAPI_IMPLEMENT

using namespace std;

#include "x32ReaperPlugin.h"
#include "x32Client.h"
#include "../reaper_plugin.h"
#include "../reaper_plugin_functions.h"
#include <fstream>

#define REG_FUNC(x,y) (*(void **)&x) = y->GetFunc(#x)

int syncLabelsAndColorsCommandId;
int initTrackInputRoutesCommandId;
int setTrackColorsFromX32Id;
char* x32IPAddress = "192.168.0.200";

static void customMenuHook(const char* menuString, HMENU menu, int flag)
{
	if (strcmp(menuString, "Main track") == 0) {
		// the track context menu
		if (flag == 0) {
			// add our menu item
			int itemCount = GetMenuItemCount(menu);
			MENUITEMINFO syncLabelsAndColorsItemInfo = {};
			syncLabelsAndColorsItemInfo.cbSize = sizeof(syncLabelsAndColorsItemInfo);
			syncLabelsAndColorsItemInfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
			syncLabelsAndColorsItemInfo.wID = syncLabelsAndColorsCommandId;
			syncLabelsAndColorsItemInfo.fType = MFT_STRING;
			syncLabelsAndColorsItemInfo.dwTypeData = "Sync Labels with X32";
			InsertMenuItem(menu, itemCount++, true, &syncLabelsAndColorsItemInfo);

			MENUITEMINFO initTrackInputRoutesItemInfo = {};
			initTrackInputRoutesItemInfo.cbSize = sizeof(syncLabelsAndColorsItemInfo);
			initTrackInputRoutesItemInfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
			initTrackInputRoutesItemInfo.wID = initTrackInputRoutesCommandId;
			initTrackInputRoutesItemInfo.fType = MFT_STRING;
			initTrackInputRoutesItemInfo.dwTypeData = "Init Track Input Routes";
			InsertMenuItem(menu, itemCount++, true, &initTrackInputRoutesItemInfo);
		}
	}
}

static bool syncLabelsAndColors()
{
	// perform the action
	x32::X32Client client(x32IPAddress);
	logNote("MyFunction");
	int trackCount = CountTracks(0);

	char buffer[255];
	for (int i = 0; i < trackCount; i++)
	{
		MediaTrack* track = GetTrack(0, i);
		double val = GetMediaTrackInfo_Value(track, "I_RECINPUT");

		if (val >= 0 && (val <= 31 || val >= 1024) && val < 1055)
		{
			bool isStereo = (val >= 1024);
			memset(buffer, 0, sizeof(buffer));

			int cardOutput = (val <= 31 ? (int)val : (int)val - 1024) + 1;
			x32::RouteInfo routeInfo = client.GetCardOutputSource(cardOutput);

			if (routeInfo.type != x32::X32Route::INVALID &&  routeInfo.type <= x32::X32Route::AN25_32)
			{
				GetTrackName(track, buffer, 255);
				client.SetLabelName(x32::X32LabelType::Channel, routeInfo.channelNumber, buffer);
				client.SetLabelName(x32::X32LabelType::Channel, i + 17, buffer);

				int x32LabelColor = client.GetLabelColor(x32::X32LabelType::Channel, routeInfo.channelNumber);
				SetTrackColor(track, x32LabelColor);
				//client.SetLabelColorFromValue(x32::X32LabelType::Channel, routeInfo.channelNumber, color);
				client.SetLabelColorFromValue(x32::X32LabelType::Channel, i + 17, x32LabelColor);

				int hardwareSendCount = GetTrackNumSends(track, 1);

				for (int i = 0; i < hardwareSendCount; i++)
					RemoveTrackSend(track, 1, i);

				CreateTrackSend(track, NULL);
				SetTrackSendInfo_Value(track, 1, 0, "I_DSTCHAN", (double)(1024 + i + 2));
			}
		}
	}

	return true;
}

static bool initTrackInputRoutes()
{
	int trackCount = CountTracks(0);

	for (int i = 0; i < trackCount && i < 16; i++)
	{
		MediaTrack* track = GetTrack(0, i);
		double val = SetMediaTrackInfo_Value(track, "I_RECINPUT", (double)i);
	}

	return true;
}

static bool commandFilter(int commandId, int flag)
{
	if (commandId == syncLabelsAndColorsCommandId) {
		return syncLabelsAndColors();
	}
	else if (commandId == initTrackInputRoutesCommandId)
	{
		return initTrackInputRoutes();
	}

	// not our command
	return false;
}

void readIniFile()
{
	ifstream iniFile("Plugins/x32Client.ini");
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
	// This function is visible to, and will be called by, REAPER
	// So let's do some checking...
	
	readIniFile();
	
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
		REG_FUNC(SetTrackColor, rec);
		REG_FUNC(GetTrackReceiveName, rec);
		REG_FUNC(GetTrackNumSends, rec);
		REG_FUNC(GetInputChannelName, rec);
		REG_FUNC(GetMediaTrackInfo_Value, rec);
		REG_FUNC(SetMediaTrackInfo_Value, rec);

		REG_FUNC(GetTrackNumSends, rec);
		REG_FUNC(CreateTrackSend, rec);
		REG_FUNC(RemoveTrackSend, rec);
		REG_FUNC(SetTrackSendInfo_Value, rec);
		// Check that the address of each API function has been set, not every version
		// of the API running out there will have the functions you need.
		if (
			!CSurf_TrackFromID ||
			!GetSetTrackState ||
			!FreeHeapPtr
			) return 0; // return an error if any address isn't set

		syncLabelsAndColorsCommandId = plugin_register("command_id", (void*)"x32ReaperPlugin_syncLabelsAndColorsCommand");
		gaccel_register_t syncLabelsAccelerator;
		syncLabelsAccelerator.accel.fVirt = FCONTROL | FALT | FVIRTKEY;
		syncLabelsAccelerator.accel.key = 'D';
		syncLabelsAccelerator.accel.cmd = syncLabelsAndColorsCommandId;
		syncLabelsAccelerator.desc = "Updates the track colors from the X32, and sets the X32 label text based on the track names";
		if (!rec->Register("gaccel", &syncLabelsAccelerator)) {
			// registering the action failed ...
		}

		initTrackInputRoutesCommandId = plugin_register("command_id", (void*)"x32ReaperPlugin_initTrackInputRoutesCommand");
		gaccel_register_t initTrackInputsAccelerator;
		initTrackInputsAccelerator.accel.fVirt = FCONTROL | FALT | FVIRTKEY;
		initTrackInputsAccelerator.accel.key = 'I';
		initTrackInputsAccelerator.accel.cmd = initTrackInputRoutesCommandId;
		initTrackInputsAccelerator.desc = "Updates the track colors from the X32, and sets the X32 label text based on the track names";
		if (!rec->Register("gaccel", &initTrackInputsAccelerator)) {
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

