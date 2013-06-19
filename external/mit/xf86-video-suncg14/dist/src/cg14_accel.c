/* $NetBSD: cg14_accel.c,v 1.1 2013/06/19 13:26:01 macallan Exp $ */
/*
 * Copyright (c) 2013 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#include <sys/types.h>

/* all driver need this */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "cg14.h"
#include <sparc/sxreg.h>

#define SX_SINGLE
/*#define SX_DEBUG*/
/*#define SX_ADD_SOFTWARE*/

#ifdef SX_DEBUG
#define ENTER xf86Msg(X_ERROR, "%s>\n", __func__);
#define DPRINTF xf86Msg
#else
#define ENTER
#define DPRINTF while (0) xf86Msg
#endif

#define arraysize(ary)        (sizeof(ary) / sizeof(ary[0]))

/* 0xcc is SX's GXcopy equivalent */
uint32_t sx_rop[] = { 0x00, 0x88, 0x44, 0xcc, 0x22, 0xaa, 0x66, 0xee,
		      0x11, 0x99, 0x55, 0xdd, 0x33, 0xbb, 0x77, 0xff};

int src_formats[] = {PICT_a8r8g8b8, PICT_x8r8g8b8,
		     PICT_a8b8g8r8, PICT_x8b8g8r8, PICT_a8};
int tex_formats[] = {PICT_a8r8g8b8, PICT_a8b8g8r8, PICT_a8};

char c[8] = " .,:+*oX";

/* write an SX register */
static inline void
write_sx_reg(Cg14Ptr p, int reg, uint32_t val)
{
	*(volatile uint32_t *)(p->sxreg + reg) = val;
}

/* read an SX register */
static inline uint32_t
read_sx_reg(Cg14Ptr p, int reg)
{
	return *(volatile uint32_t *)(p->sxreg + reg);
}

/* write a memory referencing instruction */
static inline void
write_sx_io(Cg14Ptr p, int reg, uint32_t val)
{
	*(volatile uint32_t *)(p->sxio + reg) = val;
}

static inline void
CG14Wait(Cg14Ptr p)
{
	/* we just wait until the instruction queue is empty */
	while ((read_sx_reg(p, SX_CONTROL_STATUS) & SX_MT) != 0) {};
}

static void
CG14WaitMarker(ScreenPtr pScreen, int Marker)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);

	CG14Wait(p);
}

static Bool
CG14PrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
		int xdir, int ydir, int alu, Pixel planemask)
{
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);

	ENTER;
	DPRINTF(X_ERROR, "bits per pixel: %d\n",
	    pSrcPixmap->drawable.bitsPerPixel);

	if (planemask != p->last_mask) {
		CG14Wait(p);
		write_sx_reg(p, SX_PLANEMASK, planemask);
		p->last_mask = planemask;
	}
	alu = sx_rop[alu];
	if (alu != p->last_rop) {
		CG14Wait(p);
		write_sx_reg(p, SX_ROP_CONTROL, alu);
		p->last_rop = alu;
	}
	p->srcpitch = exaGetPixmapPitch(pSrcPixmap);
	p->srcoff = exaGetPixmapOffset(pSrcPixmap);
	p->xdir = xdir;
	p->ydir = ydir;
	return TRUE;
}

static void
CG14Copy(PixmapPtr pDstPixmap,
         int srcX, int srcY, int dstX, int dstY, int w, int h)
{
	ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	int dstpitch, dstoff, srcpitch, srcoff;
	int srcstart, dststart, xinc, srcinc, dstinc;
	int line, count, s, d, num;

	ENTER;
	dstpitch = exaGetPixmapPitch(pDstPixmap);
	dstoff = exaGetPixmapOffset(pDstPixmap);
	srcpitch = p->srcpitch;
	srcoff = p->srcoff;
	/*
	 * should clear the WO bit in SX_CONTROL_STATUS, then check if SX
	 * actually wrote anything and only sync if it did
	 */
	srcstart = (srcX << 2) + (srcpitch * srcY) + srcoff;
	dststart = (dstX << 2) + (dstpitch * dstY) + dstoff;

	/*
	 * we always copy up to 32 pixels at a time so direction doesn't
	 * matter if w<=32
	 */
	if (w > 32) {
		if (p->xdir < 0) {
			srcstart += (w - 32) << 2;
			dststart += (w - 32) << 2;
			xinc = -128;
		} else
			xinc = 128;
	} else
		xinc = 128;
	if (p->ydir < 0) {
		srcstart += (h - 1) * srcpitch;
		dststart += (h - 1) * dstpitch;
		srcinc = -srcpitch;
		dstinc = -dstpitch;
	} else {
		srcinc = srcpitch;
		dstinc = dstpitch;
	}
	if (p->last_rop == 0xcc) {
		/* plain old copy */
		if ( xinc > 0) {
			/* going left to right */
			for (line = 0; line < h; line++) {
				count = 0;
				s = srcstart;
				d = dststart;
				while ( count < w) {
					num = min(32, w - count);
					write_sx_io(p, s,
					    SX_LD(10, num - 1, s & 7));
					write_sx_io(p, d,
					    SX_STM(10, num - 1, d & 7));
					s += xinc;
					d += xinc;
					count += 32;
				}
				srcstart += srcinc;
				dststart += dstinc;
			}
		} else {
			/* going right to left */
			int i, chunks = (w >> 5);
			for (line = 0; line < h; line++) {
				s = srcstart;
				d = dststart;
				count = w;
				for (i = 0; i < chunks; i++) {
					write_sx_io(p, s,
					    SX_LD(10, 31, s & 7));
					write_sx_io(p, d,
					    SX_STM(10, 31, d & 7));
					s -= 128;
					d -= 128;
					count -= 32;
				}
				/* leftovers, if any */
				if (count > 0) {
					s += (32 - count) << 2;
					d += (32 - count) << 2;
					write_sx_io(p, s,
					    SX_LD(10, count - 1, s & 7));
					write_sx_io(p, d,
					    SX_STM(10, count - 1, d & 7));
				}
				srcstart += srcinc;
				dststart += dstinc;
			}
		}
	} else {
		/* ROPs needed */
		if ( xinc > 0) {
			/* going left to right */
			for (line = 0; line < h; line++) {
				count = 0;
				s = srcstart;
				d = dststart;
				while ( count < w) {
					num = min(32, w - count);
					write_sx_io(p, s,
					    SX_LD(10, num - 1, s & 7));
					write_sx_io(p, d,
					    SX_LD(42, num - 1, d & 7));
					if (num > 16) {
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	 SX_ROP(10, 42, 74, 15));
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	 SX_ROP(26, 58, 90, num - 17));
					} else {
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	 SX_ROP(10, 42, 74, num - 1));
					}
					write_sx_io(p, d,
					    SX_STM(74, num - 1, d & 7));
					s += xinc;
					d += xinc;
					count += 32;
				}
				srcstart += srcinc;
				dststart += dstinc;
			}
		} else {
			/* going right to left */
			int i, chunks = (w >> 5);
			for (line = 0; line < h; line++) {
				s = srcstart;
				d = dststart;
				count = w;
				for (i = 0; i < chunks; i++) {
					write_sx_io(p, s, SX_LD(10, 31, s & 7));
					write_sx_io(p, d, SX_LD(42, 31, d & 7));
					write_sx_reg(p, SX_INSTRUCTIONS,
				    	    SX_ROP(10, 42, 74, 15));
					write_sx_reg(p, SX_INSTRUCTIONS,
				    	    SX_ROP(26, 58, 90, 15));
					write_sx_io(p, d,
					    SX_STM(74, 31, d & 7));
					s -= 128;
					d -= 128;
					count -= 32;
				}
				/* leftovers, if any */
				if (count > 0) {
					s += (32 - count) << 2;
					d += (32 - count) << 2;
					write_sx_io(p, s,
					    SX_LD(10, count - 1, s & 7));
					write_sx_io(p, d,
					    SX_LD(42, count - 1, d & 7));
					if (count > 16) {
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	    SX_ROP(10, 42, 74, 15));
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	 SX_ROP(26, 58, 90, count - 17));
					} else {
						write_sx_reg(p, SX_INSTRUCTIONS,
					    	 SX_ROP(10, 42, 74, count - 1));
					}
					
					write_sx_io(p, d,
					    SX_STM(74, count - 1, d & 7));
				}
				srcstart += srcinc;
				dststart += dstinc;
			}
		}
	}			
	exaMarkSync(pDstPixmap->drawable.pScreen);
}

static void
CG14DoneCopy(PixmapPtr pDstPixmap)
{
}

static Bool
CG14PrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);

	ENTER;
	DPRINTF(X_ERROR, "bits per pixel: %d\n", pPixmap->drawable.bitsPerPixel);
	write_sx_reg(p, SX_QUEUED(8), fg);
	write_sx_reg(p, SX_QUEUED(9), fg);
	if (planemask != p->last_mask) {
		CG14Wait(p);
		write_sx_reg(p, SX_PLANEMASK, planemask);
		p->last_mask = planemask;
	}
	alu = sx_rop[alu];
	if (alu != p->last_rop) {
		CG14Wait(p);
		write_sx_reg(p, SX_ROP_CONTROL, alu);
		p->last_rop = alu;
	}
	DPRINTF(X_ERROR, "%s: %x\n", __func__, alu);
	return TRUE;
}

static void
CG14Solid32(Cg14Ptr p, uint32_t start, uint32_t pitch, int w, int h)
{
	int line, x, num;
	uint32_t ptr;

	ENTER;
	if (p->last_rop == 0xcc) {
		/* simple fill */
		for (line = 0; line < h; line++) {
			x = 0;
			while (x < w) {
				ptr = start + (x << 2);
				num = min(32, w - x);
				write_sx_io(p, ptr,
				    SX_STS(8, num - 1, ptr & 7));
				x += 32;
			}
			start += pitch;
		}
	} else if (p->last_rop == 0xaa) {
		/* nothing to do here */
		return;
	} else {
		/* alright, let's do actual ROP stuff */

		/* first repeat the fill colour into 16 registers */
		write_sx_reg(p, SX_INSTRUCTIONS,
		    SX_SELECT_S(8, 8, 10, 15));

		for (line = 0; line < h; line++) {
			x = 0;
			while (x < w) {
				ptr = start + (x << 2);
				num = min(32, w - x);
				/* now suck fb data into registers */
				write_sx_io(p, ptr,
				    SX_LD(42, num - 1, ptr & 7));
				/*
				 * ROP them with the fill data we left in 10
				 * non-memory ops can only have counts up to 16
				 */
				if (num <= 16) {
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 42, 74, num - 1));
				} else {
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 42, 74, 15));
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 58, 90, num - 17));
				}
				/* and write the result back into memory */
				write_sx_io(p, ptr,
				    SX_ST(74, num - 1, ptr & 7));
				x += 32;
			}
			start += pitch;
		}
	}
}

static void
CG14Solid8(Cg14Ptr p, uint32_t start, uint32_t pitch, int w, int h)
{
	int line, x, num, off;
	uint32_t ptr;

	ENTER;
	off = start & 7;
	start &= ~7;

	if (p->last_rop == 0xcc) {
		/* simple fill */
		for (line = 0; line < h; line++) {
			x = 0;
			while (x < w) {
				ptr = start + x;
				num = min(32, w - x);
				write_sx_io(p, ptr,
				    SX_STBS(8, num - 1, off));
				x += 32;
			}
			start += pitch;
		}
	} else if (p->last_rop == 0xaa) {
		/* nothing to do here */
		return;
	} else {
		/* alright, let's do actual ROP stuff */

		/* first repeat the fill colour into 16 registers */
		write_sx_reg(p, SX_INSTRUCTIONS,
		    SX_SELECT_S(8, 8, 10, 15));

		for (line = 0; line < h; line++) {
			x = 0;
			while (x < w) {
				ptr = start + x;
				num = min(32, w - x);
				/* now suck fb data into registers */
				write_sx_io(p, ptr,
				    SX_LDB(42, num - 1, off));
				/*
				 * ROP them with the fill data we left in 10
				 * non-memory ops can only have counts up to 16
				 */
				if (num <= 16) {
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 42, 74, num - 1));
				} else {
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 42, 74, 15));
					write_sx_reg(p, SX_INSTRUCTIONS,
					    SX_ROP(10, 58, 90, num - 17));
				}
				/* and write the result back into memory */
				write_sx_io(p, ptr,
				    SX_STB(74, num - 1, off));
				x += 32;
			}
			start += pitch;
		}
	}
}

static void
CG14Solid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2)
{
	ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	int w = x2 - x1, h = y2 - y1, dstoff, dstpitch;
	int start, depth;

	ENTER;
	dstpitch = exaGetPixmapPitch(pPixmap);
	dstoff = exaGetPixmapOffset(pPixmap);

	depth = pPixmap->drawable.bitsPerPixel;
	switch (depth) {
		case 32:
			start = dstoff + (y1 * dstpitch) + (x1 << 2);
			CG14Solid32(p, start, dstpitch, w, h);
			break;
		case 8:
			start = dstoff + (y1 * dstpitch) + x1;
			CG14Solid8(p, start, dstpitch, w, h);
			break;
	}

	DPRINTF(X_ERROR, "Solid %d %d %d %d, %d %d -> %d\n", x1, y1, x2, y2,
	    dstpitch, dstoff, start);
	DPRINTF(X_ERROR, "%x %x %x\n", p->last_rop,
	    read_sx_reg(p, SX_QUEUED(8)), read_sx_reg(p, SX_QUEUED(9)));
	exaMarkSync(pPixmap->drawable.pScreen);
}

/*
 * Memcpy-based UTS.
 */
static Bool
CG14UploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
    char *src, int src_pitch)
{
	ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	char  *dst        = p->fb + exaGetPixmapOffset(pDst);
	int    dst_pitch  = exaGetPixmapPitch(pDst);

	int bpp    = pDst->drawable.bitsPerPixel;
	int cpp    = (bpp + 7) >> 3;
	int wBytes = w * cpp;

	ENTER;
	dst += (x * cpp) + (y * dst_pitch);

	CG14Wait(p);

	while (h--) {
		memcpy(dst, src, wBytes);
		src += src_pitch;
		dst += dst_pitch;
	}
	__asm("stbar;");
	return TRUE;
}

/*
 * Memcpy-based DFS.
 */
static Bool
CG14DownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
    char *dst, int dst_pitch)
{
	ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	char  *src        = p->fb + exaGetPixmapOffset(pSrc);
	int    src_pitch  = exaGetPixmapPitch(pSrc);

	ENTER;
	int bpp    = pSrc->drawable.bitsPerPixel;
	int cpp    = (bpp + 7) >> 3;
	int wBytes = w * cpp;

	src += (x * cpp) + (y * src_pitch);

	CG14Wait(p);

	while (h--) {
		memcpy(dst, src, wBytes);
		src += src_pitch;
		dst += dst_pitch;
	}

	return TRUE;
}

Bool
CG14CheckComposite(int op, PicturePtr pSrcPicture,
                           PicturePtr pMaskPicture,
                           PicturePtr pDstPicture)
{
	int i, ok = FALSE;

	ENTER;

	/*
	 * SX is in theory capable of accelerating pretty much all Xrender ops,
	 * even coordinate transformation and gradients. Support will be added
	 * over time and likely have to spill over into its own source file.
	 */
	
	if ((op != PictOpOver) && (op != PictOpAdd)) {
		xf86Msg(X_ERROR, "%s: rejecting %d\n", __func__, op);
		return FALSE;
	}
	i = 0;
	while ((i < arraysize(src_formats)) && (!ok)) {
		ok =  (pSrcPicture->format == src_formats[i]);
		i++;
	}

	if (!ok) {
		xf86Msg(X_ERROR, "%s: unsupported src format %x\n",
		    __func__, pSrcPicture->format);
		return FALSE;
	}

	DPRINTF(X_ERROR, "src is %x %d %d\n", pSrcPicture->format,
	    pSrcPicture->pDrawable->width, pSrcPicture->pDrawable->height);

	if (pMaskPicture != NULL) {
		DPRINTF(X_ERROR, "mask is %x %d %d\n", pMaskPicture->format,
		    pMaskPicture->pDrawable->width,
		    pMaskPicture->pDrawable->height);
	}
	return TRUE;
}

Bool
CG14PrepareComposite(int op, PicturePtr pSrcPicture,
                             PicturePtr pMaskPicture,
                             PicturePtr pDstPicture,
                             PixmapPtr  pSrc,
                             PixmapPtr  pMask,
                             PixmapPtr  pDst)
{
	ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);

	ENTER;

	if (pSrcPicture->pSourcePict != NULL) {
		if (pSrcPicture->pSourcePict->type == SourcePictTypeSolidFill) {
			p->fillcolour =
			    pSrcPicture->pSourcePict->solidFill.color;
			DPRINTF(X_ERROR, "%s: solid src %08x\n",
			    __func__, p->fillcolour);
		}
	}
	if ((pMaskPicture != NULL) && (pMaskPicture->pSourcePict != NULL)) {
		if (pMaskPicture->pSourcePict->type ==
		    SourcePictTypeSolidFill) {
			p->fillcolour = 
			   pMaskPicture->pSourcePict->solidFill.color;
			DPRINTF(X_ERROR, "%s: solid mask %08x\n",
			    __func__, p->fillcolour);
		}
	}
	if (pMaskPicture != NULL) {
		p->mskoff = exaGetPixmapOffset(pMask);		
		p->mskpitch = exaGetPixmapPitch(pMask);
		p->mskformat = pMaskPicture->format;
	}
	p->srcoff = exaGetPixmapOffset(pSrc);		
	p->srcpitch = exaGetPixmapPitch(pSrc);		
	p->srcformat = pSrcPicture->format;
	p->dstformat = pDstPicture->format;
	p->op = op;
#ifdef SX_DEBUG
	DPRINTF(X_ERROR, "%x %x -> %x\n", p->srcoff, p->mskoff,
	    *(uint32_t *)(p->fb + p->srcoff));	
#endif
	return TRUE;
}

void CG14Comp_Over32(Cg14Ptr p,
                   uint32_t src, uint32_t srcpitch,
                   uint32_t dst, uint32_t dstpitch,
                   int width, int height)
{
	uint32_t msk = src, mskx, dstx, m;
	int line, x, i;

	ENTER;
	/* first get the source colour */
	write_sx_io(p, p->srcoff, SX_LDUQ0(8, 0, p->srcoff & 7));
	write_sx_reg(p, SX_QUEUED(8), 0xff);
	for (line = 0; line < height; line++) {
		mskx = msk;
		dstx = dst;
#ifdef SX_SINGLE

		for (x = 0; x < width; x++) {
			m = *(volatile uint32_t *)(p->fb + mskx);
			m = m >> 24;
			if (m == 0) {
				/* nothing to do - all transparent */
			} else if (m == 0xff) {
				/* all opaque */
				write_sx_io(p, dstx, SX_STUQ0(8, 0, dstx & 7));
			} else {
				/* fetch alpha value, stick it into scam */
				/* mask is in R[12:15] */
				/*write_sx_io(p, mskx,
				    SX_LDUQ0(12, 0, mskx & 7));*/
				write_sx_reg(p, SX_QUEUED(12), m);
				/* fetch dst pixel */
				write_sx_io(p, dstx, SX_LDUQ0(20, 0, dstx & 7));
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_ORV(12, 0, R_SCAM, 0));
				/*
				 * src * alpha + R0
				 * R[9:11] * SCAM + R0 -> R[17:19]
				 */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_SAXP16X16SR8(9, 0, 17, 2));
			
				/* invert SCAM */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_XORV(12, 8, R_SCAM, 0));
#ifdef SX_DEBUG
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_XORV(12, 8, 13, 0));
#endif
				/* dst * (1 - alpha) + R[13:15] */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_SAXP16X16SR8(21, 17, 25, 2));
				write_sx_io(p, dstx,
				    SX_STUQ0C(24, 0, dstx & 7));
			}
			dstx += 4;
			mskx += 4;
		}
#else
		for (x = 0; x < width; x += 4) {
			/* fetch 4 mask values */
			write_sx_io(p, mskx, SX_LDUQ0(12, 3, mskx & 7));
			/* fetch destination pixels */
			write_sx_io(p, dstx, SX_LDUQ0(60, 3, dstx & 7));
			/* duplicate them for all channels */
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 12, 13, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 16, 17, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 20, 21, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 24, 25, 2));
			/* generate inverted alpha */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_XORS(12, 8, 28, 15));
			/* multiply source */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 12, 44, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 16, 48, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 20, 52, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 24, 56, 3));
			/* multiply dest */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(28, 60, 76, 15));
			/* add up */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(44, 76, 92, 15));
			/* write back */
			write_sx_io(p, dstx, SX_STUQ0C(92, 3, dstx & 7));
			dstx += 16;
			mskx += 16;
		}
#endif
		dst += dstpitch;
		msk += srcpitch;
	}
}

void CG14Comp_Over8(Cg14Ptr p,
                   uint32_t src, uint32_t srcpitch,
                   uint32_t dst, uint32_t dstpitch,
                   int width, int height)
{
	uint32_t msk = src, mskx, dstx, m;
	int line, x, i;
#ifdef SX_DEBUG
	char buffer[256];
#endif
	ENTER;

	/* first get the source colour */
	write_sx_io(p, p->srcoff, SX_LDUQ0(8, 0, p->srcoff & 7));
	write_sx_reg(p, SX_QUEUED(8), 0xff);
	DPRINTF(X_ERROR, "src: %d %d %d, %08x\n", read_sx_reg(p, SX_QUEUED(9)),
	    read_sx_reg(p, SX_QUEUED(10)), read_sx_reg(p, SX_QUEUED(11)),
	    *(uint32_t *)(p->fb + p->srcoff));
	for (line = 0; line < height; line++) {
		mskx = msk;
		dstx = dst;
#ifdef SX_SINGLE

		for (x = 0; x < width; x++) {
			m = *(volatile uint8_t *)(p->fb + mskx);
#ifdef SX_DEBUG
			buffer[x] = c[m >> 5];
#endif
			if (m == 0) {
				/* nothing to do - all transparent */
			} else if (m == 0xff) {
				/* all opaque */
				write_sx_io(p, dstx, SX_STUQ0(8, 0, dstx & 7));
			} else {
				/* fetch alpha value, stick it into scam */
				/* mask is in R[12:15] */
				/*write_sx_io(p, mskx & ~7, 
				    SX_LDB(12, 0, mskx & 7));*/
				write_sx_reg(p, SX_QUEUED(12), m);
				/* fetch dst pixel */
				write_sx_io(p, dstx, SX_LDUQ0(20, 0, dstx & 7));
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_ORV(12, 0, R_SCAM, 0));
				/*
				 * src * alpha + R0
				 * R[9:11] * SCAM + R0 -> R[17:19]
				 */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_SAXP16X16SR8(9, 0, 17, 2));
			
				/* invert SCAM */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_XORV(12, 8, R_SCAM, 0));
#ifdef SX_DEBUG
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_XORV(12, 8, 13, 0));
#endif
				/* dst * (1 - alpha) + R[13:15] */
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_SAXP16X16SR8(21, 17, 25, 2));
				write_sx_io(p, dstx,
				    SX_STUQ0C(24, 0, dstx & 7));
			}
			dstx += 4;
			mskx += 1;
		}
#ifdef SX_DEBUG
		buffer[x] = 0;
		xf86Msg(X_ERROR, "%s\n", buffer);
#endif
#else
		for (x = 0; x < width; x += 4) {
			/* fetch 4 mask values */
			write_sx_io(p, mskx, SX_LDB(12, 3, mskx & 7));
			/* fetch destination pixels */
			write_sx_io(p, dstx, SX_LDUQ0(60, 3, dstx & 7));
			/* duplicate them for all channels */
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 12, 13, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 16, 17, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 20, 21, 2));			
			write_sx_reg(p, SX_INSTRUCTIONS, SX_ORS(0, 24, 25, 2));
			/* generate inverted alpha */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_XORS(12, 8, 28, 15));
			/* multiply source */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 12, 44, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 16, 48, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 20, 52, 3));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(8, 24, 56, 3));
			/* multiply dest */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_MUL16X16SR8(28, 60, 76, 15));
			/* add up */
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(44, 76, 92, 15));
			/* write back */
			write_sx_io(p, dstx, SX_STUQ0C(92, 3, dstx & 7));
			dstx += 16;
			mskx += 4;
		}
#endif
		dst += dstpitch;
		msk += srcpitch;
	}
}

void CG14Comp_Add32(Cg14Ptr p,
                   uint32_t src, uint32_t srcpitch,
                   uint32_t dst, uint32_t dstpitch,
                   int width, int height)
{
	int line;
	uint32_t srcx, dstx;
	int full, part, x;

	ENTER;
	full = width >> 3;	/* chunks of 8 */
	part = width & 7;	/* leftovers */
	/* we do this up to 8 pixels at a time */
	for (line = 0; line < height; line++) {
		srcx = src;
		dstx = dst;
		for (x = 0; x < full; x++) {
			write_sx_io(p, srcx, SX_LDUQ0(8, 31, srcx & 7));
			write_sx_io(p, dstx, SX_LDUQ0(40, 31, dstx & 7));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(8, 40, 72, 15));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(24, 56, 88, 15));
			write_sx_io(p, dstx, SX_STUQ0(72, 31, dstx & 7));
			srcx += 128;
			dstx += 128;
		}

		/* do leftovers */
		write_sx_io(p, srcx, SX_LDUQ0(8, part - 1, srcx & 7));
		write_sx_io(p, dstx, SX_LDUQ0(40, part - 1, dstx & 7));
		if (part & 16) {
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(8, 40, 72, 15));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(24, 56, 88, part - 17));
		} else {
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(8, 40, 72, part - 1));
		}
		write_sx_io(p, dstx, SX_STUQ0(72, part - 1, dstx & 7));
		
		/* next line */
		src += srcpitch;
		dst += dstpitch;
	}
}

void CG14Comp_Add8(Cg14Ptr p,
                   uint32_t src, uint32_t srcpitch,
                   uint32_t dst, uint32_t dstpitch,
                   int width, int height)
{
	int line;
	uint32_t srcx, dstx, srcoff, dstoff;
	int pre, full, part, x;
	uint8_t *d;
	char buffer[256];
	ENTER;

	srcoff = src & 7;
	src &= ~7;
	dstoff = dst & 7;
	dst &= ~7;
	full = width >> 5;	/* chunks of 32 */
	part = width & 31;	/* leftovers */

#ifdef SX_DEBUG
	xf86Msg(X_ERROR, "%d %d, %d x %d, %d %d\n", srcpitch, dstpitch,
	    width, height, full, part);
#endif
	/* we do this up to 32 pixels at a time */
	for (line = 0; line < height; line++) {
		srcx = src;
		dstx = dst;
#ifdef SX_ADD_SOFTWARE
		uint8_t *s = (uint8_t *)(p->fb + srcx + srcoff); 
		d = (uint8_t *)(p->fb + dstx + dstoff); 
		for (x = 0; x < width; x++) {
			d[x] = min(255, s[x] + d[x]);
		}
#else
		for (x = 0; x < full; x++) {
			write_sx_io(p, srcx, SX_LDB(8, 31, srcoff));
			write_sx_io(p, dstx, SX_LDB(40, 31, dstoff));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(8, 40, 72, 15));
			write_sx_reg(p, SX_INSTRUCTIONS,
			    SX_ADDV(24, 56, 88, 15));
			write_sx_io(p, dstx, SX_STBC(72, 31, dstoff));
			srcx += 32;
			dstx += 32;
		}

		if (part > 0) {
			/* do leftovers */
			write_sx_io(p, srcx, SX_LDB(8, part - 1, srcoff));
			write_sx_io(p, dstx, SX_LDB(40, part - 1, dstoff));
			if (part > 16) {
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_ADDV(8, 40, 72, 15));
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_ADDV(24, 56, 88, part - 17));
			} else {
				write_sx_reg(p, SX_INSTRUCTIONS,
				    SX_ADDV(8, 40, 72, part - 1));
			}
			write_sx_io(p, dstx, SX_STBC(72, part - 1, dstoff));
		}
#endif
#ifdef SX_DEBUG
		d = (uint8_t *)(p->fb + src + srcoff);
		for (x = 0; x < width; x++) {
			buffer[x] = c[d[x]>>5];
		}
		buffer[x] = 0;
		xf86Msg(X_ERROR, "%s\n", buffer);
#endif
		/* next line */
		src += srcpitch;
		dst += dstpitch;
	}
}

void
CG14Composite(PixmapPtr pDst, int srcX, int srcY,
                              int maskX, int maskY,
                              int dstX, int dstY,
                              int width, int height)
{
	ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	uint32_t dstoff, dstpitch;
	uint32_t dst, msk, src;

	ENTER;
	dstoff = exaGetPixmapOffset(pDst);		
	dstpitch = exaGetPixmapPitch(pDst);

	switch (p->op) {
		case PictOpOver:
			dst = dstoff + (dstY * dstpitch) + (dstX << 2);
			DPRINTF(X_ERROR, "Over %08x %08x, %d %d\n",
			    p->mskformat, p->dstformat, srcX, srcY);
			switch (p->mskformat) {
				case PICT_a8:
					msk = p->mskoff + 
					    (maskY * p->mskpitch) + maskX;
					CG14Comp_Over8(p, msk, p->mskpitch,
					    dst, dstpitch, width, height);
					break;
				case PICT_a8r8g8b8:
				case PICT_a8b8g8r8:
					msk = p->mskoff + 
					    (maskY * p->mskpitch) + 
					    (maskX << 2);
					CG14Comp_Over32(p, msk, p->mskpitch,
					    dst, dstpitch, width, height);
					break;
				default:
					xf86Msg(X_ERROR,
					    "unsupported mask format\n");
			}				
			break;
		case PictOpAdd:
			DPRINTF(X_ERROR, "Add %08x %08x\n",
			    p->srcformat, p->dstformat);
			switch (p->srcformat) {
				case PICT_a8:
					src = p->srcoff +
					    (srcY * p->srcpitch) + srcX;
					dst = dstoff + (dstY * dstpitch) + dstX;
					CG14Comp_Add8(p, src, p->srcpitch,
					    dst, dstpitch, width, height);
					break;
				case PICT_a8r8g8b8:
				case PICT_x8r8g8b8:
					src = p->srcoff +
					    (srcY * p->srcpitch) + (srcX << 2);
					dst = dstoff + (dstY * dstpitch) +
					    (dstX << 2);
					CG14Comp_Add32(p, src, p->srcpitch,
					    dst, dstpitch, width, height);
					break;
				default:
					xf86Msg(X_ERROR,
					    "unsupported src format\n");
			}
			break;
		default:
			xf86Msg(X_ERROR, "unsupported op %d\n", p->op);
	}
	exaMarkSync(pDst->drawable.pScreen);
}



Bool
CG14InitAccel(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	Cg14Ptr p = GET_CG14_FROM_SCRN(pScrn);
	ExaDriverPtr pExa;

	pExa = exaDriverAlloc();
	if (!pExa)
		return FALSE;

	p->pExa = pExa;

	pExa->exa_major = EXA_VERSION_MAJOR;
	pExa->exa_minor = EXA_VERSION_MINOR;

	pExa->memoryBase = p->fb;
	pExa->memorySize = p->memsize;
	pExa->offScreenBase = p->width * p->height * 4;

	/*
	 * SX memory instructions are written to 64bit aligned addresses with
	 * a 3 bit displacement. Make sure the displacement remains constant
	 * within one column
	 */
	
	pExa->pixmapOffsetAlign = 8;
	pExa->pixmapPitchAlign = 8;

	pExa->flags = EXA_OFFSCREEN_PIXMAPS |
		      /*EXA_SUPPORTS_OFFSCREEN_OVERLAPS |*/
		      EXA_MIXED_PIXMAPS;

	/*
	 * these limits are bogus
	 * SX doesn't deal with coordinates at all, so there is no limit but
	 * we have to put something here
	 */
	pExa->maxX = 4096;
	pExa->maxY = 4096;

	pExa->WaitMarker = CG14WaitMarker;

	pExa->PrepareSolid = CG14PrepareSolid;
	pExa->Solid = CG14Solid;
	pExa->DoneSolid = CG14DoneCopy;
	pExa->PrepareCopy = CG14PrepareCopy;
	pExa->Copy = CG14Copy;
	pExa->DoneCopy = CG14DoneCopy;
	if (p->use_xrender) {
		pExa->CheckComposite = CG14CheckComposite;
		pExa->PrepareComposite = CG14PrepareComposite;
		pExa->Composite = CG14Composite;
		pExa->DoneComposite = CG14DoneCopy;
	}

	/* EXA hits more optimized paths when it does not have to fallback 
	 * because of missing UTS/DFS, hook memcpy-based UTS/DFS.
	 */
	pExa->UploadToScreen = CG14UploadToScreen;
	pExa->DownloadFromScreen = CG14DownloadFromScreen;

	/* do some hardware init */
	write_sx_reg(p, SX_PLANEMASK, 0xffffffff);
	p->last_mask = 0xffffffff;
	write_sx_reg(p, SX_ROP_CONTROL, 0xcc);
	p->last_rop = 0xcc;
	return exaDriverInit(pScreen, pExa);
}
