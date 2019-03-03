/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Definitions for the PGC driver.
 *
 * Version:	@(#)vid_pgc.h	1.0.1	2019/03/01
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		John Elliott, <jce@seasip.info>
 *
 *		Copyright 2019 Fred N. van Kempen.
 *		Copyright 2019 John Elliott.
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
#ifndef VID_PGC_H
# define VID_PGC_H


#define PGC_ERROR_RANGE    0x01
#define PGC_ERROR_INTEGER  0x02
#define PGC_ERROR_MEMORY   0x03
#define PGC_ERROR_OVERFLOW 0x04
#define PGC_ERROR_DIGIT    0x05
#define PGC_ERROR_OPCODE   0x06
#define PGC_ERROR_RUNNING  0x07
#define PGC_ERROR_STACK    0x08
#define PGC_ERROR_TOOLONG  0x09
#define PGC_ERROR_AREA     0x0A
#define PGC_ERROR_MISSING  0x0B


struct pgc;

typedef struct pgc_cl {
    uint8_t	*list;
    uint32_t	listmax;
    uint32_t	wrptr;
    uint32_t	rdptr;
    uint32_t	repeat;
    struct pgc_cl *chain;
} pgc_cl_t;

typedef struct pgc_cmd {
    char	ascii[6];
    uint8_t	hex;
    void	(*handler)(struct pgc *);
    int		(*parser) (struct pgc *, pgc_cl_t *, int);
    int p;	
} pgc_cmd_t;

typedef struct pgc {
    int		type;			/* board type */

    mem_map_t	mapping;
    mem_map_t	cga_mapping;

    pgc_cl_t	*clist,
		*clcur;
    const pgc_cmd_t *commands;

    uint8_t	mapram[2048];	/* Host <--> PGC communication buffer */
    uint8_t	*cga_vram;
    uint8_t	*vram;
    char	asc_command[7];
    uint8_t	hex_command;
    uint32_t	palette[256];
    uint32_t	userpal[256];
    uint32_t	maxw, maxh;	/* Maximum framebuffer size */
    uint32_t	visw, vish;	/* Maximum screen size */
    uint32_t	screenw, screenh;
    int16_t	pan_x, pan_y;
    uint16_t	win_x1, win_x2, win_y1, win_y2;
    uint16_t	vp_x1, vp_x2, vp_y1, vp_y2;
    int16_t	fill_pattern[16];
    int16_t	line_pattern;
    uint8_t	draw_mode;
    uint8_t	fill_mode;
    uint8_t	colour;
    uint8_t	tjust_h;	/* hor alignment 1=left 2=center 3=right*/
    uint8_t	tjust_v;	/* vert alignment 1=bottom 2=center 3=top*/

    int32_t	x, y, z;	/* drawing position */

    thread_t	*pgc_thread;
    event_t	*pgc_wake_thread;
    int64_t	wake_timer;

    int		waiting_input_fifo;
    int		waiting_output_fifo;
    int		waiting_error_fifo;
    int		cga_enabled;
    int		cga_selected;
    int		ascii_mode;
    int		result_count;
 
    int		fontbase;
    int		linepos, displine;
    int		vc;
    int		cgadispon;
    int		con, coff, cursoron, cgablink;
    int		vsynctime, vadj;
    uint16_t	ma, maback;
    int		oddeven;

    int		dispontime, dispofftime;
    int64_t	vidtime;

    int		drawcursor;

    int		(*inputbyte)(struct pgc *, uint8_t *result); 
} pgc_t;


/* I/O functions and worker thread handlers. */
extern void	pgc_out(uint16_t addr, uint8_t val, void *priv);
extern uint8_t	pgc_in(uint16_t addr, void *priv);
extern void	pgc_write(uint32_t addr, uint8_t val, void *priv);
extern uint8_t	pgc_read(uint32_t addr, void *priv);
extern void	pgc_recalctimings(pgc_t *);
extern void	pgc_poll(void *priv);
extern void	pgc_reset(pgc_t *);
extern void	pgc_wake(pgc_t *);
extern void	pgc_sleep(pgc_t *);
extern void	pgc_close(void *priv);
extern void	pgc_speed_changed(void *priv);
extern void	pgc_init(pgc_t *,
			 int maxw, int maxh, int visw, int vish,
			 int (*inpbyte)(pgc_t *, uint8_t *));

/* Misc support functions. */
extern uint8_t	*pgc_vram_addr(pgc_t *, int16_t x, int16_t y);
extern void	pgc_sto_raster(pgc_t *, int16_t *x, int16_t *y);
extern void	pgc_ito_raster(pgc_t *, int32_t *x, int32_t *y);
extern void	pgc_dto_raster(pgc_t *, double *x, double *y);
//extern int	pgc_input_byte(pgc_t *, uint8_t *val);
//extern int	pgc_output_byte(pgc_t *, uint8_t val);
extern int	pgc_output_string(pgc_t *, const char *val);
//extern int	pgc_error_byte(pgc_t *, uint8_t val);
extern int	pgc_error_string(pgc_t *, const char *val);
extern int	pgc_error(pgc_t *, int err);

/* Graphics functions. */
extern void	pgc_plot(pgc_t *, uint16_t x, uint16_t y);
extern void	pgc_write_pixel(pgc_t *, uint16_t x, uint16_t y, uint8_t ink);
extern uint8_t	pgc_read_pixel(pgc_t *, uint16_t x, uint16_t y);
extern uint16_t	pgc_draw_line(pgc_t *, int32_t x1, int32_t y1,
			      int32_t x2, int32_t y2, uint16_t linemask);
extern uint16_t	pgc_draw_line_r(pgc_t *, int32_t x1, int32_t y1,
				int32_t x2, int32_t y2, uint16_t linemask);
extern void	pgc_draw_ellipse(pgc_t *, int32_t x, int32_t y);
extern void	pgc_fill_polygon(pgc_t *,
				 unsigned corners, int32_t *x, int32_t *y);
extern void	pgc_fill_line_r(pgc_t *, int32_t x0, int32_t x1, int32_t y);

/* Command and parameter handling functions. */
extern int	pgc_commandlist_append(pgc_cl_t *, uint8_t v);
extern int	pgc_parse_bytes(pgc_t *, pgc_cl_t *, int p);
extern int	pgc_parse_words(pgc_t *, pgc_cl_t *, int p);
extern int	pgc_parse_coords(pgc_t *, pgc_cl_t *, int p);
extern int	pgc_param_byte(pgc_t *, uint8_t *val);
extern int	pgc_param_word(pgc_t *, int16_t *val);
extern int	pgc_param_coord(pgc_t *, int32_t *val);
extern int	pgc_result_byte(pgc_t *, uint8_t val);
extern int	pgc_result_word(pgc_t *, int16_t val);
extern int	pgc_result_coord(pgc_t *, int32_t val);

extern void	pgc_hndl_lut8(pgc_t *);
extern void	pgc_hndl_lut8rd(pgc_t *);


#endif	/*VID_PGC_H*/
