/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * glBegin/EndQuery interface to pipe
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/image.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_cb_queryobj.h"


struct st_query_object
{
   struct gl_query_object base;
   struct pipe_query *pq;
};


/**
 * Cast wrapper
 */
static struct st_query_object *
st_query_object(struct gl_query_object *q)
{
   return (struct st_query_object *) q;
}


static struct gl_query_object *
st_NewQueryObject(GLcontext *ctx, GLuint id)
{
   struct st_query_object *stq = ST_CALLOC_STRUCT(st_query_object);
   if (stq) {
      stq->base.Id = id;
      stq->base.Ready = GL_TRUE;
      stq->pq = NULL;
      return &stq->base;
   }
   return NULL;
}



static void
st_DeleteQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   if (stq->pq) {
      pipe->destroy_query(pipe, stq->pq);
      stq->pq = NULL;
   }

   _mesa_free(stq);
}


static void
st_BeginQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   switch (q->Target) {
   case GL_SAMPLES_PASSED_ARB:
      if (!stq->pq)
	 stq->pq = pipe->create_query( pipe, PIPE_QUERY_OCCLUSION_COUNTER );
      break;
   default:
      assert(0);
      return;
   }

   pipe->begin_query(pipe, stq->pq);
}


static void
st_EndQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   pipe->end_query(pipe, stq->pq);
}


static void
st_WaitQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   /* this function should only be called if we don't have a ready result */
   assert(!stq->base.Ready);

   while (!stq->base.Ready &&
	  !pipe->get_query_result(pipe, 
				  stq->pq,
				  TRUE,
				  &q->Result))
   {
      /* nothing */
   }
			    
   q->Ready = GL_TRUE;
}


static void
st_CheckQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   if (!q->Ready) {
      q->Ready = pipe->get_query_result(pipe, 
					stq->pq,
					FALSE,
					&q->Result);
   }
}




void st_init_query_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = st_NewQueryObject;
   functions->DeleteQuery = st_DeleteQuery;
   functions->BeginQuery = st_BeginQuery;
   functions->EndQuery = st_EndQuery;
   functions->WaitQuery = st_WaitQuery;
   functions->CheckQuery = st_CheckQuery;
}
