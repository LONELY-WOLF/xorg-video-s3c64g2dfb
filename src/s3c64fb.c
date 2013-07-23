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
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);

	if (!pS3C64FB->NoAccel) {
		INFO_MSG("Initializing the \"%s\" sub-module ...", SUB_MODULE_G2D);
		pS3C64FB->pS3C64FBEXA = InitG2DEXA(pScreen, pScrn, pS3C64FB->drmFD);
		if (pS3C64FB->pS3C64FBEXA) {
			INFO_MSG("Successfully initialized the \"%s\" sub-module",
					SUB_MODULE_G2D);
		} else {
			INFO_MSG("Could not initialize the \"%s\" sub-module",
					SUB_MODULE_G2D);
			pS3C64FB->NoAccel = TRUE;
		}
	}

	if (!pS3C64FB->pS3C64FBEXA) {
		pS3C64FB->pS3C64FBEXA = InitNullEXA(pScreen, pScrn, pS3C64FB->drmFD);
	}

	if (pS3C64FB->dri && pS3C64FB->pS3C64FBEXA) {
		pS3C64FB->dri = S3C64FBDRI2ScreenInit(pScreen);
	} else {
		pS3C64FB->dri = FALSE;
	}

	if (S3C64FBVideoScreenInit(pScreen)) {
		INFO_MSG("Initialized XV");
	} else {
		ERROR_MSG("Could not initialize XV");
	}
}

/**
 * The driver's ScreenInit() function.  Fill in pScreen, map the frame buffer,
 * save state, initialize the mode, etc.
 */
static Bool
S3C64FBScreenInit(SCREEN_INIT_ARGS_DECL)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);
	VisualPtr visual;
	xf86CrtcConfigPtr xf86_config;
	int i;

	TRACE_ENTER();

	/* Allocate and map memory areas we need */
	if (!S3C64FBMapMem(pScrn))
		return FALSE;

	xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

	/* need to point to new screen on server regeneration */
	for (i = 0; i < xf86_config->num_crtc; i++)
		xf86_config->crtc[i]->scrn = pScrn;
	for (i = 0; i < xf86_config->num_output; i++)
		xf86_config->output[i]->scrn = pScrn;

	/*
	 * The next step is to setup the screen's visuals, and initialize the
	 * framebuffer code.  In cases where the framebuffer's default
	 * choices for things like visual layouts and bits per RGB are OK,
	 * this may be as simple as calling the framebuffer's ScreenInit()
	 * function.  If not, the visuals will need to be setup before calling
	 * a fb ScreenInit() function and fixed up after.
	 *
	 * For most PC hardware at depths >= 8, the defaults that fb uses
	 * are not appropriate.  In this driver, we fixup the visuals after.
	 */

	/*
	 * Reset the visual list.
	 */

	miClearVisualTypes();
	if (!miSetVisualTypes(pScrn->depth, miGetDefaultVisualMask(pScrn->depth),
			pScrn->rgbBits, pScrn->defaultVisual)) {
		ERROR_MSG("Cannot initialize the visual type for %d bits per pixel!",
				pScrn->bitsPerPixel);
		goto fail;
	}

	if (!miSetPixmapDepths()) {
		ERROR_MSG("Cannot initialize the pixmap depth!");
		goto fail;
	}

	/* Initialize some generic 2D drawing functions: */
	if (!fbScreenInit(pScreen, s3c64fb_bo_map(pS3C64FB->scanout),
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth,
			pScrn->bitsPerPixel)) {
		ERROR_MSG("fbScreenInit() failed!");
		goto fail;
	}

	/* Fixup RGB ordering: */
	visual = pScreen->visuals + pScreen->numVisuals;
	while (--visual >= pScreen->visuals) {
		if ((visual->class | DynamicClass) == DirectColor) {
			visual->offsetRed = pScrn->offset.red;
			visual->offsetGreen = pScrn->offset.green;
			visual->offsetBlue = pScrn->offset.blue;
			visual->redMask = pScrn->mask.red;
			visual->greenMask = pScrn->mask.green;
			visual->blueMask = pScrn->mask.blue;
		}
	}

	/* Continue initializing the generic 2D drawing functions after fixing the
	 * RGB ordering:
	 */
	if (!fbPictureInit(pScreen, NULL, 0)) {
		ERROR_MSG("fbPictureInit() failed!");
		goto fail;
	}

	/* Set the initial black & white colormap indices: */
	xf86SetBlackWhitePixels(pScreen);

	/* Initialize external sub-modules for EXA now, this has to be before
	 * miDCInitialize() otherwise stacking order for wrapped ScreenPtr fxns
	 * ends up in the wrong order.
	 */
	S3C64FBAccelInit(pScreen);

	/* Initialize backing store: */
	xf86SetBackingStore(pScreen);

	/* Cause the cursor position to be updated by the mouse signal handler: */
	xf86SetSilkenMouse(pScreen);

	/* Initialize the cursor: */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if (pS3C64FB->HWCursor) {
		if (!drmmode_cursor_init(pScreen)) {
			ERROR_MSG("Hardware cursor initialization failed");
			pS3C64FB->HWCursor = FALSE;
		}
	}

	/* XXX -- Is this the right place for this?  The Intel i830 driver says:
	 * "Must force it before EnterVT, so we are in control of VT..."
	 */
	pScrn->vtSema = TRUE;

	/* Take over the virtual terminal from the console, set the desired mode,
	 * etc.:
	 */
	S3C64FBEnterVT(VT_FUNC_ARGS(0));

	/* Set the desired mode(s): */
	if (!xf86SetDesiredModes(pScrn)) {
		ERROR_MSG("xf86SetDesiredModes() failed!");
		goto fail;
	}

	/* Do some XRandR initialization: */
	if (!xf86CrtcScreenInit(pScreen)) {
		ERROR_MSG("xf86CrtcScreenInit() failed!");
		goto fail;
	}

	if (has_rotation(pS3C64FB)) {
		xf86RandR12SetRotations(pScreen, RR_Rotate_0 | RR_Rotate_90 |
				RR_Rotate_180 | RR_Rotate_270 | RR_Reflect_X | RR_Reflect_Y);
	} else {
#if XF86_CRTC_VERSION < 4
		WARNING_MSG("rotation not supported by XF86_CRTC_VERSION version: %d",
				XF86_CRTC_VERSION);
#endif
	}

	if (!miCreateDefColormap(pScreen)) {
		ERROR_MSG("Cannot create colormap!");
		goto fail;
	}

	if (!xf86HandleColormaps(pScreen, 256, 8, S3C64FBLoadPalette, NULL,
			CMAP_PALETTED_TRUECOLOR)) {
		ERROR_MSG("xf86HandleColormaps() failed!");
		goto fail;
	}

	/* Setup power management: */
	xf86DPMSInit(pScreen, xf86DPMSSet, 0);

	pScreen->SaveScreen = xf86SaveScreen;

	/* Wrap some screen functions: */
	wrap(pS3C64FB, pScreen, CloseScreen, S3C64FBCloseScreen);
	wrap(pS3C64FB, pScreen, CreateScreenResources, S3C64FBCreateScreenResources);
	wrap(pS3C64FB, pScreen, BlockHandler, S3C64FBBlockHandler);

	drmmode_screen_init(pScrn);

	TRACE_EXIT();
	return TRUE;

fail:
	TRACE_EXIT();
	return FALSE;
}


static void
S3C64FBLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		LOCO * colors, VisualPtr pVisual)
{
	TRACE_ENTER();
	TRACE_EXIT();
}


/**
 * The driver's CloseScreen() function.  This is called at the end of each
 * server generation.  Restore state, unmap the frame buffer (and any other
 * mapped memory regions), and free per-Screen data structures (except those
 * held by pScrn).
 */
static Bool
S3C64FBCloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);

	TRACE_ENTER();

	drmmode_screen_fini(pScrn);

	if (pScrn->vtSema == TRUE) {
		S3C64FBLeaveVT(VT_FUNC_ARGS(0));
	}

	if (pS3C64FB->pS3C64FBEXA) {
		if (pS3C64FB->pS3C64FBEXA->CloseScreen) {
			pS3C64FB->pS3C64FBEXA->CloseScreen(CLOSE_SCREEN_ARGS);
		}
	}

	if (pS3C64FB->dri) {
		S3C64FBDRI2CloseScreen(pScreen);
	}

	S3C64FBVideoCloseScreen(pScreen);

	S3C64FBUnmapMem(pScrn);

	pScrn->vtSema = FALSE;

	unwrap(pS3C64FB, pScreen, CloseScreen);
	unwrap(pS3C64FB, pScreen, BlockHandler);
	unwrap(pS3C64FB, pScreen, CreateScreenResources);

	TRACE_EXIT();

	return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);
}



/**
 * Adjust the screen pixmap for the current location of the front buffer.
 * This is done at EnterVT when buffers are bound as long as the resources
 * have already been created, but the first EnterVT happens before
 * CreateScreenResources.
 */
static Bool
S3C64FBCreateScreenResources(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);

	swap(pS3C64FB, pScreen, CreateScreenResources);
	if (!(*pScreen->CreateScreenResources) (pScreen))
		return FALSE;
	swap(pS3C64FB, pScreen, CreateScreenResources);

	return TRUE;
}


static void
S3C64FBBlockHandler(BLOCKHANDLER_ARGS_DECL)
{
	SCREEN_PTR(arg);
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);

	swap(pS3C64FB, pScreen, BlockHandler);
	(*pScreen->BlockHandler) (BLOCKHANDLER_ARGS);
	swap(pS3C64FB, pScreen, BlockHandler);

	/* TODO S3C64FBVideoBlockHandler(), etc.. */
}



/**
 * The driver's SwitchMode() function.  Initialize the new mode for the
 * Screen.
 */
static Bool
S3C64FBSwitchMode(SWITCH_MODE_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}



/**
 * The driver's AdjustFrame() function.  For cases where the frame buffer is
 * larger than the monitor resolution, this function can pan around the frame
 * buffer within the "viewport" of the monitor.
 */
static void
S3C64FBAdjustFrame(ADJUST_FRAME_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	drmmode_adjust_frame(pScrn, x, y);
}



/**
 * The driver's EnterVT() function.  This is called at server startup time, and
 * when the X server takes over the virtual terminal from the console.  As
 * such, it may need to save the current (i.e. console) HW state, and set the
 * HW state as needed by the X server.
 */
static Bool
S3C64FBEnterVT(VT_FUNC_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);
	int ret;

	TRACE_ENTER();

	ret = drmSetMaster(pS3C64FB->drmFD);
	if (ret) {
		ERROR_MSG("Cannot get DRM master: %s\n", strerror(ret));
	}

	if (!xf86SetDesiredModes(pScrn)) {
		ERROR_MSG("xf86SetDesiredModes() failed!");
		return FALSE;
	}

	TRACE_EXIT();
	return TRUE;
}



/**
 * The driver's LeaveVT() function.  This is called when the X server
 * temporarily gives up the virtual terminal to the console.  As such, it may
 * need to restore the console's HW state.
 */
static void
S3C64FBLeaveVT(VT_FUNC_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);
	int ret;

	TRACE_ENTER();

	ret = drmDropMaster(pS3C64FB->drmFD);
	if (ret) {
		WARNING_MSG("drmDropMaster failed: %s\n", strerror(errno));
	}

	TRACE_EXIT();
}



/**
 * The driver's FreeScreen() function.  This is called at the server's end of
 * life.  This should free any driver-allocated data that was allocated
 * up-to-and-including an unsuccessful ScreenInit() call.
 */
static void
S3C64FBFreeScreen(FREE_SCREEN_ARGS_DECL)
{
	SCRN_INFO_PTR(arg);
	S3C64FBPtr pS3C64FB = S3C64FBPTR(pScrn);

	TRACE_ENTER();

	if (!pS3C64FB) {
		/* This can happen if a Screen is deleted after Probe(): */
		return;
	}

	if (pS3C64FB->pS3C64FBEXA) {
		if (pS3C64FB->pS3C64FBEXA->FreeScreen) {
			pS3C64FB->pS3C64FBEXA->FreeScreen(FREE_SCREEN_ARGS(pScrn));
		}
		free(pS3C64FB->pS3C64FBEXA);
	}

	s3c64fb_device_del(pS3C64FB->dev);

	S3C64FBCloseDRMMaster(pScrn);

	S3C64FBFreeRec(pScrn);

	TRACE_EXIT();
}

#ifdef XSERVER_PLATFORM_BUS
static Bool
S3C64FBPlatformProbe(DriverPtr drv, int entity_num, int flags,
                   struct xf86_platform_device *dev, intptr_t match_data)
{
	ScrnInfoPtr pScrn = NULL;
	Bool foundScreen = FALSE;
	char *busid = xf86_get_platform_device_attrib(dev, ODEV_ATTRIB_BUSID);
	int fd;

	fd = drmOpen(NULL, busid);
	if (fd != -1) {
		pScrn = xf86AllocateScreen(drv, 0);
		if (!pScrn) {
			EARLY_ERROR_MSG("Cannot allocate a ScrnInfoPtr");
			return FALSE;
		}

		xf86AddEntityToScreen(pScrn, entity_num);
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

		drmClose(fd);
	}

	return foundScreen;
}
#endif

