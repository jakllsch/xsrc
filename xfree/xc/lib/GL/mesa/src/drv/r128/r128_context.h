/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_context.h,v 1.9 2001/10/02 11:44:13 alanh Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __R128_CONTEXT_H__
#define __R128_CONTEXT_H__

#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>

#include "dri_mesaint.h"
#include "dri_tmm.h"

#include "xf86drm.h"
#include "xf86drmR128.h"

#include "types.h"

#include "r128_sarea.h"
#include "r128_reg.h"

struct r128_context;
typedef struct r128_context r128ContextRec;
typedef struct r128_context *r128ContextPtr;

#include "r128_lock.h"
#include "r128_texobj.h"
#include "r128_screen.h"

/* Flags for what context state needs to be updated:
 */
#define R128_NEW_ALPHA		0x0001
#define R128_NEW_DEPTH		0x0002
#define R128_NEW_FOG		0x0004
#define R128_NEW_CLIP		0x0008
#define R128_NEW_CULL		0x0010
#define R128_NEW_MASKS		0x0020
#define R128_NEW_RENDER_NOT	0x0040
#define R128_NEW_WINDOW		0x0080
#define R128_NEW_TEXTURE	0x0100
#define R128_NEW_CONTEXT	0x0200
#define R128_NEW_ALL		0x03ff

/* Flags for software fallback cases:
 */
#define R128_FALLBACK_TEXTURE		0x0001
#define R128_FALLBACK_DRAW_BUFFER	0x0002
#define R128_FALLBACK_READ_BUFFER	0x0004
#define R128_FALLBACK_STENCIL		0x0008
#define R128_FALLBACK_RENDER_MODE	0x0010
#define R128_FALLBACK_MULTIDRAW		0x0020
#define R128_FALLBACK_LOGICOP		0x0040

/* Reasons why the GL_BLEND fallback mightn't work:
 */
#define R128_BLEND_ENV_COLOR	0x1
#define R128_BLEND_MULTITEX	0x2

/* Subpixel offsets for window coordinates (triangles):
 */
#define SUBPIXEL_X  (0.0F)
#define SUBPIXEL_Y  (0.125F)

/* Offset for points:
 */
#define PNT_X_OFFSET  ( 0.125F)
#define PNT_Y_OFFSET  (-0.125F)


typedef void (*r128_interp_func)( GLfloat t,
				  GLfloat *result,
				  const GLfloat *in,
				  const GLfloat *out );

struct r128_elt_tab {
   void (*emit_unclipped_verts)( struct vertex_buffer *VB );

   void (*build_tri_verts)( r128ContextPtr rmesa,
			    struct vertex_buffer *VB,
			    GLfloat *O, GLuint *elt );

   void (*interp)( GLfloat t, GLfloat *O,
		   const GLfloat *I, const GLfloat *J );

   void (*project_and_emit_verts)( r128ContextPtr rmesa,
				   const GLfloat *verts,
				   GLuint *elts,
				   GLuint nr );
};

struct r128_context {
   GLcontext *glCtx;			/* Mesa context */

   /* Driver and hardware state management
    */
   GLuint new_state;
   GLuint dirty;			/* Hardware state to be updated */
   r128_context_regs_t setup;

   GLuint vertsize;
   GLuint vc_format;
   GLfloat depth_scale;

   GLuint Color;			/* Current draw color */
   GLuint ClearColor;			/* Color used to clear color buffer */
   GLuint ClearDepth;			/* Value used to clear depth buffer */
   GLuint ClearStencil;			/* Value used to clear stencil */
   GLuint DepthMask;
   GLuint StencilMask;

   /* Map GL texture units onto hardware
    */
   GLint multitex;
   GLint tmu_source[2];
   GLint tex_dest[2];
   GLuint tex_combine[2];
   GLuint blend_flags;
   GLuint env_color;

   /* Texture object bookkeeping
    */
   r128TexObjPtr CurrentTexObj[2];
   r128TexObj TexObjList[R128_NR_TEX_HEAPS];
   r128TexObj SwappedOut;
   memHeap_t *texHeap[R128_NR_TEX_HEAPS];
   GLint lastTexAge[R128_NR_TEX_HEAPS];
   GLint lastTexHeap;

   /* Current rendering state, fallbacks
    */
   points_func   PointsFunc;
   line_func     LineFunc;
   triangle_func TriangleFunc;
   quad_func     QuadFunc;

   GLuint IndirectTriangles;
   GLuint Fallback;

   /* Fast path
    */
   GLuint SetupIndex;
   GLuint SetupDone;
   GLuint RenderIndex;
   GLuint OnFastPath;
   r128_interp_func interp;
   GLfloat *tmp_matrix;

   /* Vertex buffers
    */
   drmBufPtr vert_buf;
   GLuint num_verts;

   /* Elt path
    */
   drmBufPtr elt_buf, retained_buf;
   GLushort *first_elt, *next_elt;
   GLfloat *next_vert, *vert_heap;
   GLushort next_vert_index;
   GLushort first_vert_index;
   GLuint elt_vertsize;
   struct r128_elt_tab *elt_tab;
   GLfloat device_matrix[16];

   /* Page flipping
    */
   GLuint doPageFlip;
   GLuint currentPage;

   /* Drawable, cliprect and scissor information
    */
   GLenum DrawBuffer;			/* Optimize draw buffer update */
   GLint drawOffset, drawPitch;
   GLint readOffset, readPitch;

   GLuint numClipRects;			/* Cliprects for the draw buffer */
   XF86DRIClipRectPtr pClipRects;

   GLuint scissor;
   XF86DRIClipRectRec ScissorRect;	/* Current software scissor */

   /* Mirrors of some DRI state
    */
   Display *display;			/* X server display */

   __DRIcontextPrivate	*driContext;	/* DRI context */
   __DRIscreenPrivate	*driScreen;	/* DRI screen */
   __DRIdrawablePrivate	*driDrawable;	/* DRI drawable bound to this ctx */

   int lastStamp;		        /* mirror driDrawable->lastStamp */

   drmContext hHWContext;
   drmLock *driHwLock;
   int driFd;

   r128ScreenPtr r128Screen;		/* Screen private DRI data */
   R128SAREAPrivPtr sarea;		/* Private SAREA data */

   /* Performance counters
    */
   GLuint boxes;			/* Draw performance boxes */
   GLuint hardwareWentIdle;
   GLuint c_clears;
   GLuint c_drawWaits;
   GLuint c_textureSwaps;
   GLuint c_textureBytes;
   GLuint c_vertexBuffers;
};

#define R128_CONTEXT(ctx)		((r128ContextPtr)(ctx->DriverCtx))

#define R128_IS_PLAIN( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128)
#define R128_IS_PRO( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128_PRO)
#define R128_IS_MOBILITY( rmesa ) \
		(rmesa->r128Screen->chipset == R128_CARD_TYPE_R128_MOBILITY)


extern GLboolean r128CreateContext( Display *dpy, GLvisual *glVisual,
				    __DRIcontextPrivate *driContextPriv );
extern void r128DestroyContext( r128ContextPtr rmesa );
extern r128ContextPtr r128MakeCurrent( r128ContextPtr oldCtx,
				       r128ContextPtr newCtx,
				       __DRIdrawablePrivate *dPriv );

/* ================================================================
 * Byte ordering
 */
#include "X11/Xarch.h"

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define LE32_OUT( x, y )	do { x = y; } while (0)
#define LE32_OUT_FLOAT( x, y )	do { *(GLfloat *)&(x) = y; } while (0)
#else
#include <byteswap.h>
#define LE32_OUT( x, y )	do { x = bswap_32( y ); } while (0)
#define LE32_OUT_FLOAT( x, y )	do { x = bswap_32( *(unsigned int *)&y ); } while (0)
#endif

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		0
#define ENABLE_PERF_BOXES	0

#if DO_DEBUG
extern int R128_DEBUG;
#else
#define R128_DEBUG		0
#endif

#define DEBUG_ALWAYS_SYNC	0x01
#define DEBUG_VERBOSE_API	0x02
#define DEBUG_VERBOSE_MSG	0x04
#define DEBUG_VERBOSE_LRU	0x08
#define DEBUG_VERBOSE_DRI	0x10
#define DEBUG_VERBOSE_IOCTL	0x20
#define DEBUG_VERBOSE_2D	0x40

#endif
#endif /* __R128_CONTEXT_H__ */
