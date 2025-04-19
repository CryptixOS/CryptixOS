/*
 * Created by v1tr10l7 on 19.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/linux/vesa.h>
#include <Prism/Core/Types.hpp>

/* Definitions of frame buffers						*/
constexpr usize FB_MAX              = 32;

/* ioctls
   0x46 is 'F'								*/
constexpr usize FBIOGET_VSCREENINFO = 0x4600;
constexpr usize FBIOPUT_VSCREENINFO = 0x4601;
constexpr usize FBIOGET_FSCREENINFO = 0x4602;
constexpr usize FBIOGETCMAP         = 0x4604;
constexpr usize FBIOPUTCMAP         = 0x4605;
constexpr usize FBIOPAN_DISPLAY     = 0x4606;
#define FBIO_CURSOR _IOWR('F', 0x08, struct fb_cursor)
/* 0x4607-0x460B are defined below */
/* constexpr usize FBIOGET_MONITORSPEC = 0x460;C */
/* constexpr usize FBIOPUT_MONITORSPEC = 0x460;D */
/* constexpr usize FBIOSWITCH_MONIBIT = 0x460;E */
constexpr usize FBIOGET_CON2FBMAP            = 0x460f;
constexpr usize FBIOPUT_CON2FBMAP            = 0x4610;
/* arg: 0 or vesa level + 1 */
constexpr usize FBIOBLANK                    = 0x4611;
// constexpr usize FBIOGET_VBLANK    = _IOR('F', 0x12, struct fb_vblank);
constexpr usize FBIO_ALLOC                   = 0x4613;
constexpr usize FBIO_FREE                    = 0x4614;
constexpr usize FBIOGET_GLYPH                = 0x4615;
constexpr usize FBIOGET_HWCINFO              = 0x4616;
constexpr usize FBIOPUT_MODEINFO             = 0x4617;
constexpr usize FBIOGET_DISPINFO             = 0x4618;
// constexpr usize                   = FBIO_WAITFORVSYNC _IOW('F', 0x20, u32);

/* Packed Pixels	*/
constexpr usize FB_TYPE_PACKED_PIXELS        = 0;
/* Non interleaved planes */
constexpr usize FB_TYPE_PLANES               = 1;
/* Interleaved planes	*/
constexpr usize FB_TYPE_INTERLEAVED_PLANES   = 2;
/* Text/attributes	*/
constexpr usize FB_TYPE_TEXT                 = 3;
/* EGA/VGA planes	*/
constexpr usize FB_TYPE_VGA_PLANES           = 4;
/* Type identified by a V4L2 FOURCC */
constexpr usize FB_TYPE_FOURCC               = 5;

/* Monochrome text */
constexpr usize FB_AUX_TEXT_MDA              = 0;
/* CGA/EGA/VGA Color text */
constexpr usize FB_AUX_TEXT_CGA              = 1;
/* S3 MMIO fasttext */
constexpr usize FB_AUX_TEXT_S3_MMIO          = 2;
/* MGA Millenium I: text, attr, 14 reserved bytes */
constexpr usize FB_AUX_TEXT_MGA_STEP16       = 3;
/* other MGAs:      text, attr,  6 reserved bytes */
constexpr usize FB_AUX_TEXT_MGA_STEP8        = 4;
/* 8-15: SVGA tileblit compatible modes */
constexpr usize FB_AUX_TEXT_SVGA_GROUP       = 8;
/* lower three bits says step */
constexpr usize FB_AUX_TEXT_SVGA_MASK        = 7;
/* SVGA text mode:  text, attr */
constexpr usize FB_AUX_TEXT_SVGA_STEP2       = 8;
/* SVGA text mode:  text, attr,  2 reserved bytes */
constexpr usize FB_AUX_TEXT_SVGA_STEP4       = 9;
/* SVGA text mode:  text, attr,  6 reserved bytes */
constexpr usize FB_AUX_TEXT_SVGA_STEP8       = 10;
/* SVGA text mode:  text, attr, 14 reserved bytes */
constexpr usize FB_AUX_TEXT_SVGA_STEP16      = 11;
/* reserved up to 15 */
constexpr usize FB_AUX_TEXT_SVGA_LAST        = 15;

/* 16 color planes (EGA/VGA) */
constexpr usize FB_AUX_VGA_PLANES_VGA4       = 0;
/* CFB4 in planes (VGA) */
constexpr usize FB_AUX_VGA_PLANES_CFB4       = 1;
/* CFB8 in planes (VGA) */
constexpr usize FB_AUX_VGA_PLANES_CFB8       = 2;

/* Monochr. 1=Black 0=White */
constexpr usize FB_VISUAL_MONO01             = 0;
/* Monochr. 1=White 0=Black */
constexpr usize FB_VISUAL_MONO10             = 1;
/* True color	*/
constexpr usize FB_VISUAL_TRUECOLOR          = 2;
/* Pseudo color (like atari) */
constexpr usize FB_VISUAL_PSEUDOCOLOR        = 3;
/* Direct color */
constexpr usize FB_VISUAL_DIRECTCOLOR        = 4;
/* Pseudo color readonly */
constexpr usize FB_VISUAL_STATIC_PSEUDOCOLOR = 5;
/* Visual identified by a V4L2 FOURCC */
constexpr usize FB_VISUAL_FOURCC             = 6;

/* no hardware accelerator	*/
constexpr usize FB_ACCEL_NONE                = 0;
/* Atari Blitter		*/
constexpr usize FB_ACCEL_ATARIBLITT          = 1;
/* Amiga Blitter                */
constexpr usize FB_ACCEL_AMIGABLITT          = 2;
/* Cybervision64 (S3 Trio64)    */
constexpr usize FB_ACCEL_S3_TRIO64           = 3;
/* RetinaZ3 (NCR 77C32BLT)      */
constexpr usize FB_ACCEL_NCR_77C32BLT        = 4;
/* Cybervision64/3D (S3 ViRGE)	*/
constexpr usize FB_ACCEL_S3_VIRGE            = 5;
/* ATI Mach 64GX family		*/
constexpr usize FB_ACCEL_ATI_MACH64GX        = 6;
/* DEC 21030 TGA		*/
constexpr usize FB_ACCEL_DEC_TGA             = 7;
/* ATI Mach 64CT family		*/
constexpr usize FB_ACCEL_ATI_MACH64CT        = 8;
/* ATI Mach 64CT family VT class */
constexpr usize FB_ACCEL_ATI_MACH64VT        = 9;
/* ATI Mach 64CT family GT class */
constexpr usize FB_ACCEL_ATI_MACH64GT        = 10;
/* Sun Creator/Creator3D	*/
constexpr usize FB_ACCEL_SUN_CREATOR         = 11;
/* Sun cg6			*/
constexpr usize FB_ACCEL_SUN_CGSIX           = 12;
/* Sun leo/zx			*/
constexpr usize FB_ACCEL_SUN_LEO             = 13;
/* IMS Twin Turbo		*/
constexpr usize FB_ACCEL_IMS_TWINTURBO       = 14;
/* 3Dlabs Permedia 2		*/
constexpr usize FB_ACCEL_3DLABS_PERMEDIA2    = 15;
/* Matrox MGA2064W (Millenium)	*/
constexpr usize FB_ACCEL_MATROX_MGA2064W     = 16;
/* Matrox MGA1064SG (Mystique)	*/
constexpr usize FB_ACCEL_MATROX_MGA1064SG    = 17;
/* Matrox MGA2164W (Millenium II) */
constexpr usize FB_ACCEL_MATROX_MGA2164W     = 18;
/* Matrox MGA2164W (Millenium II) */
constexpr usize FB_ACCEL_MATROX_MGA2164W_AGP = 19;
/* Matrox G100 (Productiva G100) */
constexpr usize FB_ACCEL_MATROX_MGAG100      = 20;
/* Matrox G200 (Myst, Mill, ...) */
constexpr usize FB_ACCEL_MATROX_MGAG200      = 21;
/* Sun cgfourteen		 */
constexpr usize FB_ACCEL_SUN_CG14            = 22;
/* Sun bwtwo			*/
constexpr usize FB_ACCEL_SUN_BWTWO           = 23;
/* Sun cgthree			*/
constexpr usize FB_ACCEL_SUN_CGTHREE         = 24;
/* Sun tcx			*/
constexpr usize FB_ACCEL_SUN_TCX             = 25;
/* Matrox G400			*/
constexpr usize FB_ACCEL_MATROX_MGAG400      = 26;
/* nVidia RIVA 128              */
constexpr usize FB_ACCEL_NV3                 = 27;
/* nVidia RIVA TNT		*/
constexpr usize FB_ACCEL_NV4                 = 28;
/* nVidia RIVA TNT2		*/
constexpr usize FB_ACCEL_NV5                 = 29;
/* C&T 6555x			*/
constexpr usize FB_ACCEL_CT_6555x            = 30;
/* 3Dfx Banshee			*/
constexpr usize FB_ACCEL_3DFX_BANSHEE        = 31;
/* ATI Rage128 family		*/
constexpr usize FB_ACCEL_ATI_RAGE128         = 32;
/* CyberPro 2000		*/
constexpr usize FB_ACCEL_IGS_CYBER2000       = 33;
/* CyberPro 2010		*/
constexpr usize FB_ACCEL_IGS_CYBER2010       = 34;
/* CyberPro 5000		*/
constexpr usize FB_ACCEL_IGS_CYBER5000       = 35;
/* SiS 300/630/540              */
constexpr usize FB_ACCEL_SIS_GLAMOUR         = 36;
/* 3Dlabs Permedia 3		*/
constexpr usize FB_ACCEL_3DLABS_PERMEDIA3    = 37;
/* ATI Radeon family		*/
constexpr usize FB_ACCEL_ATI_RADEON          = 38;
/* Intel 810/815                */
constexpr usize FB_ACCEL_I810                = 39;
/* SiS 315, 650, 740		*/
constexpr usize FB_ACCEL_SIS_GLAMOUR_2       = 40;
/* SiS 330 ("Xabre")		*/
constexpr usize FB_ACCEL_SIS_XABRE           = 41;
/* Intel 830M/845G/85x/865G     */
constexpr usize FB_ACCEL_I830                = 42;

/* nVidia Arch 10               */
constexpr usize FB_ACCEL_NV_10               = 43;
/* nVidia Arch 20               */
constexpr usize FB_ACCEL_NV_20               = 44;
/* nVidia Arch 30               */
constexpr usize FB_ACCEL_NV_30               = 45;
/* nVidia Arch 40               */
constexpr usize FB_ACCEL_NV_40               = 46;
/* XGI Volari V3XT, V5, V8      */
constexpr usize FB_ACCEL_XGI_VOLARI_V        = 47;
/* XGI Volari Z7                */
constexpr usize FB_ACCEL_XGI_VOLARI_Z        = 48;
/* TI OMAP16xx                  */
constexpr usize FB_ACCEL_OMAP1610            = 49;
/* Trident TGUI			*/
constexpr usize FB_ACCEL_TRIDENT_TGUI        = 50;
/* Trident 3DImage		*/
constexpr usize FB_ACCEL_TRIDENT_3DIMAGE     = 51;
/* Trident Blade3D		*/
constexpr usize FB_ACCEL_TRIDENT_BLADE3D     = 52;
/* Trident BladeXP		*/
constexpr usize FB_ACCEL_TRIDENT_BLADEXP     = 53;
/* Cirrus Logic 543x/544x/5480	*/
constexpr usize FB_ACCEL_CIRRUS_ALPINE       = 53;
/* NeoMagic NM2070              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2070     = 90;
/* NeoMagic NM2090              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2090     = 91;
/* NeoMagic NM2093              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2093     = 92;
/* NeoMagic NM2097              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2097     = 93;
/* NeoMagic NM2160              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2160     = 94;
/* NeoMagic NM2200              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2200     = 95;
/* NeoMagic NM2230              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2230     = 96;
/* NeoMagic NM2360              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2360     = 97;
/* NeoMagic NM2380              */
constexpr usize FB_ACCEL_NEOMAGIC_NM2380     = 98;
/* PXA3xx			*/
constexpr usize FB_ACCEL_PXA3XX              = 99;

/* S3 Savage4                   */
constexpr usize FB_ACCEL_SAVAGE4             = 0x80;
/* S3 Savage3D                  */
constexpr usize FB_ACCEL_SAVAGE3D            = 0x81;
/* S3 Savage3D-MV               */
constexpr usize FB_ACCEL_SAVAGE3D_MV         = 0x82;
/* S3 Savage2000                */
constexpr usize FB_ACCEL_SAVAGE2000          = 0x83;
/* S3 Savage/MX-MV              */
constexpr usize FB_ACCEL_SAVAGE_MX_MV        = 0x84;
/* S3 Savage/MX                 */
constexpr usize FB_ACCEL_SAVAGE_MX           = 0x85;
/* S3 Savage/IX-MV              */
constexpr usize FB_ACCEL_SAVAGE_IX_MV        = 0x86;
/* S3 Savage/IX                 */
constexpr usize FB_ACCEL_SAVAGE_IX           = 0x87;
/* S3 ProSavage PM133           */
constexpr usize FB_ACCEL_PROSAVAGE_PM        = 0x88;
/* S3 ProSavage KM133           */
constexpr usize FB_ACCEL_PROSAVAGE_KM        = 0x89;
/* S3 Twister                   */
constexpr usize FB_ACCEL_S3TWISTER_P         = 0x8a;
/* S3 TwisterK                  */
constexpr usize FB_ACCEL_S3TWISTER_K         = 0x8b;
/* S3 Supersavage               */
constexpr usize FB_ACCEL_SUPERSAVAGE         = 0x8c;
/* S3 ProSavage DDR             */
constexpr usize FB_ACCEL_PROSAVAGE_DDR       = 0x8d;
/* S3 ProSavage DDR-K           */
constexpr usize FB_ACCEL_PROSAVAGE_DDRK      = 0x8e;

/* PKUnity-v3 Unigfx		*/
constexpr usize FB_ACCEL_PUV3_UNIGFX         = 0xa0;

/* Device supports FOURCC-based formats */
constexpr usize FB_CAP_FOURCC                = 1;

struct fb_fix_screeninfo
{
    /* identification string eg "TT Builtin" */
    char          id[16];
    /* Start of frame buffer mem */
    unsigned long smem_start;
    /* (physical address) */
    /* Length of frame buffer mem */
    u32           smem_len;
    /* see FB_TYPE_*		*/
    u32           type;
    /* Interleave for interleaved Planes */
    u32           type_aux;
    /* see FB_VISUAL_*		*/
    u32           visual;
    /* zero if no hardware panning  */
    u16           xpanstep;
    /* zero if no hardware panning  */
    u16           ypanstep;
    /* zero if no hardware ywrap    */
    u16           ywrapstep;
    /* length of a line in bytes    */
    u32           line_length;
    /* Start of Memory Mapped I/O   */
    unsigned long mmio_start;
    /* (physical address) */
    /* Length of Memory Mapped I/O  */
    u32           mmio_len;
    /* Indicate to driver which	*/
    u32           accel;
    /*  specific chip/card we have	*/
    /* see FB_CAP_*			*/
    u16           capabilities;
    /* Reserved for future compatibility */
    u16           reserved[2];
};

/* Interpretation of offset for color fields: All offsets are from the right,
 * inside a "pixel" value, which is exactly 'bits_per_pixel' wide (means: you
 * can use the offset as right argument to <<). A pixel afterwards is a bit
 * stream and is written to video memory as that unmodified.
 *
 * For pseudocolor: offset and length should be the same for all color
 * components. Offset specifies the position of the least significant bit
 * of the palette index in a pixel value. Length indicates the number
 * of available palette entries (i.e. # of entries = 1 << length).
 */
struct fb_bitfield
{
    /* beginning of bitfield	*/
    u32 offset;
    /* length of bitfield		*/
    u32 length;
    /* != 0 : Most significant bit is */
    /* right */
    u32 msb_right;
};

/* Hold-And-Modify (HAM)        */
constexpr usize FB_NONSTD_HAM          = 1;
/* order of pixels in each byte is reversed   \
constexpr usize FB_NONSTD_REV_PIX_IN_B = 2;
                                  */

/* set values immediately (or vbl)*/
constexpr usize FB_ACTIVATE_NOW        = 0;
/* activate on next open	*/
constexpr usize FB_ACTIVATE_NXTOPEN    = 1;
/* don't set, round up impossible */
constexpr usize FB_ACTIVATE_TEST       = 2;
/* values			*/
constexpr usize FB_ACTIVATE_MASK       = 15;
/* activate values on next vbl  */
constexpr usize FB_ACTIVATE_VBL        = 16;
/* change colormap on vbl	*/
constexpr usize FB_CHANGE_CMAP_VBL     = 32;
/* change all VCs on this fb	*/
constexpr usize FB_ACTIVATE_ALL        = 64;
/* force apply even when no change*/
constexpr usize FB_ACTIVATE_FORCE      = 128;
/* invalidate videomode */
constexpr usize FB_ACTIVATE_INV_MODE   = 256;
/* for KDSET vt ioctl */
constexpr usize FB_ACTIVATE_KD_TEXT    = 512;

/* (OBSOLETE) see fb_info.flags and vc_mode */
constexpr usize FB_ACCELF_TEXT         = 1;

/* horizontal sync high active	*/
constexpr usize FB_SYNC_HOR_HIGH_ACT   = 1;
/* vertical sync high active	*/
constexpr usize FB_SYNC_VERT_HIGH_ACT  = 2;
/* external sync		*/
constexpr usize FB_SYNC_EXT            = 4;
/* composite sync high active   */
constexpr usize FB_SYNC_COMP_HIGH_ACT  = 8;
/* broadcast video timings      */
/* vtotal = 144d/288n/576i => PAL  */
/* vtotal = 121d/242n/484i => NTSC */
constexpr usize FB_SYNC_BROADCAST      = 16;
/* sync on green */
constexpr usize FB_SYNC_ON_GREEN       = 32;

/* non interlaced */
constexpr usize FB_VMODE_NONINTERLACED = 0;
/* interlaced	*/
constexpr usize FB_VMODE_INTERLACED    = 1;
/* double scan */
constexpr usize FB_VMODE_DOUBLE        = 2;
/* interlaced: top line first */
constexpr usize FB_VMODE_ODD_FLD_FIRST = 4;

constexpr usize FB_VMODE_MASK          = 255;

/* ywrap instead of panning     */
constexpr usize FB_VMODE_YWRAP         = 256;
/* smooth xpan possible (internally used) */
constexpr usize FB_VMODE_SMOOTH_XPAN   = 512;
/* don't update x/yoffset	*/
constexpr usize FB_VMODE_CONUPDATE     = 512;

/*
 * Display rotation support
 */

constexpr usize FB_ROTATE_UR           = 0;
constexpr usize FB_ROTATE_CW           = 1;
constexpr usize FB_ROTATE_UD           = 2;
constexpr usize FB_ROTATE_CCW          = 3;

consteval usize PICOS2KHZ(usize a) { return 1000000000ul / a; }
consteval usize KHZ2PICOS(usize a) { return 1000000000ul / a; }

struct fb_var_screeninfo
{
    /* visible resolution		*/
    u32                xres;
    u32                yres;
    /* virtual resolution		*/
    u32                xres_virtual;
    u32                yres_virtual;
    /* offset from virtual to visible */
    u32                xoffset;
    u32                yoffset;
    /* resolution			*/

    /* guess what			*/
    u32                bits_per_pixel;
    /* 0 = color, 1 = grayscale,	*/
    u32                grayscale;
    /* >1 = FOURCC			*/
    /* bitfield in fb mem if true color, */
    struct fb_bitfield red;
    /* else only length is significant */
    struct fb_bitfield green;
    struct fb_bitfield blue;
    /* transparency			*/
    struct fb_bitfield transp;

    /* != 0 Non standard pixel format */
    u32                nonstd;
    /* see FB_ACTIVATE_*		*/
    u32                activate;

    /* height of picture in mm    */
    u32                height;
    /* width of picture in mm     */
    u32                width;

    /* (OBSOLETE) see fb_info.flags */
    u32                accel_flags;

    /* Timing: All values in pixclocks, except pixclock (of course) */
    /* pixel clock in ps (pico seconds) */
    u32                pixclock;
    /* time from sync to picture	*/
    u32                left_margin;
    /* time from picture to sync	*/
    u32                right_margin;
    /* time from sync to picture	*/
    u32                upper_margin;
    u32                lower_margin;
    /* length of horizontal sync	*/
    u32                hsync_len;
    /* length of vertical sync	*/
    u32                vsync_len;
    /* see FB_SYNC_*		*/
    u32                sync;
    /* see FB_VMODE_*		*/
    u32                vmode;
    /* angle we rotate counter clockwise */
    u32                rotate;
    /* colorspace for FOURCC-based modes */
    u32                colorspace;
    /* Reserved for future compatibility */
    u32                reserved[4];
};

struct fb_cmap
{
    /* First entry	*/
    u32  start;
    /* Number of entries */
    u32  len;
    /* Red values	*/
    u16* red;
    u16* green;
    u16* blue;
    /* transparency, can be NULL */
    u16* transp;
};

struct fb_con2fbmap
{
    u32 console;
    u32 framebuffer;
};

enum
{
    /* screen: unblanked, hsync: on,  vsync: on */
    FB_BLANK_UNBLANK       = VESA_NO_BLANKING,

    /* screen: blanked,   hsync: on,  vsync: on */
    FB_BLANK_NORMAL        = VESA_NO_BLANKING + 1,

    /* screen: blanked,   hsync: on,  vsync: off */
    FB_BLANK_VSYNC_SUSPEND = VESA_VSYNC_SUSPEND + 1,

    /* screen: blanked,   hsync: off, vsync: on */
    FB_BLANK_HSYNC_SUSPEND = VESA_HSYNC_SUSPEND + 1,

    /* screen: blanked,   hsync: off, vsync: off */
    FB_BLANK_POWERDOWN     = VESA_POWERDOWN + 1
};

/* currently in a vertical blank */
constexpr usize FB_VBLANK_VBLANKING   = 0x001;
/* currently in a horizontal blank */
constexpr usize FB_VBLANK_HBLANKING   = 0x002;
/* vertical blanks can be detected */
constexpr usize FB_VBLANK_HAVE_VBLANK = 0x004;
/* horizontal blanks can be detected */
constexpr usize FB_VBLANK_HAVE_HBLANK = 0x008;
/* global retrace counter is available */
constexpr usize FB_VBLANK_HAVE_COUNT  = 0x010;
/* the vcount field is valid */
constexpr usize FB_VBLANK_HAVE_VCOUNT = 0x020;
/* the hcount field is valid */
constexpr usize FB_VBLANK_HAVE_HCOUNT = 0x040;
/* currently in a vsync */
constexpr usize FB_VBLANK_VSYNCING    = 0x080;
/* verical syncs can be detected */
constexpr usize FB_VBLANK_HAVE_VSYNC  = 0x100;

struct fb_vblank
{
    /* FB_VBLANK flags */
    u32 flags;
    /* counter of retraces since boot */
    u32 count;
    /* current scanline position */
    u32 vcount;
    /* current scandot position */
    u32 hcount;
    /* reserved for future compatibility */
    u32 reserved[4];
};

/* Internal HW accel */
constexpr usize ROP_COPY = 0;
constexpr usize ROP_XOR  = 1;

struct fb_copyarea
{
    u32 dx;
    u32 dy;
    u32 width;
    u32 height;
    u32 sx;
    u32 sy;
};

struct fb_fillrect
{
    /* screen-relative */
    u32 dx;
    u32 dy;
    u32 width;
    u32 height;
    u32 color;
    u32 rop;
};

struct fb_image
{
    /* Where to place image */
    u32         dx;
    u32         dy;
    u32         width;
    /* Size of image */
    u32         height;
    /* Only used when a mono bitmap */
    u32         fg_color;
    u32         bg_color;
    /* Depth of the image */
    u8          depth;
    /* Pointer to image data */
    const char* data;
    /* color map info */
    fb_cmap     cmap;
};

/*
 * hardware cursor control
 */
constexpr usize FB_CUR_SETIMAGE = 0x01;
constexpr usize FB_CUR_SETPOS   = 0x02;
constexpr usize FB_CUR_SETHOT   = 0x04;
constexpr usize FB_CUR_SETCMAP  = 0x08;
constexpr usize FB_CUR_SETSHAPE = 0x10;
constexpr usize FB_CUR_SETSIZE  = 0x20;
constexpr usize FB_CUR_SETALL   = 0xff;

struct fbcurpos
{
    u16 x, y;
};

struct fb_cursor
{
    /* what to set */
    u16         set;
    /* cursor on/off */
    u16         enable;
    /* bitop operation */
    u16         rop;
    /* cursor mask bits */
    const char* mask;
    /* cursor hot spot */
    fbcurpos    hot;
    /* Cursor image */
    fb_image    image;
};

/* Settings for the generic backlight code */
constexpr usize FB_BACKLIGHT_LEVELS = 128;
constexpr usize FB_BACKLIGHT_MAX    = 0xff;
