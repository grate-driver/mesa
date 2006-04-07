/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "utils.h"

#include "intel_screen.h"
#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "intel_regions.h"

#include "i830_context.h"
#include "i830_reg.h"

/* A large amount of state doesn't need to be uploaded.
 */
#define ACTIVE (I830_UPLOAD_INVARIENT |         \
		I830_UPLOAD_CTX |		\
		I830_UPLOAD_BUFFERS |		\
		I830_UPLOAD_STIPPLE |		\
		I830_UPLOAD_TEXBLEND(0) |	\
		I830_UPLOAD_TEX(0))		


#define SET_STATE( i830, STATE )		\
do {						\
   assert(!i830->intel.prim.flush); \
   i830->current->emitted = 0;			\
   i830->current = &i830->STATE;		\
   i830->current->emitted = 0;			\
} while (0)


static void set_no_stencil_write( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_FALSE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_STENCIL_WRITE;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_STENCIL_WRITE;

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

static void set_no_depth_write( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= DISABLE_DEPTH_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= DISABLE_DEPTH_WRITE;

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

/* Set depth unit to replace.
 */
static void set_depth_replace( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_DEPTH_TEST, GL_FALSE )
    * ctx->Driver.DepthMask( ctx, GL_TRUE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] &= ~ENABLE_DIS_DEPTH_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~ENABLE_DIS_DEPTH_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_DEPTH_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_DEPTH_WRITE;

   /* ctx->Driver.DepthFunc( ctx, GL_ALWAYS )
    */
   i830->meta.Ctx[I830_CTXREG_STATE3] &= ~DEPTH_TEST_FUNC_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE3] |= (ENABLE_DEPTH_TEST_FUNC |
					  DEPTH_TEST_FUNC(COMPAREFUNC_ALWAYS));

   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


/* Set stencil unit to replace always with the reference value.
 */
static void set_stencil_replace( struct intel_context *intel,
				 GLuint s_mask,
				 GLuint s_clear)
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   /* ctx->Driver.Enable( ctx, GL_STENCIL_TEST, GL_TRUE )
    */
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] |= ENABLE_STENCIL_TEST;
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= ENABLE_STENCIL_WRITE;

   /* ctx->Driver.StencilMask( ctx, s_mask )
    */
   i830->meta.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_WRITE_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_WRITE_MASK |
					   STENCIL_WRITE_MASK((s_mask&0xff)));

   /* ctx->Driver.StencilOp( ctx, GL_REPLACE, GL_REPLACE, GL_REPLACE )
    */
   i830->meta.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_OPS_MASK);
   i830->meta.Ctx[I830_CTXREG_STENCILTST] |= 
      (ENABLE_STENCIL_PARMS |
       STENCIL_FAIL_OP(STENCILOP_REPLACE) |
       STENCIL_PASS_DEPTH_FAIL_OP(STENCILOP_REPLACE) |
       STENCIL_PASS_DEPTH_PASS_OP(STENCILOP_REPLACE));

   /* ctx->Driver.StencilFunc( ctx, GL_ALWAYS, s_clear, ~0 )
    */
   i830->meta.Ctx[I830_CTXREG_STATE4] &= ~MODE4_ENABLE_STENCIL_TEST_MASK;
   i830->meta.Ctx[I830_CTXREG_STATE4] |= (ENABLE_STENCIL_TEST_MASK |
					   STENCIL_TEST_MASK(0xff));

   i830->meta.Ctx[I830_CTXREG_STENCILTST] &= ~(STENCIL_REF_VALUE_MASK |
						ENABLE_STENCIL_TEST_FUNC_MASK);
   i830->meta.Ctx[I830_CTXREG_STENCILTST] |= 
      (ENABLE_STENCIL_REF_VALUE |
       ENABLE_STENCIL_TEST_FUNC |
       STENCIL_REF_VALUE((s_clear&0xff)) |
       STENCIL_TEST_FUNC(COMPAREFUNC_ALWAYS));



   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


static void set_color_mask( struct intel_context *intel, GLboolean state )
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   const GLuint mask = ((1 << WRITEMASK_RED_SHIFT) |
			(1 << WRITEMASK_GREEN_SHIFT) |
			(1 << WRITEMASK_BLUE_SHIFT) |
			(1 << WRITEMASK_ALPHA_SHIFT));

   i830->meta.Ctx[I830_CTXREG_ENABLES_2] &= ~mask;

   if (state) {
      i830->meta.Ctx[I830_CTXREG_ENABLES_2] |= 
	 (i830->state.Ctx[I830_CTXREG_ENABLES_2] & mask);
   }
      
   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}

/* Installs a one-stage passthrough texture blend pipeline.  Is there
 * more that can be done to turn off texturing?
 */
static void set_no_texture( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   static const struct gl_tex_env_combine_state comb = {
      GL_NONE, GL_NONE,
      { GL_TEXTURE, 0, 0, }, { GL_TEXTURE, 0, 0, },
      { GL_SRC_COLOR, 0, 0 }, { GL_SRC_ALPHA, 0, 0 },
      0, 0, 0, 0
   };

   i830->meta.TexBlendWordsUsed[0] =
     i830SetTexEnvCombine( i830, & comb, 0, TEXBLENDARG_TEXEL0,
			   i830->meta.TexBlend[0], NULL);

   i830->meta.TexBlend[0][0] |= TEXOP_LAST_STAGE;
   i830->meta.emitted &= ~I830_UPLOAD_TEXBLEND(0);
}

/* Set up a single element blend stage for 'replace' texturing with no
 * funny ops.
 */
<<<<<<< i830_metaops.c
static void enable_texture_blend_replace( i830ContextPtr i830 )
=======
static void set_texture_blend_replace( struct intel_context *intel )
>>>>>>> 1.6.2.9
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   static const struct gl_tex_env_combine_state comb = {
      GL_REPLACE, GL_REPLACE,
<<<<<<< i830_metaops.c
      { GL_TEXTURE, GL_TEXTURE, GL_TEXTURE }, { GL_TEXTURE, GL_TEXTURE, GL_TEXTURE, },
      { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_COLOR }, { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA },
=======
      { GL_TEXTURE, GL_TEXTURE, GL_TEXTURE, }, { GL_TEXTURE, GL_TEXTURE, GL_TEXTURE, },
      { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_COLOR }, { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA },
>>>>>>> 1.6.2.9
      0, 0, 1, 1
   };

   i830->meta.TexBlendWordsUsed[0] =
     i830SetTexEnvCombine( i830, & comb, 0, TEXBLENDARG_TEXEL0,
			   i830->meta.TexBlend[0], NULL);

   i830->meta.TexBlend[0][0] |= TEXOP_LAST_STAGE;
   i830->meta.emitted &= ~I830_UPLOAD_TEXBLEND(0);

/*    fprintf(stderr, "%s: TexBlendWordsUsed[0]: %d\n",  */
/* 	   __FUNCTION__, i830->meta.TexBlendWordsUsed[0]); */
}



/* Set up an arbitary piece of memory as a rectangular texture
 * (including the front or back buffer).
 */
<<<<<<< i830_metaops.c
static void set_tex_rect_source( i830ContextPtr i830,
				 GLuint offset,
				 GLuint width, 
				 GLuint height,
				 GLuint pitch, /* in bytes */
				 GLuint textureFormat )
=======
static GLboolean set_tex_rect_source( struct intel_context *intel,
				      GLuint buffer,
				      GLuint offset,
				      GLuint pitch,
				      GLuint height,
				      GLenum format,
				      GLenum type)
>>>>>>> 1.6.2.9
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   GLuint *setup = i830->meta.Tex[0];
   GLint numLevels = 1;
   GLuint textureFormat;
   GLuint cpp;

   /* A full implementation of this would do the upload through
    * glTexImage2d, and get all the conversion operations at that
    * point.  We are restricted, but still at least have access to the
    * fragment program swizzle.
    */
   switch (format) {
   case GL_BGRA:
      switch (type) {
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_BYTE:
	 textureFormat = (MAPSURF_32BIT | MT_32BIT_ARGB8888);
	 cpp = 4;
	 break;
      default:
	 return GL_FALSE;
      }
      break;
   case GL_RGBA:
      switch (type) {
      case GL_UNSIGNED_INT_8_8_8_8_REV:
      case GL_UNSIGNED_BYTE:
	 textureFormat = (MAPSURF_32BIT | MT_32BIT_ABGR8888);
	 cpp = 4;
	 break;
      default:
	 return GL_FALSE;
      }
      break;
   case GL_BGR:
      switch (type) {
      case GL_UNSIGNED_SHORT_5_6_5_REV:
	 textureFormat = (MAPSURF_16BIT | MT_16BIT_RGB565);
	 cpp = 2;
	 break;
      default:
	 return GL_FALSE;
      }
      break;
   case GL_RGB:
      switch (type) {
      case GL_UNSIGNED_SHORT_5_6_5:
	 textureFormat = (MAPSURF_16BIT | MT_16BIT_RGB565);
	 cpp = 2;
	 break;
      default:
	 return GL_FALSE;
      }
      break;

<<<<<<< i830_metaops.c
/*    fprintf(stderr, "%s: offset: %x w: %d h: %d pitch %d format %x\n", */
/* 	   __FUNCTION__, offset, width, height, pitch, textureFormat ); */
=======
   default:
      return GL_FALSE;
   }

   i830->meta.tex_buffer[0] = buffer;
   i830->meta.tex_offset[0] = offset;
>>>>>>> 1.6.2.9

   setup[I830_TEXREG_TM0LI] = (_3DSTATE_LOAD_STATE_IMMEDIATE_2 | 
			       (LOAD_TEXTURE_MAP0 << 0) | 4);
   setup[I830_TEXREG_TM0S1] = (((height - 1) << TM0S1_HEIGHT_SHIFT) |
			       ((pitch - 1) << TM0S1_WIDTH_SHIFT) |
			       textureFormat);
   setup[I830_TEXREG_TM0S2] = (((((pitch * cpp) / 4) - 1) << TM0S2_PITCH_SHIFT)  |
			       TM0S2_CUBE_FACE_ENA_MASK);   

   setup[I830_TEXREG_TM0S3] = ( (((numLevels - 1)*4) << TM0S3_MIN_MIP_SHIFT) |
				(FILTER_NEAREST << TM0S3_MIN_FILTER_SHIFT) |
				(MIPFILTER_NONE << TM0S3_MIP_FILTER_SHIFT) |
				(FILTER_NEAREST << TM0S3_MAG_FILTER_SHIFT));

   setup[I830_TEXREG_CUBE] = (_3DSTATE_MAP_CUBE | MAP_UNIT(0));

   setup[I830_TEXREG_MCS] = (_3DSTATE_MAP_COORD_SET_CMD |
			     MAP_UNIT(0) |
			     ENABLE_TEXCOORD_PARAMS |
			     TEXCOORDS_ARE_IN_TEXELUNITS |
			     TEXCOORDTYPE_CARTESIAN |
			     ENABLE_ADDR_V_CNTL |
			     TEXCOORD_ADDR_V_MODE(TEXCOORDMODE_WRAP) |
			     ENABLE_ADDR_U_CNTL |
			     TEXCOORD_ADDR_U_MODE(TEXCOORDMODE_WRAP));

   i830->meta.emitted &= ~I830_UPLOAD_TEX(0);
   return GL_TRUE;
}


<<<<<<< i830_metaops.c
/* Select between front and back draw buffers.
 */
static void set_draw_region( i830ContextPtr i830,
			      const intelRegion *region )
{
   i830->meta.Buffer[I830_DESTREG_CBUFADDR1] =
      (BUF_3D_ID_COLOR_BACK | BUF_3D_PITCH(region->pitch) | BUF_3D_USE_FENCE);
   i830->meta.Buffer[I830_DESTREG_CBUFADDR2] = region->offset;
   i830->meta.emitted &= ~I830_UPLOAD_BUFFERS;
}

/* Setup an arbitary draw format, useful for targeting
 * texture or agp memory.
 */
#if 0
static void set_draw_format( i830ContextPtr i830,
			     GLuint format,
			     GLuint depth_format)
{
   i830->meta.Buffer[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) | /* .5 */
					  DSTORG_VERT_BIAS(0x8) | /* .5 */
					  format |
					  DEPTH_IS_Z |
					  depth_format);
}
#endif


static void set_vertex_format( i830ContextPtr i830 )
=======
static void set_vertex_format( struct intel_context *intel )
>>>>>>> 1.6.2.9
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   i830->meta.Ctx[I830_CTXREG_VF] =  (_3DSTATE_VFT0_CMD |
				      VFT0_TEX_COUNT(1) |
				      VFT0_DIFFUSE |
				      VFT0_XYZ);
   i830->meta.Ctx[I830_CTXREG_VF2] = (_3DSTATE_VFT1_CMD |
				      VFT1_TEX0_FMT(TEXCOORDFMT_2D) |
				      VFT1_TEX1_FMT(TEXCOORDFMT_2D) | 
				      VFT1_TEX2_FMT(TEXCOORDFMT_2D) |
				      VFT1_TEX3_FMT(TEXCOORDFMT_2D));
   i830->meta.emitted &= ~I830_UPLOAD_CTX;
}


static void meta_import_pixel_state( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);

   i830->meta.Ctx[I830_CTXREG_STATE1] = i830->state.Ctx[I830_CTXREG_STATE1];
   i830->meta.Ctx[I830_CTXREG_STATE2] = i830->state.Ctx[I830_CTXREG_STATE2];
   i830->meta.Ctx[I830_CTXREG_STATE3] = i830->state.Ctx[I830_CTXREG_STATE3];
   i830->meta.Ctx[I830_CTXREG_STATE4] = i830->state.Ctx[I830_CTXREG_STATE4];
   i830->meta.Ctx[I830_CTXREG_STATE5] = i830->state.Ctx[I830_CTXREG_STATE5];
   i830->meta.Ctx[I830_CTXREG_IALPHAB] = i830->state.Ctx[I830_CTXREG_IALPHAB];
   i830->meta.Ctx[I830_CTXREG_STENCILTST] = i830->state.Ctx[I830_CTXREG_STENCILTST];
   i830->meta.Ctx[I830_CTXREG_ENABLES_1] = i830->state.Ctx[I830_CTXREG_ENABLES_1];
   i830->meta.Ctx[I830_CTXREG_ENABLES_2] = i830->state.Ctx[I830_CTXREG_ENABLES_2];
   i830->meta.Ctx[I830_CTXREG_AA] = i830->state.Ctx[I830_CTXREG_AA];
   i830->meta.Ctx[I830_CTXREG_FOGCOLOR] = i830->state.Ctx[I830_CTXREG_FOGCOLOR];
   i830->meta.Ctx[I830_CTXREG_BLENDCOLOR0] = i830->state.Ctx[I830_CTXREG_BLENDCOLOR0];
   i830->meta.Ctx[I830_CTXREG_BLENDCOLOR1] = i830->state.Ctx[I830_CTXREG_BLENDCOLOR1];
   i830->meta.Ctx[I830_CTXREG_MCSB0] = i830->state.Ctx[I830_CTXREG_MCSB0];
   i830->meta.Ctx[I830_CTXREG_MCSB1] = i830->state.Ctx[I830_CTXREG_MCSB1];
   
<<<<<<< i830_metaops.c
/*    fprintf(stderr, "%s: %f,%f-%f,%f 0x%x%x%x%x %f,%f-%f,%f\n", */
/* 	   __FUNCTION__, */
/* 	   x0,y0,x1,y1,red,green,blue,alpha,s0,t0,s1,t1); */


   /* initial vertex, left bottom */
   tmp.v.x = x0;
   tmp.v.y = y0;
   tmp.v.z = 1.0;
   tmp.v.w = 1.0;
   tmp.v.color.red = red;
   tmp.v.color.green = green;
   tmp.v.color.blue = blue;
   tmp.v.color.alpha = alpha;
   tmp.v.specular.red = 0;
   tmp.v.specular.green = 0;
   tmp.v.specular.blue = 0;
   tmp.v.specular.alpha = 0;
   tmp.v.u0 = s0;
   tmp.v.v0 = t0;
   for (i = 0 ; i < 8 ; i++)
      vb[i] = tmp.ui[i];

   /* right bottom */
   vb += 8;
   tmp.v.x = x1;
   tmp.v.u0 = s1;
   for (i = 0 ; i < 8 ; i++)
      vb[i] = tmp.ui[i];

   /* right top */
   vb += 8;
   tmp.v.y = y1;
   tmp.v.v0 = t1;
   for (i = 0 ; i < 8 ; i++)
      vb[i] = tmp.ui[i];

   /* left top */
   vb += 8;
   tmp.v.x = x0;
   tmp.v.u0 = s0;
   for (i = 0 ; i < 8 ; i++)
      vb[i] = tmp.ui[i];

/*    fprintf(stderr, "%s: DV1: %x\n",  */
/* 	   __FUNCTION__, i830->meta.Buffer[I830_DESTREG_DV1]); */
}

static void draw_poly(i830ContextPtr i830, 
		      GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha,
                      GLuint numVerts,
                      GLfloat verts[][2],
                      GLfloat texcoords[][2])
{
   GLuint vertex_size = 8;
   GLuint *vb = intelEmitInlinePrimitiveLocked( &i830->intel, 
						PRIM3D_TRIFAN, 
						numVerts * vertex_size,
						vertex_size );
   intelVertex tmp;
   int i, k;

   /* initial constant vertex fields */
   tmp.v.z = 1.0;
   tmp.v.w = 1.0; 
   tmp.v.color.red = red;
   tmp.v.color.green = green;
   tmp.v.color.blue = blue;
   tmp.v.color.alpha = alpha;
   tmp.v.specular.red = 0;
   tmp.v.specular.green = 0;
   tmp.v.specular.blue = 0;
   tmp.v.specular.alpha = 0;

   for (k = 0; k < numVerts; k++) {
      tmp.v.x = verts[k][0];
      tmp.v.y = verts[k][1];
      tmp.v.u0 = texcoords[k][0];
      tmp.v.v0 = texcoords[k][1];

      for (i = 0 ; i < vertex_size ; i++)
         vb[i] = tmp.ui[i];

      vb += vertex_size;
   }
}

void 
i830ClearWithTris(intelContextPtr intel, GLbitfield mask,
		  GLboolean all,
		  GLint cx, GLint cy, GLint cw, GLint ch)
{
   i830ContextPtr i830 = I830_CONTEXT( intel );
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   intelScreenPrivate *screen = intel->intelScreen;
   int x0, y0, x1, y1;

   INTEL_FIREVERTICES(intel);
   SET_STATE( i830, meta );
   set_initial_state( i830 );
/*    set_no_texture( i830 ); */
   set_vertex_format( i830 ); 

   LOCK_HARDWARE(intel);

   if(!all) {
      x0 = cx;
      y0 = cy;
      x1 = x0 + cw;
      y1 = y0 + ch;
   } else {
      x0 = 0;
      y0 = 0;
      x1 = x0 + dPriv->w;
      y1 = y0 + dPriv->h;
   }
=======
>>>>>>> 1.6.2.9

   i830->meta.Ctx[I830_CTXREG_STATE3] &= ~CULLMODE_MASK;
   i830->meta.Stipple[I830_STPREG_ST1] &= ~ST1_ENABLE;
   i830->meta.emitted &= ~I830_UPLOAD_CTX;

<<<<<<< i830_metaops.c
   if(mask & BUFFER_BIT_FRONT_LEFT) {
      set_no_depth_stencil_write( i830 );
      set_color_mask( i830, GL_TRUE );
      set_draw_region( i830, &screen->front );
      draw_quad(i830, x0, x1, y0, y1,
		intel->clear_red, intel->clear_green,
		intel->clear_blue, intel->clear_alpha,
		0, 0, 0, 0);
   }
=======
>>>>>>> 1.6.2.9

<<<<<<< i830_metaops.c
   if(mask & BUFFER_BIT_BACK_LEFT) {
      set_no_depth_stencil_write( i830 );
      set_color_mask( i830, GL_TRUE );
      set_draw_region( i830, &screen->back );

      draw_quad(i830, x0, x1, y0, y1,
		intel->clear_red, intel->clear_green,
		intel->clear_blue, intel->clear_alpha,
		0, 0, 0, 0);
   }

   if(mask & BUFFER_BIT_STENCIL) {
      set_stencil_replace( i830, 
			   intel->ctx.Stencil.WriteMask[0], 
			   intel->ctx.Stencil.Clear);

      set_color_mask( i830, GL_FALSE );
      set_draw_region( i830, &screen->front );
      draw_quad( i830, x0, x1, y0, y1, 0, 0, 0, 0, 0, 0, 0, 0 );
   }

   UNLOCK_HARDWARE(intel);

   INTEL_FIREVERTICES(intel);
   SET_STATE( i830, state );
=======
   i830->meta.Buffer[I830_DESTREG_SENABLE] = i830->state.Buffer[I830_DESTREG_SENABLE];
   i830->meta.Buffer[I830_DESTREG_SR1] = i830->state.Buffer[I830_DESTREG_SR1];
   i830->meta.Buffer[I830_DESTREG_SR2] = i830->state.Buffer[I830_DESTREG_SR2];
   i830->meta.emitted &= ~I830_UPLOAD_BUFFERS;
>>>>>>> 1.6.2.9
}


<<<<<<< i830_metaops.c
#if 0
=======

/* Select between front and back draw buffers.
 */
static void meta_draw_region( struct intel_context *intel,
			      struct intel_region *draw_region,
			      struct intel_region *depth_region )
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   GLuint format;
   GLuint depth_format = DEPTH_FRMT_16_FIXED;
>>>>>>> 1.6.2.9

   intel_region_release(intel, &i830->meta.draw_region);
   intel_region_reference(&i830->meta.draw_region, draw_region);
   
<<<<<<< i830_metaops.c
   
      enable_texture_blend_replace( i830 ); 


      /* Set the 3d engine to draw into the agp memory
       */

      set_draw_region( i830, destOffset ); 
      set_draw_format( i830, destFormat, depthFormat );  


      /* Draw a single quad, no cliprects:
       */
      i830->intel.numClipRects = 1;
      i830->intel.pClipRects = &tmp;
      i830->intel.pClipRects[0].x1 = 0;
      i830->intel.pClipRects[0].y1 = 0;
      i830->intel.pClipRects[0].x2 = width;
      i830->intel.pClipRects[0].y2 = height;
=======
   intel_region_release(intel, &i830->meta.depth_region);   
   intel_region_reference(&i830->meta.depth_region, depth_region);
>>>>>>> 1.6.2.9

   /* XXX FBO: grab code from i915 meta_draw_region */

   /* XXX: 555 support?
    */
   if (draw_region->cpp == 2)
      format = DV_PF_565;
   else
      format = DV_PF_8888;

   if (depth_region) {
      if (depth_region->cpp == 2)
	 depth_format = DEPTH_FRMT_16_FIXED;
      else
	 depth_format = DEPTH_FRMT_24_FIXED_8_OTHER;
   }

<<<<<<< i830_metaops.c
   /* Todo -- don't want to clobber all the drawing state like we do
    * for readpixels -- most of this state can be handled just fine.
    */
   if (	ctx->_ImageTransferState ||
	unpack->SwapBytes ||
	unpack->LsbFirst ||
	ctx->Color.AlphaEnabled || 
	ctx->Depth.Test ||
	ctx->Fog.Enabled ||
	ctx->Scissor.Enabled ||
	ctx->Stencil.Enabled ||
	!ctx->Color.ColorMask[0] ||
	!ctx->Color.ColorMask[1] ||
	!ctx->Color.ColorMask[2] ||
	!ctx->Color.ColorMask[3] ||
	ctx->Color.ColorLogicOpEnabled ||
	ctx->Texture._EnabledUnits ||
	ctx->Depth.OcclusionTest) {
      fprintf(stderr, "%s: other tests failed\n", __FUNCTION__);
      return GL_FALSE;
   }
=======
   i830->meta.Buffer[I830_DESTREG_DV1] = (DSTORG_HORT_BIAS(0x8) | /* .5 */
					  DSTORG_VERT_BIAS(0x8) | /* .5 */
					  format |
					  DEPTH_IS_Z |
					  depth_format);
>>>>>>> 1.6.2.9

   i830->meta.emitted &= ~I830_UPLOAD_BUFFERS;
}


/* Operations where the 3D engine is decoupled temporarily from the
 * current GL state and used for other purposes than simply rendering
 * incoming triangles.
 */
static void install_meta_state( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   memcpy(&i830->meta, &i830->initial, sizeof(i830->meta) );

   i830->meta.active = ACTIVE;
   i830->meta.emitted = 0;

   SET_STATE(i830, meta);
   set_vertex_format(intel);
   set_no_texture(intel);
}

static void leave_meta_state( struct intel_context *intel )
{
   struct i830_context *i830 = i830_context(&intel->ctx);
   intel_region_release(intel, &i830->meta.draw_region);
   intel_region_release(intel, &i830->meta.depth_region);
/*    intel_region_release(intel, &i830->meta.tex_region[0]); */
   SET_STATE(i830, state);
}



void i830InitMetaFuncs( struct i830_context *i830 )
{
   i830->intel.vtbl.install_meta_state = install_meta_state;
   i830->intel.vtbl.leave_meta_state = leave_meta_state;
   i830->intel.vtbl.meta_no_depth_write = set_no_depth_write;
   i830->intel.vtbl.meta_no_stencil_write = set_no_stencil_write;
   i830->intel.vtbl.meta_stencil_replace = set_stencil_replace;
   i830->intel.vtbl.meta_depth_replace = set_depth_replace;
   i830->intel.vtbl.meta_color_mask = set_color_mask;
   i830->intel.vtbl.meta_no_texture = set_no_texture;
   i830->intel.vtbl.meta_texture_blend_replace = set_texture_blend_replace;
   i830->intel.vtbl.meta_tex_rect_source = set_tex_rect_source;
   i830->intel.vtbl.meta_draw_region = meta_draw_region;
   i830->intel.vtbl.meta_import_pixel_state = meta_import_pixel_state;
}




<<<<<<< i830_metaops.c
      /* Set the pixel image up as a rectangular texture.
       */
      set_tex_rect_source( i830, 
			   src_offset, 
			   width, 
			   height, 
			   pitch, /* XXXX!!!! -- /2 sometimes */
			   textureFormat ); 
   
   
      enable_texture_blend_replace( i830 ); 

   
      /* Draw to the current draw buffer:
       */
      set_draw_offset( i830, dst_offset );

      /* Draw a quad, use regular cliprects
       */
/*       fprintf(stderr, "x: %d y: %d width %d height %d\n", x, y, width, height); */

      draw_quad( i830, 
		 x, x+width, y, y+height,
		 0, 255, 0, 0, 
		 0, width, 0, height );

      intelWindowMoved( intel );
   }
   UNLOCK_HARDWARE( intel );
   intelFinish( ctx ); /* required by GL */
   
   SET_STATE(i830, state);

   return GL_TRUE;
}
=======
>>>>>>> 1.6.2.9

#endif

/**
 * Copy the window contents named by dPriv to the rotated (or reflected)
 * color buffer.
 * srcBuf is BUFFER_BIT_FRONT_LEFT or BUFFER_BIT_BACK_LEFT to indicate the source.
 */
void
i830RotateWindow(intelContextPtr intel, __DRIdrawablePrivate *dPriv,
                 GLuint srcBuf)
{
   i830ContextPtr i830 = I830_CONTEXT( intel );
   intelScreenPrivate *screen = intel->intelScreen;
   const GLuint cpp = screen->cpp;
   drm_clip_rect_t fullRect;
   GLuint textureFormat, srcOffset, srcPitch;
   const drm_clip_rect_t *clipRects;
   int numClipRects;
   int i;

   int xOrig, yOrig;
   int origNumClipRects;
   drm_clip_rect_t *origRects;

   /*
    * set up hardware state
    */
   intelFlush( &intel->ctx );

   SET_STATE( i830, meta ); 
   set_initial_state( i830 ); 
   set_no_texture( i830 ); 
   set_vertex_format( i830 ); 
   set_no_depth_stencil_write( i830 );
   set_color_mask( i830, GL_FALSE );

   LOCK_HARDWARE(intel);

   /* save current drawing origin and cliprects (restored at end) */
   xOrig = intel->drawX;
   yOrig = intel->drawY;
   origNumClipRects = intel->numClipRects;
   origRects = intel->pClipRects;

   if (!intel->numClipRects)
      goto done;

   /*
    * set drawing origin, cliprects for full-screen access to rotated screen
    */
   fullRect.x1 = 0;
   fullRect.y1 = 0;
   fullRect.x2 = screen->rotatedWidth;
   fullRect.y2 = screen->rotatedHeight;
   intel->drawX = 0;
   intel->drawY = 0;
   intel->numClipRects = 1;
   intel->pClipRects = &fullRect;

   set_draw_region( i830, &screen->rotated );

   if (cpp == 4)
      textureFormat = MAPSURF_32BIT | MT_32BIT_ARGB8888;
   else
      textureFormat = MAPSURF_16BIT | MT_16BIT_RGB565;

   if (srcBuf == BUFFER_BIT_FRONT_LEFT) {
      srcPitch = screen->front.pitch;   /* in bytes */
      srcOffset = screen->front.offset; /* bytes */
      clipRects = dPriv->pClipRects;
      numClipRects = dPriv->numClipRects;
   }
   else {
      srcPitch = screen->back.pitch;   /* in bytes */
      srcOffset = screen->back.offset; /* bytes */
      clipRects = dPriv->pBackClipRects;
      numClipRects = dPriv->numBackClipRects;
   }

   /* set the whole screen up as a texture to avoid alignment issues */
   set_tex_rect_source(i830,
                       srcOffset,
                       screen->width,
                       screen->height,
                       srcPitch,
                       textureFormat);

   enable_texture_blend_replace(i830);

   /*
    * loop over the source window's cliprects
    */
   for (i = 0; i < numClipRects; i++) {
      int srcX0 = clipRects[i].x1;
      int srcY0 = clipRects[i].y1;
      int srcX1 = clipRects[i].x2;
      int srcY1 = clipRects[i].y2;
      GLfloat verts[4][2], tex[4][2];
      int j;

      /* build vertices for four corners of clip rect */
      verts[0][0] = srcX0;  verts[0][1] = srcY0;
      verts[1][0] = srcX1;  verts[1][1] = srcY0;
      verts[2][0] = srcX1;  verts[2][1] = srcY1;
      verts[3][0] = srcX0;  verts[3][1] = srcY1;

      /* .. and texcoords */
      tex[0][0] = srcX0;  tex[0][1] = srcY0;
      tex[1][0] = srcX1;  tex[1][1] = srcY0;
      tex[2][0] = srcX1;  tex[2][1] = srcY1;
      tex[3][0] = srcX0;  tex[3][1] = srcY1;

      /* transform coords to rotated screen coords */

      for (j = 0; j < 4; j++) {
         matrix23TransformCoordf(&screen->rotMatrix,
                                 &verts[j][0], &verts[j][1]);
      }

      /* draw polygon to map source image to dest region */
      draw_poly(i830, 255, 255, 255, 255, 4, verts, tex);

   } /* cliprect loop */

   assert(!intel->prim.flush); 
   intelFlushBatchLocked( intel, GL_FALSE, GL_FALSE, GL_FALSE );

 done:
   /* restore original drawing origin and cliprects */
   intel->drawX = xOrig;
   intel->drawY = yOrig;
   intel->numClipRects = origNumClipRects;
   intel->pClipRects = origRects;

   UNLOCK_HARDWARE(intel);

   SET_STATE( i830, state );
}

