/*
 * Copyright © 2012 Artjom Sabadyr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Artjom Sabadyr <zvenayte@pisem.net>
 */

#include "s3c64fb.h"

//Prototypes


_X_EXPORT DriverRec S3C64FB =
{
	S3C64FB_VERSION,
	S3C64FB_DRIVER_NAME,
	S3C64FBIdentify,
	S3C64FBProbe,
	S3C64FBAvailableOptions,
	NULL,
	0
};

static SymTabRec S3C64FBChipsets[] = 
{
	{0, "s3c6410"},
	{-1, NULL}
};

static XF86ModuleVersionInfo S3C64FBVersRec =
{
		S3C64FB_DRIVER_NAME,
		MODULEVENDORSTRING,
		MODINFOSTRING1,
		MODINFOSTRING2,
		XORG_VERSION_CURRENT,
		S3C64FB_MAJOR_VERSION, S3C64FB_MINOR_VERSION, S3C64FB_PATCHLEVEL,
		ABI_CLASS_VIDEODRV,
		ABI_VIDEODRV_VERSION,
		MOD_CLASS_VIDEODRV,
		{0, 0, 0, 0}
};

typedef enum
{
	OPTION_DEBUG,
	OPTION_DRI,
	OPTION_NO_ACCEL,
	OPTION_HW_CURSOR
} S3C64FBOpts;

static const OptionInfoRec S3C64FBOptions[] =
{
	{ OPTION_DEBUG,		"Debug",	OPTV_BOOLEAN,	{0},	FALSE },
	{ OPTION_DRI,		"DRI",		OPTV_BOOLEAN,	{0},	FALSE },
	{ OPTION_NO_ACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0},	FALSE },
	{ OPTION_HW_CURSOR,	"HWcursor",	OPTV_BOOLEAN,	{0},	FALSE },
	{ -1,				NULL,		OPTV_NONE,		{0},	FALSE }
};

static MODULESETUPPROTO(S3C64FBSetup);

_X_EXPORT XF86ModuleData s3c64fbModuleData = { &S3C64FBVersRec, S3C64FBSetup, NULL };

static pointer S3C64FBSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	/* This module should be loaded only once, but check to be sure: */
	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&S3C64FB, module, 0);

		/* The return value must be non-NULL on success even though there is no
		 * TearDownProc.
		 */
		return (pointer) 1;
	} else {
		if (errmaj)
			*errmaj = LDR_ONCEONLY;
		return NULL;
	}
}

static Bool S3C64FBGetRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate != NULL)
		return TRUE;

	pScrn->driverPrivate = calloc(1, sizeof(S3C64FBRec));
	if (pScrn->driverPrivate == NULL)
		return FALSE;

	return TRUE;
}

static void S3C64FBFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return;
	free(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

static void S3C64FBIdentify(int flags)
{
	xf86PrintChipsets(S3C64FB_NAME, "Driver for Samsung s3c6410 chipset", S3C64FBChipsets);
}

static Bool S3C64FBProbe(DriverPtr drv, int flags)
{
	Bool foundScreen = FALSE;
	int numDevSections, numUsed;
	GDevPtr *devSections;
	int *usedChips;
	int i;
	/*
	* Find the config file Device sections that match this
	* driver, and return if there are none.
	*/
	if ((numDevSections = xf86MatchDevice(S3C64FB_DRIVER_NAME,
	&devSections)) <= 0)
	{
		return FALSE;
	}
	xf86AddBusDeviceToConfigure(S3C64FB_DRIVER_NAME, BUS_NONE, NULL, 0);

	pScrn = xf86AllocateScreen(drv, 0);

	if (!pScrn)
	{
		return FALSE;
	}

	if (devSections)
	{
		int entity = xf86ClaimNoSlot(drv, 0, devSections[0], TRUE);
		xf86AddEntityToScreen(pScrn, entity);
	}

	foundScreen = TRUE;

	pScrn->driverVersion = S3C64FB_VERSION;
	pScrn->driverName    = (char *)S3C64FB_DRIVER_NAME;
	pScrn->name          = (char *)S3C64FB_NAME;
	pScrn->Probe         = S3C64FBProbe;
	pScrn->PreInit       = S3C64FBPreInit;
	pScrn->ScreenInit    = S3C64FBScreenInit;
	pScrn->SwitchMode    = S3C64FBSwitchMode;
	pScrn->AdjustFrame   = S3C64FBAdjustFrame;
	pScrn->EnterVT       = S3C64FBEnterVT;
	pScrn->LeaveVT       = S3C64FBLeaveVT;
	pScrn->FreeScreen    = S3C64FBFreeScreen;

	free(devSections);
	return foundScreen;
}

static Bool S3C64FBPreInit(ScrnInfoPtr pScrn, int flags)
{
	S3C64FBPtr pS3C64FB;
	int default_depth, fbbpp;

	TRACE_ENTER();

	if (pScrn->numEntities != 1)
	{
		return FALSE;
	}

	S3C64FBGetRec(pScrn);
	pS3C64FB = S3C64FBPTR(pScrn);

	pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
	pS3C64FB->pEntityInfo = pEnt;

	pScrn->monitor = pScrn->confScreen->monitor;

	/* Get the current depth, and set it for XFree86: */
	default_depth = 24;  /* TODO: get from kernel */
	fbbpp = 32;  /* TODO: get from kernel */

	if (!xf86SetDepthBpp(pScrn, default_depth, 0, fbbpp, Support32bppFb)) {
		/* The above function prints an error message. */
		goto fail;
	}
	xf86PrintDepthBpp(pScrn);

	/* Set the color weight: */
	if (!xf86SetWeight(pScrn, defaultWeight, defaultMask)) {
		/* The above function prints an error message. */
		goto fail;
	}

	/* Set the gamma: */
	if (!xf86SetGamma(pScrn, defaultGamma)) {
		/* The above function prints an error message. */
		goto fail;
	}

	/* Visual init: */
	if (!xf86SetDefaultVisual(pScrn, -1)) {
		/* The above function prints an error message. */
		goto fail;
	}

	/* We don't support 8-bit depths: */
	if (pScrn->depth < 16) {
		ERROR_MSG("The requested default visual (%s) has an unsupported "
				"depth (%d).",
				xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
		goto fail;
	}

	/* Using a programmable clock: */
	pScrn->progClock = TRUE;

	/*
	 * Process the "xorg.conf" file options:
	 */
	xf86CollectOptions(pScrn, NULL);
	if (!(pS3C64FB->pOptionInfo = calloc(1, sizeof(S3C64FBOptions))))
		return FALSE;
	memcpy(pS3C64FB->pOptionInfo, S3C64FBOptions, sizeof(S3C64FBOptions));
	xf86ProcessOptions(pScrn->scrnIndex, pS3C64FB->pEntityInfo->device->options,
			pS3C64FB->pOptionInfo);

	/* Determine if the user wants debug messages turned on: */
	s3c64fbDebug = xf86ReturnOptValBool(pS3C64FB->pOptionInfo, OPTION_DEBUG, FALSE);

	pS3C64FB->dri = xf86ReturnOptValBool(pS3C64FB->pOptionInfo, OPTION_DRI, TRUE);

	/* Determine if user wants to disable hw mouse cursor: */
	pS3C64FB->HWCursor = xf86ReturnOptValBool(pS3C64FB->pOptionInfo,
			OPTION_HW_CURSOR, TRUE);
	INFO_MSG("Using %s cursor", pS3C64FB->HWCursor ? "HW" : "SW");

	/* Determine if the user wants to disable acceleration: */
	pS3C64FB->NoAccel = xf86ReturnOptValBool(pS3C64FB->pOptionInfo,
			OPTION_NO_ACCEL, FALSE);

	xf86RandR12PreInit(pScrn);

	/* Let XFree86 calculate or get (from command line) the display DPI: */
	xf86SetDpi(pScrn, 0, 0);

	/* Ensure we have a supported depth: */
	switch (pScrn->bitsPerPixel) {
	case 16:
	case 24:
	case 32:
		break;
	default:
		ERROR_MSG("The requested number of bits per pixel (%d) is unsupported.",
				pScrn->bitsPerPixel);
		goto fail;
	}


	/* Load external sub-modules now: */

	if (!(xf86LoadSubModule(pScrn, "dri2") &&
			xf86LoadSubModule(pScrn, "exa") &&
			xf86LoadSubModule(pScrn, "fb"))) {
		goto fail;
	}

	if (xf86LoadSubModule(pScrn, SUB_MODULE_PVR)) {
		INFO_MSG("Loaded the %s sub-module", SUB_MODULE_PVR);
	} else {
		INFO_MSG("Cannot load the %s sub-module", SUB_MODULE_PVR);
		/* note that this is not fatal.. since IMG/PVR EXA module
		 * is closed source, it is only optional.
		 */
		pS3C64FB->NoAccel = TRUE;  /* don't call InitPowerVREXA() */
	}

	TRACE_EXIT();
	return TRUE;

fail:
	TRACE_EXIT();
	S3C64FBFreeRec(pScrn);
	return FALSE;
}


/**
 * Initialize EXA and DRI2
 */
static void
S3C64FBAccelInit(ScreenPtr pScreen)
{





