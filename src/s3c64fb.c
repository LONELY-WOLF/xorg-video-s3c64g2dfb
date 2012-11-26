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
	VERSION,
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






