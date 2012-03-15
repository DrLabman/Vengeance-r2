/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_surf.c: surface-related refresh code

#include "quakedef.h"

int		lightmap_bytes;		// 1, 2, or 4

int		lightmap_textures;

//#define	BLOCK_WIDTH		128
//#define	BLOCK_HEIGHT	128

//#define	MAX_LIGHTMAPS	1024
int			active_lightmaps;

//typedef struct glRect_s {
//	unsigned char l,t,w,h;
//} glRect_t;

//temp lightmap
unsigned		blocklights[BLOCK_WIDTH*BLOCK_HEIGHT*3];
//qmb :coloured lights

glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
qboolean	lightmap_modified[MAX_LIGHTMAPS];
glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[4*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

//world texture chains
msurface_t  *skychain = NULL;
msurface_t  *waterchain = NULL;
msurface_t	*extrachain = NULL;
msurface_t	*outlinechain = NULL;

//qmb :detail texture
int		detailtexture;
int		detailtexture2;

extern int multitex_go;

entity_t	*currententity;

void R_RenderDynamicLightmaps (msurface_t *fa);
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride);
void R_WriteLightCacheFromLeaf( mleaf_t *leaf, msurface_shader_light_cache_t *cache );
void R_WriteLightCacheFromPoint( vec3_t orgin, msurface_shader_light_cache_t *cache );

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	int		reletive;
	int		count;

	if (currententity && currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}
	
	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;	
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

lp1DMTexFUNC qglMTexCoord1fARB = NULL;
lpMTexFUNC qglMTexCoord2fARB = NULL;
lpSelTexFUNC qglSelectTextureARB = NULL;

void GL_SelectTexture (GLenum target);

void GL_EnableTMU(int tmu)
{
	GL_SelectTexture(tmu);
	glEnable(GL_TEXTURE_2D);
}

void GL_DisableTMU(int tmu)
{
	if (multitex_go)
	{
		GL_SelectTexture(tmu);
		glDisable(GL_TEXTURE_2D);
	}
}

void GL_VertexLight(vec3_t vertex)
{
	extern vec3_t	lightcolor;

	R_LightPoint(vertex);

	VectorScale(lightcolor, 1.0f / 100.0f, lightcolor);
	glColor3fv (lightcolor);
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

void Surf_DrawTextureChainsFour(model_t *model, int channels);
void Surf_DrawTextureChainsTwo(model_t *model, int channels);
void Surf_DrawTextureChainsOne(model_t *model, int channels);

void Surf_DrawTextureChainsShader(model_t *model, int channels);
void Surf_DrawTextureChainsLightPass(model_t *model, int channels);

static void R_WriteLightCacheFromLeaf( mleaf_t *leaf, msurface_shader_light_cache_t *cache ) {
	int i;
	memset( cache->mask, 0, sizeof( cache->mask ) );
	for( i = 0 ; i < R_MAX_SHADER_LIGHTS ; i++ ) {
		qboolean isVisible = R_Shader_IsLightInScopeByLeaf( i, leaf ); 
		if( isVisible ) {
			cache->mask[ i / 8 ] += 1 << (i & 7);
		}
	}
}

static void R_WriteLightCacheFromPoint( vec3_t origin, msurface_shader_light_cache_t *cache ) {
	int i;
	memset( cache->mask, 0, sizeof( cache->mask ) );
	for( i = 0 ; i < R_MAX_SHADER_LIGHTS ; i++ ) {
		qboolean isVisible = R_Shader_IsLightInScopeByPoint( i, origin ); 
		if( isVisible ) {
			cache->mask[ i / 8 ] += 1 << (i & 7);
		}
	}
}
/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node, float *modelorg)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	double		dot;

	//make sure we are still inside the world
	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	//is this node visible
	if (node->visframe != r_visframecount)
		return;

	//i think this checks if its on the screen and not behind the viewer
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;
	
// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		msurface_shader_light_cache_t lightCache;
	
		pleaf = (mleaf_t *)node;

		// determine the shader light visibility
        R_WriteLightCacheFromLeaf( pleaf, &lightCache );

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		while( c-- > 0 )
		{
			(*mark)->visframe = r_framecount;
			(*mark)->shaderLights = lightCache;
			mark++;
		}

	// deal with model fragments in this leaf
		if (pleaf->efrags)
			R_StoreEfrags (&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side], modelorg);

// recurse down the back side
	if (r_outline.value)
		R_RecursiveWorldNode (node->children[!side], modelorg);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = cl.worldmodel->surfaces + node->firstsurface;

		{
			for ( ; c ; c--, surf++)
			{
				if (surf->visframe != r_framecount)
					continue;

				if (surf->flags & SURF_DRAWSKY) {
					surf->texturechain = skychain;
					skychain = surf;
				} else if (surf->flags & SURF_DRAWTURB) {
					surf->texturechain = waterchain;
					waterchain = surf;
				} else {
					//add chain for drawing.
					surf->texturechain = surf->texinfo->texture->texturechain;
					surf->texinfo->texture->texturechain = surf;

					//setup eyecandy chain
					if ((gl_detail.value&&gl_textureunits<4)||(surf->flags & SURF_UNDERWATER && gl_caustics.value)||(surf->texinfo->texture->gl_fullbright!=0&&gl_textureunits < 3)||(surf->flags & SURF_SHINY_METAL && gl_shiny.value)||(surf->flags & SURF_SHINY_GLASS && gl_shiny.value))
					{
						surf->extra=extrachain;
						extrachain = surf;
					}

					if (r_outline.value){
						surf->outline=outlinechain;
						outlinechain = surf;
					}
				}
			}
		}
	}

// recurse down the back side
	if (!r_outline.value)
		R_RecursiveWorldNode (node->children[!side], modelorg);
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	int			k;
	vec3_t		mins, maxs;
	model_t		*clmodel;
	qboolean	rotated;
	int			i;
	//QMB: lit bmodels
	extern vec3_t	lightcolor;
	int			lnum;
	vec3_t		dist;
	float       add;
	float		modelorg[3];

	currententity = e;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

//Lighting for model
//dynamic lights for non lightmaped bmodels
	if (clmodel->firstmodelsurface == 0){
		R_LightPoint(e->origin);
		for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
		{
			if (cl_dlights[lnum].die >= cl.time)
			{
				VectorSubtract (e->origin, cl_dlights[lnum].origin, dist);
				add = cl_dlights[lnum].radius - Length(dist);
				if (add > 0)
				{
					lightcolor[0] += add * cl_dlights[lnum].colour[0];
					lightcolor[1] += add * cl_dlights[lnum].colour[1];
					lightcolor[2] += add * cl_dlights[lnum].colour[2];
				}
			}
		}
		VectorScale(lightcolor, 1.0f / 100.0f, lightcolor);

		if (gl_ammoflash.value){
			lightcolor[0] += sin(2 * cl.time * M_PI)/4;
			lightcolor[1] += sin(2 * cl.time * M_PI)/4;
			lightcolor[2] += sin(2 * cl.time * M_PI)/4;
		}

		VectorMultiply(lightcolor, e->baseline.colormod, lightcolor);

		glColor4f (lightcolor[0], lightcolor[1], lightcolor[2], e->alpha);
	}else {
		glColor4f (1.0,1.0,1.0,1.0);
		memset (lightmap_polys, 0, sizeof(lightmap_polys));
	}

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

    glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug

// calculate dynamic lighting for bmodel if it's not an
// instanced model
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		for (k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if ((cl_dlights[k].die < cl.time) ||
				(!cl_dlights[k].radius))
				continue;

			R_MarkLights (&cl_dlights[k], 1<<k,
				clmodel->nodes + clmodel->hulls[0].firstclipnode);
		}
	}

	R_DrawBrush (e, &modelorg[0]);

	glPopMatrix ();
}

void R_DrawBrush (entity_t *e, float *modelorg)
{
	model_t *clmodel = e->model;
	int			i;
	msurface_t	*surf;
	msurface_shader_light_cache_t lightCache;

	{
		vec3_t center;
		VectorLerp( e->model->mins, 0.5, e->model->maxs, center );
		VectorAdd( e->origin, center, center );
		R_WriteLightCacheFromPoint( center, &lightCache );
	}

	surf = &clmodel->surfaces[clmodel->firstmodelsurface];
	//if the chains haven't been made then make them
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, surf++)
	{
		float		dot;
		mplane_t	*pplane;
	// find which side of the node we are on
		pplane = surf->plane;

		switch (pplane->type)
		{
		case PLANE_X:
			dot = modelorg[0] - pplane->dist;
			break;
		case PLANE_Y:
			dot = modelorg[1] - pplane->dist;
			break;
		case PLANE_Z:
			dot = modelorg[2] - pplane->dist;
			break;
		default:
			dot = DotProduct (modelorg, pplane->normal) - pplane->dist;
			break;
		}

		surf->shaderLights = lightCache;

	// draw the polygon
		if (r_outline.value||
			((surf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(surf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (surf->flags & SURF_DRAWSKY) {
				surf->texturechain = skychain;
				skychain = surf;
			} else if (surf->flags & SURF_DRAWTURB) {
				surf->texturechain = waterchain;
				waterchain = surf;
			} else {
				//add chain for drawing.
				surf->texturechain = surf->texinfo->texture->texturechain;
				surf->texinfo->texture->texturechain = surf;

				//setup eyecandy chain
				if ((gl_detail.value&&gl_textureunits<4)||(surf->flags & SURF_UNDERWATER && gl_caustics.value)||(surf->texinfo->texture->gl_fullbright!=0&&gl_textureunits < 3)||(surf->flags & SURF_SHINY_METAL && gl_shiny.value)||(surf->flags & SURF_SHINY_GLASS && gl_shiny.value))
				{
					if (!R_Skybox){
						surf->extra=extrachain;
						extrachain = surf;
					}
				}
				if (r_outline.value){
					surf->outline=outlinechain;
					outlinechain = surf;
				}
			}
		}
	}

	// draw PPL lighting
	if(r_shadow_realtime_draw_world.value && R_Shader_CanRenderLights() ) {
		Surf_DrawTextureChainsShader( clmodel, true );
		Surf_DrawTextureChainsLightPass( clmodel, true );
	} else
	if (!multitex_go)
		Surf_DrawTextureChainsOne (clmodel, true);
	else if (gl_textureunits >= 4)
		Surf_DrawTextureChainsFour (clmodel, true);
	else
		Surf_DrawTextureChainsTwo (clmodel, true);
}

/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;
	float		modelorg[3];

	memset (&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy (r_refdef.vieworg, modelorg);

	// start MPO
	// if this is a reflection we're drawing, we need to flip vertically across the water
	if (g_drawing_refl)
	{
		float distance;
		
		distance = DotProduct(modelorg, waterNormals[g_active_refl]) - g_waterDistance2[g_active_refl]; 
		VectorMA(r_refdef.vieworg, distance*-2, waterNormals[g_active_refl], modelorg);
	}
	// stop MPO

	glColor3f (1,1,1);
	memset (lightmap_polys, 0, sizeof(lightmap_polys));

//draw the display list
	R_RecursiveWorldNode (cl.worldmodel->nodes, &modelorg[0]);

	// draw PPL lighting
	if(r_shadow_realtime_draw_world.value && R_Shader_CanRenderLights() ) {
		Surf_DrawTextureChainsShader( cl.worldmodel, false );
		Surf_DrawTextureChainsLightPass( cl.worldmodel, false );
	} else
	//draw the world normally
	if (!multitex_go)
		Surf_DrawTextureChainsOne (cl.worldmodel, false);
	else if (gl_textureunits >= 4)
		Surf_DrawTextureChainsFour (cl.worldmodel, false);
	else
		Surf_DrawTextureChainsTwo (cl.worldmodel, false);
}

/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;
	byte	solid[4096];

	// fixes weird invisi-wall bug with mh's water
//	if (r_oldviewleaf == r_viewleaf && !r_novis.value)
//		return;
	
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value)
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	}
	else
		vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);
		
	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

/*
================
BuildSurfaceDisplayList
================
*/
void BuildSurfaceDisplayList (model_t *m, msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;

// reconstruct the polygon
	pedges = m->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = m->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = m->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = m->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= 128;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= 128;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][7] = s;
		poly->verts[i][8] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER) )
	{
		for (i = 0 ; i < lnumverts ; ++i)
		{
			vec3_t v1, v2;
			float *prev, *this, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			this = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract( this, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001
			if ((fabs( v1[0] - v2[0] ) <= COLINEAR_EPSILON) &&
				(fabs( v1[1] - v2[1] ) <= COLINEAR_EPSILON) && 
				(fabs( v1[2] - v2[2] ) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;
}	

//==============================================================================================
//Lightmaps
//==============================================================================================
/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_LIGHTMAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	Sys_Error ("AllocBlock: full");

	return 0;
}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;
	R_BuildLightMap (surf, base, BLOCK_WIDTH*lightmap_bytes);
}

void GL_UploadLightmap (void) 
{
	int i;

	GL_SelectTexture(GL_TEXTURE0_ARB);

	//
	// upload all lightmaps that were filled
	//
	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		glBindTexture(GL_TEXTURE_2D,lightmap_textures + i);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, lightmap_bytes, BLOCK_WIDTH, BLOCK_HEIGHT, 0, gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*lightmap_bytes);
	}
	GL_SelectTexture(GL_TEXTURE0_ARB);
}

/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int		i,j;
	model_t	*m;
	extern vec3_t	lightcolor;

	memset (allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	if (!lightmap_textures)
	{
		lightmap_textures = texture_extension_number;
		texture_extension_number += MAX_LIGHTMAPS;
	}

	gl_lightmap_format = GL_RGBA;
	lightmap_bytes = 4;

	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		for (i=0 ; i<m->numsurfaces ; i++)
		{
			GL_CreateSurfaceLightmap (m->surfaces + i);
			if ( m->surfaces[i].flags & SURF_DRAWTURB )
				continue;

			BuildSurfaceDisplayList (m, m->surfaces + i);
		}
	}
	GL_UploadLightmap();
}

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int         lnum;
	int         sd, td;
	float       dist, rad, minlight;
	vec3_t      impact, local;
	int         s, t;
	int         i;
	int         smax, tmax;
	mtexinfo_t  *tex;
// LordHavoc: .lit support begin
	float		cred, cgreen, cblue, brightness;
	unsigned	*bl;
// LordHavoc: .lit support end   

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;       // not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) - surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++) {
			impact[i] = cl_dlights[lnum].origin[i] - surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
     		
// LordHavoc: .lit support begin
		bl = blocklights;
		cred = cl_dlights[lnum].colour[0] * 256.0f;
		cgreen = cl_dlights[lnum].colour[1] * 256.0f;
		cblue = cl_dlights[lnum].colour[2] * 256.0f;
// LordHavoc: .lit support end

		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
			td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
				{
					brightness = rad - dist;
					bl[0] += (int) (brightness * cred);
					bl[1] += (int) (brightness * cgreen);
					bl[2] += (int) (brightness * cblue);
				}
				bl += 3;
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			t;
	int			i, j, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	unsigned	*bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (!cl.worldmodel->lightdata)
	{
		bl = blocklights;
		for (i=0 ; i<size ; i++)
		{
			*bl++ = 255*256;//r
			*bl++ = 255*256;//g
			*bl++ = 255*256;//b
		}
		goto store;
	}

// clear to no light
	bl = blocklights;
	for (i=0 ; i<size ; i++)
	{
		*bl++ = 0;//r
		*bl++ = 0;//g
		*bl++ = 0;//b
	}
	
// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			
			bl = blocklights;
			for (i=0 ; i<size ; i++)
			{
				*bl++ += *lightmap++ * scale;//r
				*bl++ += *lightmap++ * scale;//g
				*bl++ += *lightmap++ * scale;//b
			}
			
		}
	
		//qmb :overbright dynamic ligths only
		//so scale back the normal lightmap so it renders normally
		bl=blocklights;
		for (i=0; i<size*3 && gl_combine; i++)
			*bl++ >>= 2;

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

// bound, invert, and shift
store:

	stride -= (smax<<2);
	bl = blocklights;
	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			//FIXME: should scale back so it remains the right colour
			// LordHavoc: .lit support begin
			t = bl[0] >> 7;if (t > 255) t = 255;*dest++ = t;//r
			t = bl[1] >> 7;if (t > 255) t = 255;*dest++ = t;//g
			t = bl[2] >> 7;if (t > 255) t = 255;*dest++ = t;//b
			bl = bl + 3;
			*dest++ = 255;
			// LordHavoc: .lit support end
		}
	}
}

/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps (msurface_t *fa)
{
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;

	c_brush_polys++;

	if (fa->flags & ( SURF_DRAWSKY | SURF_DRAWTURB) )
		return;

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == r_framecount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (r_dynamic.value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
			R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
		}
	}
}
