/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the "Removable Devices" dialog.
 *
 * Version:	@(#)win_settings_remov.h	1.0.14	2019/05/03
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2017-2019 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
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


/************************************************************************
 *									*
 *			  Removable Devices Dialog			*
 *									*
 ************************************************************************/

/* GLobal variables needed for the Removable Devices dialog. */
static int rd_ignore_change = 0;
static int cdlv_current_sel;
static int zdlv_current_sel;
static int modlv_current_sel;


static void
cdrom_track_init(void)
{
    int i;

    for (i = 0; i < CDROM_NUM; i++) {
	if (cdrom[i].bus_type == CDROM_BUS_ATAPI)
		ide_tracking |= (2 << (cdrom[i].bus_id.ide_channel << 3));
	else if (cdrom[i].bus_type == CDROM_BUS_SCSI)
		scsi_tracking[cdrom[i].bus_id.scsi.id] |= (2 << (cdrom[i].bus_id.scsi.lun << 3));
    }
}


static int
combo_to_string(int id)
{
    if (id == 0)
	return(IDS_DISABLED);

    return(IDS_3580 + id - 1);
}


static void
id_to_combo(HWND h, int num)
{
    WCHAR temp[16];
    int i;

    for (i = 0; i < num; i++) {
	swprintf(temp, sizeof_w(temp), L"%i", i);
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)temp);
    }
}


static void
cdrom_image_list_init(HWND hwndList)
{
    HICON hiconItem;
    HIMAGELIST hSmall;

    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
			      GetSystemMetrics(SM_CYSMICON),
			      ILC_MASK | ILC_COLOR32, 1, 1);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_CDROM_D);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_CDROM);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
}


static BOOL
cdrom_recalc_list(HWND hwndList)
{
    WCHAR temp[128];
    LVITEM lvI;
    int fsid, i;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    for (i = 0; i < CDROM_NUM; i++) {
	fsid = temp_cdrom_drives[i].bus_type;

	lvI.iSubItem = 0;
	switch (fsid) {
		case CDROM_BUS_DISABLED:
		default:
			lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
			lvI.iImage = 0;
			break;

		case CDROM_BUS_ATAPI:
			swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
				 get_string(combo_to_string(fsid)),
			         temp_cdrom_drives[i].bus_id.ide_channel >> 1,
				 temp_cdrom_drives[i].bus_id.ide_channel & 1);
			lvI.pszText = temp;
			lvI.iImage = 1;
			break;

		case CDROM_BUS_SCSI:
			swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
				 get_string(combo_to_string(fsid)),
				 temp_cdrom_drives[i].bus_id.scsi.id,
				 temp_cdrom_drives[i].bus_id.scsi.lun);
			lvI.pszText = temp;
			lvI.iImage = 1;
			break;
	}
	lvI.iItem = i;
	if (ListView_InsertItem(hwndList, &lvI) == -1)
		return FALSE;

	lvI.iSubItem = 1;
	if (fsid == CDROM_BUS_DISABLED) {
		lvI.pszText = (LPTSTR)get_string(IDS_NONE);
	} else {
		wsprintf(temp, L"%ix",
			 cdrom_speeds[temp_cdrom_drives[i].speed_idx].speed);
		lvI.pszText = temp;
	}
	lvI.iItem = i;
	lvI.iImage = 0;
	if (ListView_SetItem(hwndList, &lvI) == -1)
		return FALSE;
    }

    return TRUE;
}


static BOOL
cdrom_init_columns(HWND hwndList)
{
    LVCOLUMN lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPTSTR)get_string(IDS_BUS);
    lvc.cx = 342;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 0, &lvc) == -1)
	return FALSE;

    lvc.iSubItem = 1;
    lvc.pszText = (LPTSTR)get_string(IDS_3579);
    lvc.cx = 80;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 1, &lvc) == -1)
	return FALSE;

    return TRUE;
}


static int
cdrom_get_selected(HWND hdlg)
{
    int drive = -1;
    int i, j = 0;
    HWND h;

    for (i = 0; i < 6; i++) {
	h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
	j = ListView_GetItemState(h, i, LVIS_SELECTED);
	if (j)
		drive = i;
    }

    return drive;
}


static void
cdrom_update_item(HWND hwndList, int i)
{
    WCHAR temp[128];
    LVITEM lvI;
    int fsid;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    lvI.iSubItem = 0;
    lvI.iItem = i;

    fsid = temp_cdrom_drives[i].bus_type;

    switch (fsid) {
	case CDROM_BUS_DISABLED:
	default:
		lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
		lvI.iImage = 0;
		break;

	case CDROM_BUS_ATAPI:
		swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
			 get_string(combo_to_string(fsid)),
			 temp_cdrom_drives[i].bus_id.ide_channel >> 1,
			 temp_cdrom_drives[i].bus_id.ide_channel & 1);
		lvI.pszText = temp;
		lvI.iImage = 1;
		break;

	case CDROM_BUS_SCSI:
		swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
			 get_string(combo_to_string(fsid)),
			 temp_cdrom_drives[i].bus_id.scsi.id,
			 temp_cdrom_drives[i].bus_id.scsi.lun);
		lvI.pszText = temp;
		lvI.iImage = 1;
		break;
    }
    if (ListView_SetItem(hwndList, &lvI) == -1)
	return;

    lvI.iSubItem = 1;
    if (fsid == CDROM_BUS_DISABLED) {
	lvI.pszText = (LPTSTR)get_string(IDS_NONE);
    } else {
	wsprintf(temp, L"%ix",
		 cdrom_speeds[temp_cdrom_drives[i].speed_idx].speed);
	lvI.pszText = temp;
    }
    lvI.iItem = i;
    lvI.iImage = 0;
    if (ListView_SetItem(hwndList, &lvI) == -1)
	return;
}


static void
cdrom_add_locations(HWND hdlg)
{
    WCHAR temp[128];
    HWND h;
    int i = 0;

    h = GetDlgItem(hdlg, IDC_COMBO_CD_BUS);
    for (i = CDROM_BUS_DISABLED; i < CDROM_BUS_MAX; i++)
	SendMessage(h, CB_ADDSTRING, 0, win_string(combo_to_string(i)));

    /* Create a list of usable CD-ROM speeds. */
    h = GetDlgItem(hdlg, IDC_COMBO_CD_SPEED);
    for (i = 0; cdrom_speeds[i].speed > 0; i++) {
	swprintf(temp, sizeof_w(temp), L"%ix", cdrom_speeds[i].speed);
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)temp);
    }

    h = GetDlgItem(hdlg, IDC_COMBO_CD_ID);
    id_to_combo(h, 16);

    h = GetDlgItem(hdlg, IDC_COMBO_CD_LUN);
    id_to_combo(h, 8);

    h = GetDlgItem(hdlg, IDC_COMBO_CD_CHANNEL_IDE);
    for (i = 0; i < 8; i++) {
	swprintf(temp, sizeof_w(temp), L"%01i:%01i", i >> 1, i & 1);
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)temp);
    }
}


static void
cdrom_recalc_location_controls(HWND hdlg, int assign_id)
{
    HWND h;
    int i;
    int bus = temp_cdrom_drives[cdlv_current_sel].bus_type;

    for (i = IDT_1741; i < (IDT_1743 + 1); i++) {
	h = GetDlgItem(hdlg, i);
	EnableWindow(h, FALSE);
	ShowWindow(h, SW_HIDE);
    }

    h = GetDlgItem(hdlg, IDC_COMBO_CD_ID);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_CD_LUN);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_CD_CHANNEL_IDE);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_CD_SPEED);
    if (bus == CDROM_BUS_DISABLED) {
	EnableWindow(h, FALSE);
	ShowWindow(h, SW_HIDE);
    } else {
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	SendMessage(h, CB_SETCURSEL,
		    temp_cdrom_drives[cdlv_current_sel].speed_idx, 0);
    }

    switch(bus) {
	case CDROM_BUS_ATAPI:
		h = GetDlgItem(hdlg, IDT_1743);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		if (assign_id)
			temp_cdrom_drives[cdlv_current_sel].bus_id.ide_channel = next_free_ide_channel();

		h = GetDlgItem(hdlg, IDC_COMBO_CD_CHANNEL_IDE);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			    temp_cdrom_drives[cdlv_current_sel].bus_id.ide_channel, 0);
		break;

	case CDROM_BUS_SCSI:
		h = GetDlgItem(hdlg, IDT_1741);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		h = GetDlgItem(hdlg, IDT_1742);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		if (assign_id)
			next_free_scsi_id_and_lun((uint8_t *) &temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.id, (uint8_t *) &temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.lun);

		h = GetDlgItem(hdlg, IDC_COMBO_CD_ID);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.id, 0);

		h = GetDlgItem(hdlg, IDC_COMBO_CD_LUN);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.lun, 0);
		break;
    }
}


static void
cdrom_track(uint8_t id)
{
    if (temp_cdrom_drives[id].bus_type == CDROM_BUS_ATAPI)
	ide_tracking |= (2 << (temp_cdrom_drives[id].bus_id.ide_channel << 3));
    else if (temp_cdrom_drives[id].bus_type == CDROM_BUS_SCSI)
	scsi_tracking[temp_cdrom_drives[id].bus_id.scsi.id] |= (1ULL << temp_cdrom_drives[id].bus_id.scsi.lun);
}


static void
cdrom_untrack(uint8_t id)
{
    if (temp_cdrom_drives[id].bus_type == CDROM_BUS_ATAPI)
	ide_tracking &= ~(2 << (temp_cdrom_drives[id].bus_id.ide_channel << 3));
    else if (temp_cdrom_drives[id].bus_type == CDROM_BUS_SCSI)
	scsi_tracking[temp_cdrom_drives[id].bus_id.scsi.id] &= ~(1ULL << temp_cdrom_drives[id].bus_id.scsi.lun);
}


#if 0
static void
cdrom_track_all(void)
{
    int i;

    for (i = 0; i < CDROM_NUM; i++)
	cdrom_track(i);
}
#endif


static void
zip_track_init(void)
{
    int i;

    for (i = 0; i < ZIP_NUM; i++) {
	if (zip_drives[i].bus_type == ZIP_BUS_ATAPI)
		ide_tracking |= (4 << (zip_drives[i].bus_id.ide_channel << 3));
	else if (zip_drives[i].bus_type == ZIP_BUS_SCSI)
		scsi_tracking[zip_drives[i].bus_id.scsi.id] |= (4 << (zip_drives[i].bus_id.scsi.lun << 3));
    }
}


static void
zip_image_list_init(HWND hwndList)
{
    HICON hiconItem;
    HIMAGELIST hSmall;

    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
			      GetSystemMetrics(SM_CYSMICON),
			      ILC_MASK | ILC_COLOR32, 1, 1);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_ZIP_D);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_ZIP);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
}


static BOOL
zip_recalc_list(HWND hwndList)
{
    WCHAR temp[256];
    LVITEM lvI;
    int fsid, i;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    for (i = 0; i < ZIP_NUM; i++) {
	fsid = temp_zip_drives[i].bus_type;

	lvI.iSubItem = 0;
	switch (fsid) {
		case ZIP_BUS_DISABLED:
		default:
			lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
			lvI.iImage = 0;
			break;

		case ZIP_BUS_ATAPI:
			swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
				get_string(combo_to_string(fsid)),
				temp_zip_drives[i].bus_id.ide_channel >> 1,
				temp_zip_drives[i].bus_id.ide_channel & 1);
			lvI.pszText = temp;
			lvI.iImage = 1;
			break;

		case ZIP_BUS_SCSI:
			swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
				 get_string(combo_to_string(fsid)),
				 temp_zip_drives[i].bus_id.scsi.id,
				 temp_zip_drives[i].bus_id.scsi.lun);
			lvI.pszText = temp;
			lvI.iImage = 1;
			break;
	}
	lvI.iItem = i;
	if (ListView_InsertItem(hwndList, &lvI) == -1)
		return FALSE;

	lvI.iSubItem = 1;
	lvI.pszText = (LPTSTR)get_string(temp_zip_drives[i].is_250 ? IDS_3295 : IDS_3294);
	lvI.iItem = i;
	lvI.iImage = 0;
	if (ListView_SetItem(hwndList, &lvI) == -1)
		return FALSE;
    }

    return TRUE;
}


static BOOL
zip_init_columns(HWND hwndList)
{
    LVCOLUMN lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPTSTR)get_string(IDS_BUS);
    lvc.cx = 342;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 0, &lvc) == -1)
	return FALSE;

    lvc.iSubItem = 1;
    lvc.pszText = (LPTSTR)get_string(IDS_TYPE);
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 1, &lvc) == -1)
	return FALSE;

    return TRUE;
}


static int
zip_get_selected(HWND hdlg)
{
    int drive = -1;
    int i, j = 0;
    HWND h;

    for (i = 0; i < 6; i++) {	// FIXME: ZIP_NUM ?
	h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	j = ListView_GetItemState(h, i, LVIS_SELECTED);
	if (j)
		drive = i;
    }

    return drive;
}


static void
zip_update_item(HWND hwndList, int i)
{
    WCHAR temp[128];
    LVITEM lvI;
    int fsid;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    lvI.iSubItem = 0;
    lvI.iItem = i;

    fsid = temp_zip_drives[i].bus_type;

    switch (fsid) {
	case ZIP_BUS_DISABLED:
	default:
		lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
		lvI.iImage = 0;
		break;

	case ZIP_BUS_ATAPI:
		swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
			 get_string(combo_to_string(fsid)),
			 temp_zip_drives[i].bus_id.ide_channel >> 1,
			 temp_zip_drives[i].bus_id.ide_channel & 1);
		lvI.pszText = temp;
		lvI.iImage = 1;
		break;

	case ZIP_BUS_SCSI:
		swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
			 get_string(combo_to_string(fsid)),
			 temp_zip_drives[i].bus_id.scsi.id,
			 temp_zip_drives[i].bus_id.scsi.lun);
		lvI.pszText = temp;
		lvI.iImage = 1;
		break;
    }
    if (ListView_SetItem(hwndList, &lvI) == -1)
	return;

    lvI.iSubItem = 1;
    lvI.pszText = (LPTSTR)get_string(temp_zip_drives[i].is_250 ? IDS_3295 : IDS_3294);
    lvI.iItem = i;
    lvI.iImage = 0;
    if (ListView_SetItem(hwndList, &lvI) == -1)
	return;
}


static void
zip_add_locations(HWND hdlg)
{
    WCHAR temp[128];
    HWND h;
    int i;

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_BUS);
    for (i = ZIP_BUS_DISABLED; i < ZIP_BUS_MAX; i++)
	SendMessage(h, CB_ADDSTRING, 0, win_string(combo_to_string(i)));

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_ID);
    id_to_combo(h, 16);

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_LUN);
    id_to_combo(h, 8);

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_CHANNEL_IDE);
    for (i = 0; i < 8; i++) {
	swprintf(temp, sizeof_w(temp), L"%01i:%01i", i >> 1, i & 1);
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)temp);
    }
}


static void
zip_recalc_location_controls(HWND hdlg, int assign_id)
{
    HWND h;
    int i;
    int bus = temp_zip_drives[zdlv_current_sel].bus_type;

    for (i = IDT_1754; i < (IDT_1756 + 1); i++) {
	h = GetDlgItem(hdlg, i);
	EnableWindow(h, FALSE);
	ShowWindow(h, SW_HIDE);
    }

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_ID);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_LUN);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_CHANNEL_IDE);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    switch(bus) {
	case ZIP_BUS_ATAPI:
		h = GetDlgItem(hdlg, IDT_1756);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);

		if (assign_id)
			temp_zip_drives[zdlv_current_sel].bus_id.ide_channel = next_free_ide_channel();

		h = GetDlgItem(hdlg, IDC_COMBO_ZIP_CHANNEL_IDE);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			    temp_zip_drives[zdlv_current_sel].bus_id.ide_channel, 0);
		break;

	case ZIP_BUS_SCSI:
		h = GetDlgItem(hdlg, IDT_1754);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		h = GetDlgItem(hdlg, IDT_1755);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		if (assign_id)
			next_free_scsi_id_and_lun((uint8_t *) &temp_zip_drives[zdlv_current_sel].bus_id.scsi.id, (uint8_t *) &temp_zip_drives[zdlv_current_sel].bus_id.scsi.lun);

		h = GetDlgItem(hdlg, IDC_COMBO_ZIP_ID);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			temp_zip_drives[zdlv_current_sel].bus_id.scsi.id, 0);

		h = GetDlgItem(hdlg, IDC_COMBO_ZIP_LUN);
		ShowWindow(h, SW_SHOW);
		EnableWindow(h, TRUE);
		SendMessage(h, CB_SETCURSEL,
			temp_zip_drives[zdlv_current_sel].bus_id.scsi.lun, 0);
		break;
    }
}


static void
zip_track(uint8_t id)
{
    if (temp_zip_drives[id].bus_type == ZIP_BUS_ATAPI)
	ide_tracking |= (1ULL << temp_zip_drives[id].bus_id.ide_channel);
    else if (temp_zip_drives[id].bus_type == ZIP_BUS_SCSI)
	scsi_tracking[temp_zip_drives[id].bus_id.scsi.id] |= (1ULL << temp_zip_drives[id].bus_id.scsi.lun);
}


static void
zip_untrack(uint8_t id)
{
    if (temp_zip_drives[id].bus_type == ZIP_BUS_ATAPI)
	ide_tracking &= ~(1ULL << temp_zip_drives[id].bus_id.ide_channel);
    else if (temp_zip_drives[id].bus_type == ZIP_BUS_SCSI)
	scsi_tracking[temp_zip_drives[id].bus_id.scsi.id] &= ~(1ULL << temp_zip_drives[id].bus_id.scsi.lun);
}


#if 0
static void
zip_track_all(void)
{
    int i;

    for (i = 0; i < ZIP_NUM; i++)
	zip_track(i);
}
#endif


static WIN_RESULT CALLBACK
mmc_devices_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND h = NULL;
    int old_sel = 0;
    int assign = 0;
    int b;

    switch (message) {
	case WM_INITDIALOG:
		rd_ignore_change = 1;

		cdlv_current_sel = 0;
		h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
		cdrom_init_columns(h);
		cdrom_image_list_init(h);
		cdrom_recalc_list(h);
		ListView_SetItemState(h, 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		cdrom_add_locations(hdlg);
		h = GetDlgItem(hdlg, IDC_COMBO_CD_BUS);
		b = temp_cdrom_drives[cdlv_current_sel].bus_type;
		SendMessage(h, CB_SETCURSEL, b, 0);
		cdrom_recalc_location_controls(hdlg, 0);

	case WM_NOTIFY:
		if (rd_ignore_change)
			return FALSE;

		if ((((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) && (((LPNMHDR)lParam)->idFrom == IDC_LIST_CDROM_DRIVES)) {
			old_sel = cdlv_current_sel;
			h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
			cdlv_current_sel = cdrom_get_selected(hdlg);
			if (cdlv_current_sel == old_sel)
				return FALSE;
			if (cdlv_current_sel == -1) {
				rd_ignore_change = 1;
				cdlv_current_sel = old_sel;
				ListView_SetItemState(h, cdlv_current_sel, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				rd_ignore_change = 0;
				return FALSE;
			}
			rd_ignore_change = 1;

			h = GetDlgItem(hdlg, IDC_COMBO_CD_BUS);
			b = temp_cdrom_drives[cdlv_current_sel].bus_type;
			SendMessage(h, CB_SETCURSEL, b, 0);
			cdrom_recalc_location_controls(hdlg, 0);
		}
		rd_ignore_change = 0;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDC_COMBO_CD_BUS:
				if (rd_ignore_change)
					return FALSE;

				rd_ignore_change = 1;
				h = GetDlgItem(hdlg, IDC_COMBO_CD_BUS);
				b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
				if (b == temp_cdrom_drives[cdlv_current_sel].bus_type)
					goto cdrom_bus_skip;
				cdrom_untrack(cdlv_current_sel);
				assign = (temp_cdrom_drives[cdlv_current_sel].bus_type == b) ? 0 : 1;
				if (temp_cdrom_drives[cdlv_current_sel].bus_type == CDROM_BUS_DISABLED)
					temp_cdrom_drives[cdlv_current_sel].speed_idx = cdrom_speed_idx(cdrom_speed_idx(CDROM_SPEED_DEFAULT));
				temp_cdrom_drives[cdlv_current_sel].bus_type = b;
				cdrom_recalc_location_controls(hdlg, assign);
				cdrom_track(cdlv_current_sel);
				h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
				cdrom_update_item(h, cdlv_current_sel);
cdrom_bus_skip:
				rd_ignore_change = 0;
				return FALSE;

			case IDC_COMBO_CD_ID:
				if (rd_ignore_change)
					return FALSE;

				rd_ignore_change = 1;
				h = GetDlgItem(hdlg, IDC_COMBO_CD_ID);
				cdrom_untrack(cdlv_current_sel);
				b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
				temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.id = (uint8_t)b;
				cdrom_track(cdlv_current_sel);
				h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
				cdrom_update_item(h, cdlv_current_sel);
				rd_ignore_change = 0;
				return FALSE;

			case IDC_COMBO_CD_LUN:
				if (rd_ignore_change)
					return FALSE;

				rd_ignore_change = 1;
				h = GetDlgItem(hdlg, IDC_COMBO_CD_LUN);
				cdrom_untrack(cdlv_current_sel);
				b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
				temp_cdrom_drives[cdlv_current_sel].bus_id.scsi.lun = (uint8_t)b;
				cdrom_track(cdlv_current_sel);
				h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
				cdrom_update_item(h, cdlv_current_sel);
				rd_ignore_change = 0;
				return FALSE;

			case IDC_COMBO_CD_CHANNEL_IDE:
				if (rd_ignore_change)
					return FALSE;

				rd_ignore_change = 1;
				h = GetDlgItem(hdlg, IDC_COMBO_CD_CHANNEL_IDE);
				cdrom_untrack(cdlv_current_sel);
				b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
				temp_cdrom_drives[cdlv_current_sel].bus_id.ide_channel = (uint8_t)b;
				cdrom_track(cdlv_current_sel);
				h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
				cdrom_update_item(h, cdlv_current_sel);
				rd_ignore_change = 0;
				return FALSE;

			case IDC_COMBO_CD_SPEED:
				if (rd_ignore_change)
					return FALSE;

				rd_ignore_change = 1;
				h = GetDlgItem(hdlg, IDC_COMBO_CD_SPEED);
				b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
				temp_cdrom_drives[cdlv_current_sel].speed_idx = (uint8_t)b;
				h = GetDlgItem(hdlg, IDC_LIST_CDROM_DRIVES);
				cdrom_update_item(h, cdlv_current_sel);
				rd_ignore_change = 0;
				return FALSE;

		}
		return FALSE;

	default:
		break;
    }

    return FALSE;
}

static WIN_RESULT CALLBACK
iomega_devices_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND h = NULL;
    int old_sel = 0;
    int assign = 0;
    int b;

    switch (message) {
    case WM_INITDIALOG:
	rd_ignore_change = 1;

	zdlv_current_sel = 0;
	h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	zip_init_columns(h);
	zip_image_list_init(h);
	zip_recalc_list(h);
	zip_add_locations(hdlg);
	h = GetDlgItem(hdlg, IDC_COMBO_ZIP_BUS);
	b = temp_zip_drives[zdlv_current_sel].bus_type;
	SendMessage(h, CB_SETCURSEL, b, 0);
	zip_recalc_location_controls(hdlg, 0);
	h = GetDlgItem(hdlg, IDC_CHECK250);
	SendMessage(h, BM_SETCHECK,
	    temp_zip_drives[zdlv_current_sel].is_250, 0);
	rd_ignore_change = 0;
	return TRUE;

    case WM_NOTIFY:
	if (rd_ignore_change)
	    return FALSE;

	if ((((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) && (((LPNMHDR)lParam)->idFrom == IDC_LIST_ZIP_DRIVES)) {
	    old_sel = zdlv_current_sel;
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zdlv_current_sel = zip_get_selected(hdlg);
	    if (zdlv_current_sel == old_sel)
		return FALSE;

	    if (zdlv_current_sel == -1) {
		rd_ignore_change = 1;
		zdlv_current_sel = old_sel;
		ListView_SetItemState(h, zdlv_current_sel, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		rd_ignore_change = 0;
		return FALSE;
	    }
	    rd_ignore_change = 1;

	    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_BUS);
	    b = temp_zip_drives[zdlv_current_sel].bus_type;
	    SendMessage(h, CB_SETCURSEL, b, 0);
	    zip_recalc_location_controls(hdlg, 0);
	    h = GetDlgItem(hdlg, IDC_CHECK250);
	    SendMessage(h, BM_SETCHECK,
		temp_zip_drives[zdlv_current_sel].is_250, 0);
	}
	rd_ignore_change = 0;
	break;

    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDC_COMBO_ZIP_BUS:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_BUS);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    if (temp_zip_drives[zdlv_current_sel].bus_type == b)
		goto zip_bus_skip;
	    zip_untrack(zdlv_current_sel);
	    assign = (temp_zip_drives[zdlv_current_sel].bus_type == b) ? 0 : 1;
	    temp_zip_drives[zdlv_current_sel].bus_type = b;
	    zip_recalc_location_controls(hdlg, assign);
	    zip_track(zdlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zip_update_item(h, zdlv_current_sel);
	zip_bus_skip:
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_ZIP_ID:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_ID);
	    zip_untrack(zdlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_zip_drives[zdlv_current_sel].bus_id.scsi.id = (uint8_t)b;
	    zip_track(zdlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zip_update_item(h, zdlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_ZIP_LUN:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_LUN);
	    zip_untrack(zdlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_zip_drives[zdlv_current_sel].bus_id.scsi.lun = (uint8_t)b;
	    zip_track(zdlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zip_update_item(h, zdlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_ZIP_CHANNEL_IDE:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_ZIP_CHANNEL_IDE);
	    zip_untrack(zdlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_zip_drives[zdlv_current_sel].bus_id.ide_channel = (uint8_t)b;
	    zip_track(zdlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zip_update_item(h, zdlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_CHECK250:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_CHECK250);
	    b = (int)SendMessage(h, BM_GETCHECK, 0, 0);
	    temp_zip_drives[zdlv_current_sel].is_250 = (int8_t)b;
	    h = GetDlgItem(hdlg, IDC_LIST_ZIP_DRIVES);
	    zip_update_item(h, zdlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;
	}
	return FALSE;

    default:
	break;
    }

    return FALSE;
}

static void
mo_track_init(void)
{
    int i;

    for (i = 0; i < MO_NUM; i++) {
	if (mo_drives[i].bus_type == MO_BUS_ATAPI)
	    ide_tracking |= (4 << (mo_drives[i].bus_id.ide_channel << 3));
	else if (mo_drives[i].bus_type == MO_BUS_SCSI)
	    scsi_tracking[mo_drives[i].bus_id.scsi.id] |= (4 << (mo_drives[i].bus_id.scsi.lun << 3));
    }
}


static void
mo_image_list_init(HWND hwndList)
{
    HICON hiconItem;
    HIMAGELIST hSmall;

    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
	GetSystemMetrics(SM_CYSMICON),
	ILC_MASK | ILC_COLOR32, 1, 1);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_MO_D);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    hiconItem = LoadIcon(hInstance, (LPCWSTR)ICON_MO);
    ImageList_AddIcon(hSmall, hiconItem);
    DestroyIcon(hiconItem);

    ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
}


static BOOL
mo_recalc_list(HWND hwndList)
{
    WCHAR temp[256];
    LVITEM lvI;
    int fsid, i;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    for (i = 0; i < MO_NUM; i++) {
	fsid = temp_mo_drives[i].bus_type;

	lvI.iSubItem = 0;
	switch (fsid) {
	case MO_BUS_DISABLED:
	default:
	    lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
	    lvI.iImage = 0;
	    break;

	case MO_BUS_ATAPI:
	    swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
		get_string(combo_to_string(fsid)),
		temp_mo_drives[i].bus_id.ide_channel >> 1,
		temp_mo_drives[i].bus_id.ide_channel & 1);
	    lvI.pszText = temp;
	    lvI.iImage = 1;
	    break;

	case MO_BUS_SCSI:
	    swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
		get_string(combo_to_string(fsid)),
		temp_mo_drives[i].bus_id.scsi.id,
		temp_mo_drives[i].bus_id.scsi.lun);
	    lvI.pszText = temp;
	    lvI.iImage = 1;
	    break;
	}
	lvI.iItem = i;
	if (ListView_InsertItem(hwndList, &lvI) == -1)
	    return FALSE;
    }

    return TRUE;
}


static BOOL
mo_init_columns(HWND hwndList)
{
    LVCOLUMN lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = (LPTSTR)get_string(IDS_BUS);
    lvc.cx = 342;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 0, &lvc) == -1)
	return FALSE;

    lvc.iSubItem = 1;
    lvc.pszText = (LPTSTR)get_string(IDS_TYPE);
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_InsertColumn(hwndList, 1, &lvc) == -1)
	return FALSE;

    return TRUE;
}


static int
mo_get_selected(HWND hdlg)
{
    int drive = -1;
    int i, j = 0;
    HWND h;

    for (i = 0; i < 6; i++) {	// FIXME: MO_NUM ?
	h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	j = ListView_GetItemState(h, i, LVIS_SELECTED);
	if (j)
	    drive = i;
    }

    return drive;
}


static void
mo_update_item(HWND hwndList, int i)
{
    WCHAR temp[128];
    LVITEM lvI;
    int fsid;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = lvI.iSubItem = lvI.state = 0;

    lvI.iSubItem = 0;
    lvI.iItem = i;

    fsid = temp_mo_drives[i].bus_type;

    switch (fsid) {
    case MO_BUS_DISABLED:
    default:
	lvI.pszText = (LPTSTR)get_string(IDS_DISABLED);
	lvI.iImage = 0;
	break;

    case MO_BUS_ATAPI:
	swprintf(temp, sizeof_w(temp), L"%ls (%01i:%01i)",
	    get_string(combo_to_string(fsid)),
	    temp_mo_drives[i].bus_id.ide_channel >> 1,
	    temp_mo_drives[i].bus_id.ide_channel & 1);
	lvI.pszText = temp;
	lvI.iImage = 1;
	break;

    case MO_BUS_SCSI:
	swprintf(temp, sizeof_w(temp), L"%ls (%02i:%02i)",
	    get_string(combo_to_string(fsid)),
	    temp_mo_drives[i].bus_id.scsi.id,
	    temp_mo_drives[i].bus_id.scsi.lun);
	lvI.pszText = temp;
	lvI.iImage = 1;
	break;
    }
    if (ListView_SetItem(hwndList, &lvI) == -1)
	return;
}


static void
mo_add_locations(HWND hdlg)
{
    WCHAR temp[128];
    HWND h;
    int i;

    h = GetDlgItem(hdlg, IDC_COMBO_MO_BUS);
    for (i = MO_BUS_DISABLED; i < MO_BUS_MAX; i++)
	SendMessage(h, CB_ADDSTRING, 0, win_string(combo_to_string(i)));

    h = GetDlgItem(hdlg, IDC_COMBO_MO_ID);
    id_to_combo(h, 16);

    h = GetDlgItem(hdlg, IDC_COMBO_MO_LUN);
    id_to_combo(h, 8);

    h = GetDlgItem(hdlg, IDC_COMBO_MO_CHANNEL_IDE);
    for (i = 0; i < 8; i++) {
	swprintf(temp, sizeof_w(temp), L"%01i:%01i", i >> 1, i & 1);
	SendMessage(h, CB_ADDSTRING, 0, (LPARAM)temp);
    }
}


static void
mo_recalc_location_controls(HWND hdlg, int assign_id)
{
    HWND h;
    int i;
    int bus = temp_mo_drives[modlv_current_sel].bus_type;

    for (i = IDT_1754; i < (IDT_1756 + 1); i++) {
	h = GetDlgItem(hdlg, i);
	EnableWindow(h, FALSE);
	ShowWindow(h, SW_HIDE);
    }

    h = GetDlgItem(hdlg, IDC_COMBO_MO_ID);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_MO_LUN);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    h = GetDlgItem(hdlg, IDC_COMBO_MO_CHANNEL_IDE);
    EnableWindow(h, FALSE);
    ShowWindow(h, SW_HIDE);

    switch (bus) {
    case MO_BUS_ATAPI:
	h = GetDlgItem(hdlg, IDT_1756);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);

	if (assign_id)
	    temp_mo_drives[modlv_current_sel].bus_id.ide_channel = next_free_ide_channel();

	h = GetDlgItem(hdlg, IDC_COMBO_MO_CHANNEL_IDE);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	SendMessage(h, CB_SETCURSEL,
	    temp_mo_drives[modlv_current_sel].bus_id.ide_channel, 0);
	break;

    case MO_BUS_SCSI:
	h = GetDlgItem(hdlg, IDT_1754);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	h = GetDlgItem(hdlg, IDT_1755);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	if (assign_id)
	    next_free_scsi_id_and_lun((uint8_t*)&temp_mo_drives[modlv_current_sel].bus_id.scsi.id, (uint8_t*)&temp_mo_drives[modlv_current_sel].bus_id.scsi.lun);

	h = GetDlgItem(hdlg, IDC_COMBO_MO_ID);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	SendMessage(h, CB_SETCURSEL,
	    temp_mo_drives[modlv_current_sel].bus_id.scsi.id, 0);

	h = GetDlgItem(hdlg, IDC_COMBO_MO_LUN);
	ShowWindow(h, SW_SHOW);
	EnableWindow(h, TRUE);
	SendMessage(h, CB_SETCURSEL,
	    temp_mo_drives[modlv_current_sel].bus_id.scsi.lun, 0);
	break;
    }
}


static void
mo_track(uint8_t id)
{
    if (temp_mo_drives[id].bus_type == MO_BUS_ATAPI)
	ide_tracking |= (1ULL << temp_mo_drives[id].bus_id.ide_channel);
    else if (temp_mo_drives[id].bus_type == MO_BUS_SCSI)
	scsi_tracking[temp_mo_drives[id].bus_id.scsi.id] |= (1ULL << temp_mo_drives[id].bus_id.scsi.lun);
}


static void
mo_untrack(uint8_t id)
{
    if (temp_mo_drives[id].bus_type == MO_BUS_ATAPI)
	ide_tracking &= ~(1ULL << temp_mo_drives[id].bus_id.ide_channel);
    else if (temp_mo_drives[id].bus_type == MO_BUS_SCSI)
	scsi_tracking[temp_mo_drives[id].bus_id.scsi.id] &= ~(1ULL << temp_mo_drives[id].bus_id.scsi.lun);
}

static WIN_RESULT CALLBACK
mo_devices_proc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND h = NULL;
    int old_sel = 0;
    int assign = 0;
    int b;

    switch (message) {
    case WM_INITDIALOG:
	rd_ignore_change = 1;

	zdlv_current_sel = 0;
	h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	mo_init_columns(h);
	mo_image_list_init(h);
	mo_recalc_list(h);
	mo_add_locations(hdlg);
	h = GetDlgItem(hdlg, IDC_COMBO_MO_BUS);
	b = temp_mo_drives[modlv_current_sel].bus_type;
	SendMessage(h, CB_SETCURSEL, b, 0);
	mo_recalc_location_controls(hdlg, 0);
	rd_ignore_change = 0;
	return TRUE;

    case WM_NOTIFY:
	if (rd_ignore_change)
	    return FALSE;

	if ((((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) && (((LPNMHDR)lParam)->idFrom == IDC_LIST_MO_DRIVES)) {
	    old_sel = modlv_current_sel;
	    h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	    modlv_current_sel = mo_get_selected(hdlg);
	    if (modlv_current_sel == old_sel)
		return FALSE;

	    if (modlv_current_sel == -1) {
		rd_ignore_change = 1;
		modlv_current_sel = old_sel;
		ListView_SetItemState(h, modlv_current_sel, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
		rd_ignore_change = 0;
		return FALSE;
	    }
	    rd_ignore_change = 1;

	    h = GetDlgItem(hdlg, IDC_COMBO_MO_BUS);
	    b = temp_mo_drives[modlv_current_sel].bus_type;
	    SendMessage(h, CB_SETCURSEL, b, 0);
	    mo_recalc_location_controls(hdlg, 0);
	}
	rd_ignore_change = 0;
	break;

    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	case IDC_COMBO_MO_BUS:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_MO_BUS);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    if (temp_mo_drives[modlv_current_sel].bus_type == b)
		goto mo_bus_skip;
	    mo_untrack(modlv_current_sel);
	    assign = (temp_mo_drives[modlv_current_sel].bus_type == b) ? 0 : 1;
	    temp_mo_drives[modlv_current_sel].bus_type = b;
	    mo_recalc_location_controls(hdlg, assign);
	    mo_track(modlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	    mo_update_item(h, modlv_current_sel);
	mo_bus_skip:
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_MO_ID:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_MO_ID);
	    zip_untrack(zdlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_mo_drives[modlv_current_sel].bus_id.scsi.id = (uint8_t)b;
	    mo_track(modlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	    mo_update_item(h, modlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_MO_LUN:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_MO_LUN);
	    mo_untrack(modlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_mo_drives[modlv_current_sel].bus_id.scsi.lun = (uint8_t)b;
	    mo_track(modlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	    mo_update_item(h, modlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;

	case IDC_COMBO_ZIP_CHANNEL_IDE:
	    if (rd_ignore_change)
		return FALSE;

	    rd_ignore_change = 1;
	    h = GetDlgItem(hdlg, IDC_COMBO_MO_CHANNEL_IDE);
	    mo_untrack(modlv_current_sel);
	    b = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
	    temp_mo_drives[modlv_current_sel].bus_id.ide_channel = (uint8_t)b;
	    mo_track(modlv_current_sel);
	    h = GetDlgItem(hdlg, IDC_LIST_MO_DRIVES);
	    mo_update_item(h, modlv_current_sel);
	    rd_ignore_change = 0;
	    return FALSE;
	}
	return FALSE;

    default:
	break;
    }

    return FALSE;
}
