// Driver Init module
#include "burner.h"
#include "neocdlist.h"
#include <Dbt.h>

int bDrvOkay = 0;						// 1 if the Driver has been initted okay, and it's okay to use the BurnDrv functions

TCHAR szAppRomPaths[DIRS_MAX][MAX_PATH] = { { _T("") }, { _T("") }, { _T("") }, { _T("") }, { _T("") },
											{ _T("") }, { _T("roms/nes/") }, { _T("roms/nes_fds/") }, { _T("roms/nes_hb/") }, { _T("roms/spectrum/") },
											{ _T("roms/msx/") }, { _T("roms/sms/") }, { _T("roms/gamegear/") }, { _T("roms/sg1000/") }, { _T("roms/coleco/") },
											{ _T("roms/tg16/") }, { _T("roms/sgx/") }, { _T("roms/pce/") }, { _T("roms/megadrive/") }, { _T("roms/") } };

static bool bSaveRAM = false;

static INT32 nNeoCDZnAudSampleRateSave = 0;

static int DrvBzipOpen()
{
	BzipOpen(false);

	// If there is a problem with the romset, report it
	switch (BzipStatus()) {
	case BZIP_STATUS_BADDATA: {
		if (!bHideROMWarnings) {
			FBAPopupDisplay(PUF_TYPE_WARNING);
		}
		break;
	}
	case BZIP_STATUS_ERROR: {
		FBAPopupDisplay(PUF_TYPE_ERROR);

#if 0 || !defined FBNEO_DEBUG
		// Don't even bother trying to start the game if we know it won't work
		BzipClose();
		return 1;
#endif

		break;
	}
	default: {

#if 0 && defined FBNEO_DEBUG
		FBAPopupDisplay(PUF_TYPE_INFO);
#else
		FBAPopupDisplay(PUF_TYPE_INFO | PUF_TYPE_LOGONLY);
#endif

	}
	}

	return 0;
}

static int DoLibInit()					// Do Init of Burn library driver
{
	int nRet = 0;

	if (DrvBzipOpen()) {
		return 1;
	}

	if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) != HARDWARE_SNK_MVS) {
		if (!bQuietLoading) ProgressCreate();
	}

	nRet = BurnDrvInit();

	BzipClose();

	if (!bQuietLoading) ProgressDestroy();

	if (nRet) {
		return 3;
	}
	else {
		return 0;
	}
}

// Catch calls to BurnLoadRom() once the emulation has started;
// Intialise the zip module before forwarding the call, and exit cleanly.
static int __cdecl DrvLoadRom(unsigned char* Dest, int* pnWrote, int i)
{
	int nRet;

	BzipOpen(false);

	if ((nRet = BurnExtLoadRom(Dest, pnWrote, i)) != 0) {
		char* pszFilename;

		BurnDrvGetRomName(&pszFilename, i, 0);

		FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_LOAD_REQUEST), pszFilename, BurnDrvGetText(DRV_NAME));
		FBAPopupDisplay(PUF_TYPE_ERROR);
	}

	BzipClose();

	BurnExtLoadRom = DrvLoadRom;

	ScrnTitle();

	return nRet;
}

int __cdecl DrvCartridgeAccess(BurnCartrigeCommand nCommand)
{
	switch (nCommand) {
	case CART_INIT_START:
		if (!bQuietLoading) ProgressCreate();
		if (DrvBzipOpen()) {
			return 1;
		}
		break;
	case CART_INIT_END:
		if (!bQuietLoading) ProgressDestroy();
		BzipClose();
		break;
	case CART_EXIT:
		break;
	default:
		return 1;
	}

	return 0;
}

void NeoCDZRateChangeback()
{
	if (nNeoCDZnAudSampleRateSave != 0) {
		bprintf(PRINT_IMPORTANT, _T("Switching sound rate back to user-selected %dhz\n"), nNeoCDZnAudSampleRateSave);
		nAudSampleRate[nAudSelect] = nNeoCDZnAudSampleRateSave;
		nNeoCDZnAudSampleRateSave = 0;
	}
}

static void NeoCDZRateChange()
{
	if (nAudSampleRate[nAudSelect] != 44100) {
		nNeoCDZnAudSampleRateSave = nAudSampleRate[nAudSelect];
		bprintf(PRINT_IMPORTANT, _T("Switching sound rate to 44100hz (from %dhz) as required by NeoGeo CDZ\n"), nNeoCDZnAudSampleRateSave);
		nAudSampleRate[nAudSelect] = 44100; // force 44100hz for CDDA
	}
}

int DrvInit(int nDrvNum, bool bRestore)
{
	int nStatus;

	DrvExit();						// Make sure exitted
	MediaExit(false);


	nBurnDrvActive = nDrvNum;		// Set the driver number

	if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_MVS) {

		BurnExtCartridgeSetupCallback = DrvCartridgeAccess;

		if (!bQuietLoading) {		// not from cmdline - show slot selector
			if (SelMVSDialog()) {
				POST_INITIALISE_MESSAGE;
				return 0;
			}
		}
	}

	if ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NEOCD) {
		if (CDEmuInit()) {
			FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_CDEMU_INI_FAIL));
			FBAPopupDisplay(PUF_TYPE_ERROR);

			POST_INITIALISE_MESSAGE;
			return 0;
		}

		NeoCDInfo_Init();

		NeoCDZRateChange();
	}

	if (bVidAutoSwitchFull && !bVidAutoSwitchFullDisable) {
		nVidFullscreen = 1;
	}

	// Init the input and save the audio as well as the blitter init for later
	// IE: reduce the number of mode changes which is nice for emulator front-ends
	// this fixes the Mortal Kombat games from having the incorrect audio pitch when being launched via the command line
	bVidOkay = 1; // don't init video yet
	bAudOkay = 1; // don't init audio yet (but grab the soundcard params (nBurnSoundRate) so soundcores can init)
	MediaInit();
	bVidOkay = 0;
	bAudOkay = 0;

	// Define nMaxPlayers early; GameInpInit() needs it (normally defined in DoLibInit()).
	// GameInpInit() needs max players b/c of the macro system.
	nMaxPlayers = BurnDrvGetMaxPlayers();

	// Init game input.
	// This basically sets everything to zero / sets constants / clears all pc side inputs.
	GameInpInit();

	//// NOTE: This block is for loading last config, and then defaults for every input
	//// that hasn't been set yet.  Seems like overkill, but is also game specific.
	//// Seems better to load defaults first, then player specific stuff....
	//// --> ANYWAY: The goal would be to construct some kind of default input set based on the system,
	//// and the last user stuff.......  However, with PNP, last user stuff doesn't really make sense unless
	//// we could uniquely identify the person behind the pad, which isn't possible.....
	//{
	//	// If we don't load the last game input config, we load the hardware defaults instead...
	//	if (ConfigGameLoad(true)) {
	//		ConfigGameLoadHardwareDefaults();
	//	}
	//	// Fills out all other undefined inputs from some other config file...
	//}

	// We are just going to create an input set from the default hardware inputs.
	// Skip the loading of the inis and stuff....
	// We can come up with a rewrite for that at a later date if we like.
	// ConfigGameLoadHardwareDefaults();
	SetDefaultGameInputs();

	// Now that we have our defaults, we can populate the input set.
	// NOTE: The code above, where the calls to GameInpAutoOne are made,
	// can eventually be folded into the code for creating the deafault input set.  We
	// can probably find a way to greatly reduce its complexity at that time as well.
	CreateDefaultInputDesc();

	// NOTE: This is where we can check to see if there are any gamepads, etc. plugged in.
	// If so, then the default(current) input desc or whatever can be modified.
	// --> Now we can modify the input set based on if any gamepads are plugged in.

	// Really what we want to do is MODIFY the current input description so that it
	// includes any plugged in gamepads....
	// So we don't need to do a whole mapping step, we just need to modify the parts
	// of the description that correspond to each player.
	UpdatPCInputsForGamepads();

	// Other note: If there aren't pads currently plugged in, then I don't really
	// see a reason for them to be mapped (via defaults) in the first place.  We
	// should just keep a reasonable set of keyboard inputs for all of it.
	RebuildInputSet();

	InputMake(true);




	if (kNetGame) {
		nBurnCPUSpeedAdjust = 0x0100;
	}

	nStatus = DoLibInit();			// Init the Burn library's driver

	if (nStatus) {
		if (nStatus & 2) {
			BurnDrvExit();			// Exit the driver

			ScrnTitle();

			FBAPopupAddText(PUF_TEXT_DEFAULT, MAKEINTRESOURCE(IDS_ERR_BURN_INIT), BurnDrvGetText(DRV_FULLNAME));
			FBAPopupDisplay(PUF_TYPE_WARNING);
		}

		NeoCDZRateChangeback();

		nBurnDrvActive = -1;

		POST_INITIALISE_MESSAGE;
		return 1;
	}

	BurnExtLoadRom = DrvLoadRom;

	bDrvOkay = 1;						// Okay to use all BurnDrv functions

	if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
		nScreenSize = nScreenSizeVer;
		bVidArcaderes = bVidArcaderesVer;
		nVidWidth = nVidVerWidth;
		nVidHeight = nVidVerHeight;
	}
	else {
		nScreenSize = nScreenSizeHor;
		bVidArcaderes = bVidArcaderesHor;
		nVidWidth = nVidHorWidth;
		nVidHeight = nVidHorHeight;
	}

	bSaveRAM = false;
	if (kNetGame) {
		NetworkInitInput();
		NetworkGetInput();
	}
	else {
		if (bRestore) {
			StatedAuto(0);
			bSaveRAM = true;

			ConfigCheatLoad();
		}
	}

	nBurnLayer = 0xFF;				// show all layers

	// Reset the speed throttling code, so we don't 'jump' after the load
	RunReset();

	VidExit();
	POST_INITIALISE_MESSAGE;
	CallRegisteredLuaFunctions(LUACALL_ONSTART);

	if (!strcmp(BurnDrvGetTextA(DRV_NAME), "sfiii3nr1")) {
		if (ReadValueAtHardwareAddress(0x638FC63, 1, 0) == 0x0A)
			WriteValueAtHardwareAddress(0x638FC63, 0x0B, 1, 0);
	}


	return 0;
}

int DrvInitCallback()
{
	return DrvInit(nBurnDrvActive, false);
}

int DrvExit()
{
	if (bDrvOkay) {
		NeoCDZRateChangeback();

		StopReplay();

		VidExit();

		InvalidateRect(hScrnWnd, NULL, 1);
		UpdateWindow(hScrnWnd);			// Blank screen window

		DestroyWindow(hInpdDlg);		// Make sure the Input Dialog is exited
		DestroyWindow(hInpDIPSWDlg);	// Make sure the DipSwitch Dialog is exited
		DestroyWindow(hInpCheatDlg);	// Make sure the Cheat Dialog is exited

		if (nBurnDrvActive < nBurnDrvCount) {
			MemCardEject();				// Eject memory card if present

			if (bSaveRAM) {
				StatedAuto(1);			// Save NV (or full) RAM
				bSaveRAM = false;
			}

			ConfigGameSave(bSaveInputs);

			GameInpExit();				// Exit game input

			BurnDrvExit();				// Exit the driver
		}
	}

	BurnExtLoadRom = NULL;

	bDrvOkay = 0;					// Stop using the BurnDrv functions

	bRunPause = 0;					// Don't pause when exitted

	if (bAudOkay) {
		// Write silence into the sound buffer on exit, and for drivers which don't use pBurnSoundOut
		AudWriteSilence();
	}

	CDEmuExit();

	BurnExtCartridgeSetupCallback = NULL;

	nBurnDrvActive = ~0U;			// no driver selected

	return 0;
}
