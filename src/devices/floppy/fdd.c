/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the floppy drive emulation.
 *
 * Version:	@(#)fdd.c	1.0.20	2019/05/03
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Sarah Walker, <tommowalker@tommowalker.co.uk>
 *
 *		Copyright 2018,2019 Fred N. van Kempen.
 *		Copyright 2016-2019 Miran Grca.
 *		Copyright 2008-2018 Sarah Walker.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#define HAVE_STDARG_H
#define dbglog fdd_log
#include "../../emu.h"
#include "../../timer.h"
#include "../../ui/ui.h"
#include "../../plat.h"
#include "fdd.h"
#include "fdd_86f.h"
#include "fdd_fdi.h"
#include "fdd_imd.h"
#include "fdd_img.h"
#include "fdd_json.h"
#include "fdd_mfm.h"
#include "fdd_td0.h"
#include "fdc.h"


typedef struct {
    int8_t	drive;
    int8_t	type;
    int8_t	turbo;
    int8_t	check_bpb;

    int		track;
    int		densel;
    int		head;
} fdd_t;


/* FIXME: these should be combined. */
DRIVE		drives[FDD_NUM];
wchar_t		floppyfns[4][512];
fdd_t		fdd[FDD_NUM];
int		ui_writeprot[FDD_NUM] = {0, 0, 0, 0};
int		fdd_cur_track[FDD_NUM];
int		writeprot[FDD_NUM], fwriteprot[FDD_NUM];
int64_t		fdd_poll_time[FDD_NUM] = { 16LL, 16LL, 16LL, 16LL };
int		drive_type[FDD_NUM];
int		drive_empty[FDD_NUM] = {1, 1, 1, 1};
int		fdd_changed[FDD_NUM];
int64_t		motoron[FDD_NUM];
int		oldtrack[FDD_NUM] = {0, 0, 0, 0};
d86f_handler_t	d86f_handler[FDD_NUM];

int		defaultwriteprot = 0;
int		curdrive = 0;
int		motorspin;
int		fdc_indexcount = 52;
int		fdd_notfound = 0;
#ifdef ENABLE_FDD_LOG
int		fdd_do_log = ENABLE_FDD_LOG;
#endif


static fdc_t	*fdd_fdc;
static int	fdd_period = 32;


static const struct {
    const wchar_t	*ext;
    int		(*load)(int drive, const wchar_t *fn);
    void	(*close)(int drive);
    int		size;
} loaders[]= {
    { L"001",	img_load,	img_close,	-1 },
    { L"002",	img_load,	img_close,	-1 },
    { L"003",	img_load,	img_close,	-1 },
    { L"004",	img_load,	img_close,	-1 },
    { L"005",	img_load,	img_close,	-1 },
    { L"006",	img_load,	img_close,	-1 },
    { L"007",	img_load,	img_close,	-1 },
    { L"008",	img_load,	img_close,	-1 },
    { L"009",	img_load,	img_close,	-1 },
    { L"010",	img_load,	img_close,	-1 },
    { L"12", 	img_load,	img_close,	-1 },
    { L"144",	img_load,	img_close,	-1 },
    { L"360",	img_load,	img_close,	-1 },
    { L"720",	img_load,	img_close,	-1 },
    { L"86F",	d86f_load,     d86f_close,	-1 },
    { L"BIN",	img_load,	img_close,	-1 },
    { L"CQ", 	img_load,	img_close,	-1 },
    { L"CQM",	img_load,	img_close,	-1 },
    { L"DDI",	img_load,       img_close,	-1 },
    { L"DSK",	img_load,	img_close,	-1 },
    { L"FDI",	fdi_load,	fdi_close,	-1 },
    { L"FDF",	img_load,	img_close,	-1 },
    { L"FLP",	img_load,	img_close,	-1 },
    { L"HDM",	img_load,	img_close,	-1 },
    { L"IMA",	img_load,	img_close,	-1 },
    { L"IMD",	imd_load,	imd_close,	-1 },
    { L"IMG",	img_load,	img_close,	-1 },
    { L"JSON",	json_load,	json_close,	-1 },
    { L"MFM",	mfm_load,	mfm_close,	-1 },
    { L"TD0",	td0_load,	td0_close,	-1 },
    { L"VFD",	img_load,	img_close,	-1 },
    { L"XDF",	img_load,	img_close,	-1 },
    { NULL,	NULL,		NULL,		-1 }
};

static int driveloaders[4];


/* Flags:
   Bit  0:	300 rpm supported;
   Bit  1:	360 rpm supported;
   Bit  2:	size (0 = 3.5", 1 = 5.25");
   Bit  3:	sides (0 = 1, 1 = 2);
   Bit  4:	double density supported;
   Bit  5:	high density supported;
   Bit  6:	extended density supported;
   Bit  7:	double step for 40-track media;
   Bit  8:	invert DENSEL polarity;
   Bit  9:	ignore DENSEL;
   Bit 10:	drive is a PS/2 drive;
*/
#define FLAG_RPM_300		1
#define FLAG_RPM_360		2
#define FLAG_525		   4
#define FLAG_DS			   8
#define FLAG_HOLE0		  16
#define FLAG_HOLE1		  32
#define FLAG_HOLE2		  64
#define FLAG_DOUBLE_STEP	 128
#define FLAG_INVERT_DENSEL	 256
#define FLAG_IGNORE_DENSEL	 512
#define FLAG_PS2		1024


static const struct {
   const char	*internal_name;
   const char	*name;
   int		max_track;
   int		flags;
} drive_types[] = {
    {
	"none", "None",
	0, 0
    },
    {
	"525_1dd", "5.25\" 160/180K",
	43, FLAG_RPM_300 | FLAG_525 | FLAG_HOLE0
    },
    {
	"525_2dd", "5.25\" 320/360K",
	43, FLAG_RPM_300 | FLAG_525 | FLAG_DS | FLAG_HOLE0
    },
    {
	"525_2qd", "5.25\" 720K",
	86, FLAG_RPM_300 | FLAG_525 | FLAG_DS | FLAG_HOLE0 | FLAG_DOUBLE_STEP
    },
    {
	"525_2hd_ps2", "5.25\" 1.2M PS/2",
	86, FLAG_RPM_360 | FLAG_525 | FLAG_DS | FLAG_HOLE0 | FLAG_HOLE1 | \
	      FLAG_DOUBLE_STEP | FLAG_INVERT_DENSEL | FLAG_PS2
    },
    {
	"525_2hd", "5.25\" 1.2M",
	86, FLAG_RPM_360 | FLAG_525 | FLAG_DS | FLAG_HOLE0 | FLAG_HOLE1 | \
	      FLAG_DOUBLE_STEP
    },
    {
	"525_2hd_dualrpm", "5.25\" 1.2M 300/360 RPM",
	86, FLAG_RPM_300 | FLAG_RPM_360 | FLAG_525 | FLAG_DS | FLAG_HOLE0 | \
	    FLAG_HOLE1 | FLAG_DOUBLE_STEP
    },
    {
	"35_1dd", "3.5\" 360K",
	86, FLAG_RPM_300 | FLAG_HOLE0 | FLAG_DOUBLE_STEP
    },
    {
	"35_2dd", "3.5\" 720K",
	86, FLAG_RPM_300 | FLAG_DS | FLAG_HOLE0 | FLAG_DOUBLE_STEP
    },
    {
	"35_2hd_ps2", "3.5\" 1.44M PS/2",
	86, FLAG_RPM_300 | FLAG_DS | FLAG_HOLE0 | FLAG_HOLE1 | \
	    FLAG_DOUBLE_STEP | FLAG_INVERT_DENSEL | FLAG_PS2
    },
    {
	"35_2hd", "3.5\" 1.44M",
	86, FLAG_RPM_300 | FLAG_DS | FLAG_HOLE0 | FLAG_HOLE1 | \
	    FLAG_DOUBLE_STEP
    },
    {
	"35_2hd_nec", "3.5\" 1.25M PC-98",
	86, FLAG_RPM_300 | FLAG_RPM_360 | FLAG_DS | FLAG_HOLE0 | \
	    FLAG_HOLE1 | FLAG_DOUBLE_STEP | FLAG_INVERT_DENSEL
    },
    {
	"35_2hd_3mode", "3.5\" 1.44M 300/360 RPM",
	86, FLAG_RPM_300 | FLAG_RPM_360 | FLAG_DS | FLAG_HOLE0 | \
	    FLAG_HOLE1 | FLAG_DOUBLE_STEP
    },
    {
	"35_2ed", "3.5\" 2.88M",
	86, FLAG_RPM_300 | FLAG_DS | FLAG_HOLE0 | FLAG_HOLE1 | \
	    FLAG_HOLE2 | FLAG_DOUBLE_STEP
    },
    {
	NULL
    }
};


#ifdef _LOGGING
void
fdd_log(int level, const char *fmt, ...)
{
# ifdef ENABLE_FDD_LOG
    va_list ap;

    if (fdd_do_log >= level) {
	va_start(ap, fmt);
	pclog_ex(fmt, ap);
	va_end(ap);
    }
# endif
}
#endif


const char *
fdd_getname(int type)
{
    return drive_types[type].name;
}


const char *
fdd_get_internal_name(int type)
{
    return drive_types[type].internal_name;
}


int
fdd_get_from_internal_name(const char *s)
{
    int c;

    for (c = 0; drive_types[c].internal_name != NULL; c++)
	if (! strcmp(drive_types[c].internal_name, s))
		return c;

    /* Not found. */
    return 0;
}


/* This is needed for the dump as 86F feature. */
void
fdd_do_seek(int drive, int track)
{
    if (drives[drive].seek)
	drives[drive].seek(drive, track);
}


void
fdd_forced_seek(int drive, int track_diff)
{
    fdd[drive].track += track_diff;

    if (fdd[drive].track < 0)
	fdd[drive].track = 0;

    if (fdd[drive].track > drive_types[fdd[drive].type].max_track)
	fdd[drive].track = drive_types[fdd[drive].type].max_track;

    fdd_do_seek(drive, fdd[drive].track);
}


void
fdd_seek(int drive, int track_diff)
{
    if (! track_diff)
	return;

    fdd[drive].track += track_diff;

    if (fdd[drive].track < 0)
	fdd[drive].track = 0;

    if (fdd[drive].track > drive_types[fdd[drive].type].max_track)
	fdd[drive].track = drive_types[fdd[drive].type].max_track;

    fdd_changed[drive] = 0;

    fdd_do_seek(drive, fdd[drive].track);
}


int
fdd_track0(int drive)
{
    /* If drive is disabled, TRK0 never gets set. */
    if (! drive_types[fdd[drive].type].max_track)
	return 0;

    return !fdd[drive].track;
}


int
fdd_current_track(int drive)
{
    return fdd[drive].track;
}


void
fdd_set_densel(int densel)
{
    int i;

    for (i = 0; i < 4; i++) {
	if (drive_types[fdd[i].type].flags & FLAG_INVERT_DENSEL)
		fdd[i].densel = densel ^ 1;
	else
		fdd[i].densel = densel;
    }
}


int
fdd_getrpm(int drive)
{
    int hole = fdd_hole(drive);
    int densel = 0;

    densel = fdd[drive].densel;

    if (drive_types[fdd[drive].type].flags & FLAG_INVERT_DENSEL)
	densel ^= 1;

    if (! (drive_types[fdd[drive].type].flags & FLAG_RPM_360))
	return 300;
    if (! (drive_types[fdd[drive].type].flags & FLAG_RPM_300))
	return 360;

    if (drive_types[fdd[drive].type].flags & FLAG_525) {
	return densel ? 360 : 300;
    } else {
	/* fdd_hole(drive) returns 0 for double density media, 1 for high density, and 2 for extended density. */
	if (hole == 1)
		return densel ? 300 : 360;
	else
		return 300;
    }
}


int
fdd_can_read_medium(int drive)
{
    int hole = fdd_hole(drive);

    hole = 1 << (hole + 4);

    return (drive_types[fdd[drive].type].flags & hole) ? 1 : 0;
}


int
fdd_doublestep_40(int drive)
{
    return (drive_types[fdd[drive].type].flags & FLAG_DOUBLE_STEP) ? 1 : 0;
}


void
fdd_set_type(int drive, int type)
{
    int old_type = fdd[drive].type;

    fdd[drive].type = type;
    if ((drive_types[old_type].flags ^ drive_types[type].flags) & FLAG_INVERT_DENSEL)
	fdd[drive].densel ^= 1;
}


int
fdd_get_type(int drive)
{
    return fdd[drive].type;
}


int
fdd_get_flags(int drive)
{
    return drive_types[fdd[drive].type].flags;
}


int
fdd_is_525(int drive)
{
    return drive_types[fdd[drive].type].flags & FLAG_525;
}


int
fdd_is_dd(int drive)
{
    return (drive_types[fdd[drive].type].flags & 0x70) == 0x10;
}


int
fdd_is_ed(int drive)
{
    return drive_types[fdd[drive].type].flags & FLAG_HOLE2;
}


int
fdd_is_double_sided(int drive)
{
    return drive_types[fdd[drive].type].flags & FLAG_DS;
}


void
fdd_set_head(int drive, int head)
{
    if (head && !fdd_is_double_sided(drive))
	fdd[drive].head = 0;
    else
	fdd[drive].head = head;
}


int
fdd_get_head(int drive)
{
    if (! fdd_is_double_sided(drive))
	return 0;

    return fdd[drive].head;
}


void
fdd_set_turbo(int drive, int turbo)
{
    fdd[drive].turbo = turbo;
}


int
fdd_get_turbo(int drive)
{
    return fdd[drive].turbo;
}


void
fdd_set_check_bpb(int drive, int check_bpb)
{
    fdd[drive].check_bpb = check_bpb;
}


int
fdd_get_check_bpb(int drive)
{
    return fdd[drive].check_bpb;
}


int
fdd_get_densel(int drive)
{
    return fdd[drive].densel;
}


int
fdd_load(int drive, const wchar_t *fn)
{
    int c = 0, size;
    int ok;
    wchar_t *p;
    FILE *f;

    DEBUG("FDD: loading drive %i with '%ls'\n", drive, fn);

    if (!fn) return(0);
    p = plat_get_extension(fn);
    if (!p) return(0);
    f = plat_fopen(fn, L"rb");
    if (!f) return(0);
    fseek(f, -1, SEEK_END);
    size = ftell(f) + 1;
    fclose(f);

    while (loaders[c].ext) {
	if (!wcscasecmp(p, loaders[c].ext) && (size == loaders[c].size || loaders[c].size == -1)) {
		driveloaders[drive] = c;
		wcsncpy(floppyfns[drive], fn, sizeof_w(floppyfns[drive]));
		d86f_setup(drive);
		ok = loaders[c].load(drive, floppyfns[drive]);
		if (! ok)
			goto no_load;

		drive_empty[drive] = 0;
		fdd_forced_seek(drive, 0);
		fdd_changed[drive] = 1;
		return(1);
	}

	c++;
    }

no_load:
    DEBUG("FDD: could not load '%ls' %s\n",fn,p);
    drive_empty[drive] = 1;
    fdd_set_head(drive, 0);
    memset(floppyfns[drive], 0, sizeof(floppyfns[drive]));
    ui_sb_icon_state(drive, 1);

    return 0;
}


void
fdd_close(int drive)
{
    DEBUG("FDD: closing drive %d\n", drive);

    /* Make sure the 86F poll is back to idle state. */
    d86f_stop(drive);

    if (loaders[driveloaders[drive]].close)
	loaders[driveloaders[drive]].close(drive);

    drive_empty[drive] = 1;

    fdd_set_head(drive, 0);

    floppyfns[drive][0] = 0;

    drives[drive].hole = NULL;
    drives[drive].poll = NULL;
    drives[drive].seek = NULL;
    drives[drive].readsector = NULL;
    drives[drive].writesector = NULL;
    drives[drive].comparesector = NULL;
    drives[drive].readaddress = NULL;
    drives[drive].format = NULL;
    drives[drive].byteperiod = NULL;
    drives[drive].stop = NULL;

    d86f_destroy(drive);

    ui_sb_icon_state(drive, 1);
}


int
fdd_hole(int drive)
{
    if (drives[drive].hole)
	return drives[drive].hole(drive);

    return 0;
}


double
fdd_byteperiod(int drive)
{
    if (drives[drive].byteperiod)
	return drives[drive].byteperiod(drive);

    return 32.0;
}


double
fdd_real_period(int drive)
{
    double ddbp;
    double dusec;

    ddbp = fdd_byteperiod(drive);
    dusec = (double)TIMER_USEC;

    /*
     * This is a giant hack but until the timings become even more
     * correct, this is needed to make floppies work right on thati
     * BIOS.
     */
    if (fdd_get_turbo(drive))
	return (32.0 * dusec);

    return (ddbp * dusec);
}


void
fdd_poll(int drive)
{
    if (drive >= FDD_NUM) {
	ERRLOG("FDD: polling impossible drive %i !\n", drive);
	return;
    }

    fdd_poll_time[drive] += (int64_t) fdd_real_period(drive);

    if (drives[drive].poll)
	drives[drive].poll(drive);

    if (fdd_notfound) {
	fdd_notfound--;
	if (! fdd_notfound)
		fdc_noidam(fdd_fdc);
    }
}


void
fdd_poll_0(void *priv)
{
    fdd_poll(0);
}


void
fdd_poll_1(void *priv)
{
    fdd_poll(1);
}


void
fdd_poll_2(void *priv)
{
    fdd_poll(2);
}


void
fdd_poll_3(void *priv)
{
    fdd_poll(3);
}


void
fdd_reset(void)
{
    INFO("FDD: reset\n");

    curdrive = 0;
    fdd_period = 32;

    timer_add(fdd_poll_0, NULL, &fdd_poll_time[0], &motoron[0]);
    timer_add(fdd_poll_1, NULL, &fdd_poll_time[1], &motoron[1]);
    timer_add(fdd_poll_2, NULL, &fdd_poll_time[2], &motoron[2]);
    timer_add(fdd_poll_3, NULL, &fdd_poll_time[3], &motoron[3]);
}


int
fdd_get_bitcell_period(int rate)
{
    int bit_rate = 250;

    switch (rate) {
	case 0: /*High density*/
		bit_rate = 500;
		break;

	case 1: /*Double density (360 rpm)*/
		bit_rate = 300;
		break;

	case 2: /*Double density*/
		bit_rate = 250;
		break;

	case 3: /*Extended density*/
		bit_rate = 1000;
		break;
    }

    return 1000000 / bit_rate*2; /*Bitcell period in ns*/
}


void
fdd_set_rate(int drive, int drvden, int rate)
{
    switch (rate) {
	case 0: /*High density*/
		fdd_period = 16;
		break;

	case 1:
		switch(drvden) {
			case 0: /*Double density (360 rpm)*/
				fdd_period = 26;
				break;

			case 1: /*High density (360 rpm)*/
				fdd_period = 16;
				break;

			case 2:
				fdd_period = 4;
				break;
		}
		break;

	case 2: /*Double density*/
		fdd_period = 32;
		break;

	case 3: /*Extended density*/
		fdd_period = 8;
		break;
    }
}


void
fdd_readsector(int drive, int sector, int track, int side, int density, int sector_size)
{
    if (drives[drive].readsector)
	drives[drive].readsector(drive, sector, track, side, density, sector_size);
    else
	fdd_notfound = 1000;
}


void
fdd_writesector(int drive, int sector, int track, int side, int density, int sector_size)
{
    if (drives[drive].writesector)
	drives[drive].writesector(drive, sector, track, side, density, sector_size);
    else
	fdd_notfound = 1000;
}


void
fdd_comparesector(int drive, int sector, int track, int side, int density, int sector_size)
{
    if (drives[drive].comparesector)
	drives[drive].comparesector(drive, sector, track, side, density, sector_size);
    else
	fdd_notfound = 1000;
}


void
fdd_readaddress(int drive, int side, int density)
{
    if (drives[drive].readaddress)
	drives[drive].readaddress(drive, side, density);
}


void
fdd_format(int drive, int side, int density, uint8_t fill)
{
    if (drives[drive].format)
	drives[drive].format(drive, side, density, fill);
    else
	fdd_notfound = 1000;
}


void
fdd_stop(int drive)
{
    if (drives[drive].stop)
	drives[drive].stop(drive);
}


void
fdd_set_fdc(void *fdc)
{
    fdd_fdc = (fdc_t *) fdc;
}
