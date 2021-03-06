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
// mathlib.c -- math primitives

#include <math.h>
#include "quakedef.h"

vec3_t vec3_origin = {0,0,0};
float ixtable[4096];

float	degRadScalar;
double	angleModScalar1;
double	angleModScalar2;

void Math_Init()
{
	degRadScalar	= (float)(M_PI / 180.0);
	angleModScalar1	= (360.0 / 65536.0);
	angleModScalar2	= (65536.0 / 360.0);
}

/*-----------------------------------------------------------------*/

float m_bytenormals[NUMVERTEXNORMALS][3] =
{
{-0.525731f,  0.000000f,  0.850651f}, {-0.442863f,  0.238856f,  0.864188f}, 
{-0.295242f,  0.000000f,  0.955423f}, {-0.309017f,  0.500000f,  0.809017f}, 
{-0.162460f,  0.262866f,  0.951056f}, { 0.000000f,  0.000000f,  1.000000f}, 
{ 0.000000f,  0.850651f,  0.525731f}, {-0.147621f,  0.716567f,  0.681718f}, 
{ 0.147621f,  0.716567f,  0.681718f}, { 0.000000f,  0.525731f,  0.850651f}, 
{ 0.309017f,  0.500000f,  0.809017f}, { 0.525731f,  0.000000f,  0.850651f}, 
{ 0.295242f,  0.000000f,  0.955423f}, { 0.442863f,  0.238856f,  0.864188f}, 
{ 0.162460f,  0.262866f,  0.951056f}, {-0.681718f,  0.147621f,  0.716567f}, 
{-0.809017f,  0.309017f,  0.500000f}, {-0.587785f,  0.425325f,  0.688191f}, 
{-0.850651f,  0.525731f,  0.000000f}, {-0.864188f,  0.442863f,  0.238856f},
{-0.716567f,  0.681718f,  0.147621f}, {-0.688191f,  0.587785f,  0.425325f}, 
{-0.500000f,  0.809017f,  0.309017f}, {-0.238856f,  0.864188f,  0.442863f}, 
{-0.425325f,  0.688191f,  0.587785f}, {-0.716567f,  0.681718f, -0.147621f}, 
{-0.500000f,  0.809017f, -0.309017f}, {-0.525731f,  0.850651f,  0.000000f}, 
{ 0.000000f,  0.850651f, -0.525731f}, {-0.238856f,  0.864188f, -0.442863f},
{ 0.000000f,  0.955423f, -0.295242f}, {-0.262866f,  0.951056f, -0.162460f}, 
{ 0.000000f,  1.000000f,  0.000000f}, { 0.000000f,  0.955423f,  0.295242f},
{-0.262866f,  0.951056f,  0.162460f}, { 0.238856f,  0.864188f,  0.442863f}, 
{ 0.262866f,  0.951056f,  0.162460f}, { 0.500000f,  0.809017f,  0.309017f}, 
{ 0.238856f,  0.864188f, -0.442863f}, { 0.262866f,  0.951056f, -0.162460f}, 
{ 0.500000f,  0.809017f, -0.309017f}, { 0.850651f,  0.525731f,  0.000000f}, 
{ 0.716567f,  0.681718f,  0.147621f}, { 0.716567f,  0.681718f, -0.147621f}, 
{ 0.525731f,  0.850651f,  0.000000f}, { 0.425325f,  0.688191f,  0.587785f}, 
{ 0.864188f,  0.442863f,  0.238856f}, { 0.688191f,  0.587785f,  0.425325f}, 
{ 0.809017f,  0.309017f,  0.500000f}, { 0.681718f,  0.147621f,  0.716567f},
{ 0.587785f,  0.425325f,  0.688191f}, { 0.955423f,  0.295242f,  0.000000f}, 
{ 1.000000f,  0.000000f,  0.000000f}, { 0.951056f,  0.162460f,  0.262866f},
{ 0.850651f, -0.525731f,  0.000000f}, { 0.955423f, -0.295242f,  0.000000f}, 
{ 0.864188f, -0.442863f,  0.238856f}, { 0.951056f, -0.162460f,  0.262866f}, 
{ 0.809017f, -0.309017f,  0.500000f}, { 0.681718f, -0.147621f,  0.716567f},
{ 0.850651f,  0.000000f,  0.525731f}, { 0.864188f,  0.442863f, -0.238856f},
{ 0.809017f,  0.309017f, -0.500000f}, { 0.951056f,  0.162460f, -0.262866f}, 
{ 0.525731f,  0.000000f, -0.850651f}, { 0.681718f,  0.147621f, -0.716567f}, 
{ 0.681718f, -0.147621f, -0.716567f}, { 0.850651f,  0.000000f, -0.525731f},
{ 0.809017f, -0.309017f, -0.500000f}, { 0.864188f, -0.442863f, -0.238856f}, 
{ 0.951056f, -0.162460f, -0.262866f}, { 0.147621f,  0.716567f, -0.681718f}, 
{ 0.309017f,  0.500000f, -0.809017f}, { 0.425325f,  0.688191f, -0.587785f}, 
{ 0.442863f,  0.238856f, -0.864188f}, { 0.587785f,  0.425325f, -0.688191f}, 
{ 0.688191f,  0.587785f, -0.425325f}, {-0.147621f,  0.716567f, -0.681718f}, 
{-0.309017f,  0.500000f, -0.809017f}, { 0.000000f,  0.525731f, -0.850651f},
{-0.525731f,  0.000000f, -0.850651f}, {-0.442863f,  0.238856f, -0.864188f},
{-0.295242f,  0.000000f, -0.955423f}, {-0.162460f,  0.262866f, -0.951056f}, 
{ 0.000000f,  0.000000f, -1.000000f}, { 0.295242f,  0.000000f, -0.955423f}, 
{ 0.162460f,  0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{ 0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{ 0.147621f, -0.716567f, -0.681718f}, { 0.000000f, -0.525731f, -0.850651f}, 
{ 0.309017f, -0.500000f, -0.809017f}, { 0.442863f, -0.238856f, -0.864188f}, 
{ 0.162460f, -0.262866f, -0.951056f}, { 0.238856f, -0.864188f, -0.442863f}, 
{ 0.500000f, -0.809017f, -0.309017f}, { 0.425325f, -0.688191f, -0.587785f}, 
{ 0.716567f, -0.681718f, -0.147621f}, { 0.688191f, -0.587785f, -0.425325f}, 
{ 0.587785f, -0.425325f, -0.688191f}, { 0.000000f, -0.955423f, -0.295242f},
{ 0.000000f, -1.000000f,  0.000000f}, { 0.262866f, -0.951056f, -0.162460f}, 
{ 0.000000f, -0.850651f,  0.525731f}, { 0.000000f, -0.955423f,  0.295242f}, 
{ 0.238856f, -0.864188f,  0.442863f}, { 0.262866f, -0.951056f,  0.162460f}, 
{ 0.500000f, -0.809017f,  0.309017f}, { 0.716567f, -0.681718f,  0.147621f}, 
{ 0.525731f, -0.850651f,  0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f,  0.000000f}, {-0.716567f, -0.681718f, -0.147621f},
{-0.716567f, -0.681718f,  0.147621f}, {-0.525731f, -0.850651f,  0.000000f}, 
{-0.500000f, -0.809017f,  0.309017f}, {-0.238856f, -0.864188f,  0.442863f},
{-0.262866f, -0.951056f,  0.162460f}, {-0.864188f, -0.442863f,  0.238856f},
{-0.809017f, -0.309017f,  0.500000f}, {-0.688191f, -0.587785f,  0.425325f},
{-0.681718f, -0.147621f,  0.716567f}, {-0.442863f, -0.238856f,  0.864188f},
{-0.587785f, -0.425325f,  0.688191f}, {-0.309017f, -0.500000f,  0.809017f},
{-0.147621f, -0.716567f,  0.681718f}, {-0.425325f, -0.688191f,  0.587785f},
{-0.162460f, -0.262866f,  0.951056f}, { 0.442863f, -0.238856f,  0.864188f},
{ 0.162460f, -0.262866f,  0.951056f}, { 0.309017f, -0.500000f,  0.809017f},
{ 0.147621f, -0.716567f,  0.681718f}, { 0.000000f, -0.525731f,  0.850651f},
{ 0.425325f, -0.688191f,  0.587785f}, { 0.587785f, -0.425325f,  0.688191f},
{ 0.688191f, -0.587785f,  0.425325f}, {-0.955423f,  0.295242f,  0.000000f},
{-0.951056f,  0.162460f,  0.262866f}, {-1.000000f,  0.000000f,  0.000000f},
{-0.850651f,  0.000000f,  0.525731f}, {-0.955423f, -0.295242f,  0.000000f},
{-0.951056f, -0.162460f,  0.262866f}, {-0.864188f,  0.442863f, -0.238856f},
{-0.951056f,  0.162460f, -0.262866f}, {-0.809017f,  0.309017f, -0.500000f},
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f},
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f,  0.147621f, -0.716567f},
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f,  0.000000f, -0.525731f},
{-0.688191f,  0.587785f, -0.425325f}, {-0.587785f,  0.425325f, -0.688191f},
{-0.425325f,  0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f},
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f},
};

byte NormalToByte(const vec3_t n)
{
	int i, best;
	float bestdistance, distance;

	best = 0;
	bestdistance = DotProduct (n, m_bytenormals[0]);
	for (i = 1;i < NUMVERTEXNORMALS;i++)
	{
		distance = DotProduct (n, m_bytenormals[i]);
		if (distance > bestdistance)
		{
			bestdistance = distance;
			best = i;
		}
	}
	return best;
}

// note: uses byte partly to force unsigned for the validity check
void ByteToNormal(byte num, vec3_t n)
{
	if (num < NUMVERTEXNORMALS)
		VectorCopy(m_bytenormals[num], n);
	else
		VectorClear(n); // FIXME: complain?
}

float Q_RSqrt(float number)
{
	float y;

	if (number == 0.0f)
		return 0.0f;

	*((int *)&y) = 0x5f3759df - ((* (int *) &number) >> 1);
	return y * (1.5f - (number * 0.5f * y * y));
}


// assumes "src" is normalized
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	// LordHavoc: optimized to death and beyond
	int pos;
	float minelem;

	if (src[0])
	{
		dst[0] = 0;
		if (src[1])
		{
			dst[1] = 0;
			if (src[2])
			{
				dst[2] = 0;
				pos = 0;
				minelem = fabs(src[0]);
				if (fabs(src[1]) < minelem)
				{
					pos = 1;
					minelem = fabs(src[1]);
				}
				if (fabs(src[2]) < minelem)
					pos = 2;

				dst[pos] = 1;
				dst[0] -= src[pos] * src[0];
				dst[1] -= src[pos] * src[1];
				dst[2] -= src[pos] * src[2];

				// normalize the result
				VectorNormalize(dst);
			}
			else
				dst[2] = 1;
		}
		else
		{
			dst[1] = 1;
			dst[2] = 0;
		}
	}
	else
	{
		dst[0] = 1;
		dst[1] = 0;
		dst[2] = 0;
	}
}


// LordHavoc: like AngleVectors, but taking a forward vector instead of angles, useful!
void VectorVectors(const vec3_t forward, vec3_t right, vec3_t up)
{
	float d;

	right[0] = forward[2];
	right[1] = -forward[0];
	right[2] = forward[1];

	d = DotProduct(forward, right);
	right[0] -= d * forward[0];
	right[1] -= d * forward[1];
	right[2] -= d * forward[2];
	VectorNormalizeFast(right);
	CrossProduct(right, forward, up);
}

void VectorVectorsDouble(const double *forward, double *right, double *up)
{
	double d;

	right[0] = forward[2];
	right[1] = -forward[0];
	right[2] = forward[1];

	d = DotProduct(forward, right);
	right[0] -= d * forward[0];
	right[1] -= d * forward[1];
	right[2] -= d * forward[2];
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float t0, t1;
	float angle, c, s;
	vec3_t vr, vu, vf;

	angle = DEG2RAD(degrees);

	c = cos(angle);
	s = sin(angle);

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	VectorVectors(vf, vr, vu);

	t0 = vr[0] *  c + vu[0] * -s;
	t1 = vr[0] *  s + vu[0] *  c;
	dst[0] = (t0 * vr[0] + t1 * vu[0] + vf[0] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[0] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[0] * vf[2]) * point[2];

	t0 = vr[1] *  c + vu[1] * -s;
	t1 = vr[1] *  s + vu[1] *  c;
	dst[1] = (t0 * vr[0] + t1 * vu[0] + vf[1] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[1] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[1] * vf[2]) * point[2];

	t0 = vr[2] *  c + vu[2] * -s;
	t1 = vr[2] *  s + vu[2] *  c;
	dst[2] = (t0 * vr[0] + t1 * vu[0] + vf[2] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[2] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[2] * vf[2]) * point[2];
}

/*-----------------------------------------------------------------*/
float anglemod(float a)
{
	a = angleModScalar1 * ((int)(a * angleModScalar2) & 65535);	// Tomaz Speed
	return a;
}

// LordHavoc note 1:
// BoxOnPlaneSide did a switch on a 'signbits' value and had optimized
// assembly in an attempt to accelerate it further, very inefficient
// considering that signbits of the frustum planes only changed each
// frame, and the world planes changed only at load time.
// So, to optimize it further I took the obvious route of storing a function
// pointer in the plane struct itself, and shrunk each of the individual
// cases to a single return statement.
// LordHavoc note 2:
// realized axial cases would be a nice speedup for world geometry, although
// never useful for the frustum planes.
int BoxOnPlaneSideX (vec3_t emins, vec3_t emaxs, mplane_t *p) {return p->dist <= emins[0] ? 1 : (p->dist >= emaxs[0] ? 2 : 3);}
int BoxOnPlaneSideY (vec3_t emins, vec3_t emaxs, mplane_t *p) {return p->dist <= emins[1] ? 1 : (p->dist >= emaxs[1] ? 2 : 3);}
int BoxOnPlaneSideZ (vec3_t emins, vec3_t emaxs, mplane_t *p) {return p->dist <= emins[2] ? 1 : (p->dist >= emaxs[2] ? 2 : 3);}
int BoxOnPlaneSide0 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2]) >= p->dist) | (((p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2]) < p->dist) << 1));}
int BoxOnPlaneSide1 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2]) >= p->dist) | (((p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2]) < p->dist) << 1));}
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2]) >= p->dist) | (((p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2]) < p->dist) << 1));}
int BoxOnPlaneSide3 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2]) >= p->dist) | (((p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2]) < p->dist) << 1));}
int BoxOnPlaneSide4 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2]) >= p->dist) | (((p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2]) < p->dist) << 1));}
int BoxOnPlaneSide5 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2]) >= p->dist) | (((p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2]) < p->dist) << 1));}
int BoxOnPlaneSide6 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2]) >= p->dist) | (((p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2]) < p->dist) << 1));}
int BoxOnPlaneSide7 (vec3_t emins, vec3_t emaxs, mplane_t *p) {return (((p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2]) >= p->dist) | (((p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2]) < p->dist) << 1));}

void BoxOnPlaneSideClassify(mplane_t *p)
{
	switch(p->type)
	{
	case 0: // x axis
		p->BoxOnPlaneSideFunc = BoxOnPlaneSideX;
		break;
	case 1: // y axis
		p->BoxOnPlaneSideFunc = BoxOnPlaneSideY;
		break;
	case 2: // z axis
		p->BoxOnPlaneSideFunc = BoxOnPlaneSideZ;
		break;
	default:
		if (p->normal[2] < 0) // 4
		{
			if (p->normal[1] < 0) // 2
			{
				if (p->normal[0] < 0) // 1
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide7;
				else
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide6;
			}
			else
			{
				if (p->normal[0] < 0) // 1
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide5;
				else
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide4;
			}
		}
		else
		{
			if (p->normal[1] < 0) // 2
			{
				if (p->normal[0] < 0) // 1
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide3;
				else
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide2;
			}
			else
			{
				if (p->normal[0] < 0) // 1
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide1;
				else
					p->BoxOnPlaneSideFunc = BoxOnPlaneSide0;
			}
		}
		break;
	}
}

void PlaneClassify(mplane_t *p)
{
	if (p->normal[0] == 1)
		p->type = 0;
	else if (p->normal[1] == 1)
		p->type = 1;
	else if (p->normal[2] == 1)
		p->type = 2;
	else
		p->type = 3;
	BoxOnPlaneSideClassify(p);
}

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	double angle, sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right || up)
	{
		angle = angles[ROLL] * (M_PI*2 / 360);
		sr = sin(angle);
		cr = cos(angle);
		if (right)
		{
			right[0] = -1*(sr*sp*cy+cr*-sy);
			right[1] = -1*(sr*sp*sy+cr*cy);
			right[2] = -1*(sr*cp);
		}
		if (up)
		{
			up[0] = (cr*sp*cy+-sr*-sy);
			up[1] = (cr*sp*sy+-sr*cy);
			up[2] = cr*cp;
		}
	}
}

void AngleVectorsFLU (const vec3_t angles, vec3_t forward, vec3_t left, vec3_t up)
{
	double angle, sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (left || up)
	{
		angle = angles[ROLL] * (M_PI*2 / 360);
		sr = sin(angle);
		cr = cos(angle);
		if (left)
		{
			left[0] = sr*sp*cy+cr*-sy;
			left[1] = sr*sp*sy+cr*cy;
			left[2] = sr*cp;
		}
		if (up)
		{
			up[0] = cr*sp*cy+-sr*-sy;
			up[1] = cr*sp*sy+-sr*cy;
			up[2] = cr*cp;
		}
	}
}

void AngleMatrix (const vec3_t angles, const vec3_t translate, vec_t matrix[][4])
{
	double angle, sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
	matrix[0][0] = cp*cy;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[0][2] = cr*sp*cy+-sr*-sy;
	matrix[0][3] = translate[0];
	matrix[1][0] = cp*sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = cr*sp*sy+-sr*cy;
	matrix[1][3] = translate[1];
	matrix[2][0] = -sp;
	matrix[2][1] = sr*cp;
	matrix[2][2] = cr*cp;
	matrix[2][3] = translate[2];
}


// LordHavoc: renamed this to Length, and made the normal one a #define
float VectorNormalizeLength (vec3_t v)
{
	float length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations (const float in1[3*3], const float in2[3*3], float out[3*3])
{
	out[0*3+0] = in1[0*3+0] * in2[0*3+0] + in1[0*3+1] * in2[1*3+0] + in1[0*3+2] * in2[2*3+0];
	out[0*3+1] = in1[0*3+0] * in2[0*3+1] + in1[0*3+1] * in2[1*3+1] + in1[0*3+2] * in2[2*3+1];
	out[0*3+2] = in1[0*3+0] * in2[0*3+2] + in1[0*3+1] * in2[1*3+2] + in1[0*3+2] * in2[2*3+2];
	out[1*3+0] = in1[1*3+0] * in2[0*3+0] + in1[1*3+1] * in2[1*3+0] + in1[1*3+2] * in2[2*3+0];
	out[1*3+1] = in1[1*3+0] * in2[0*3+1] + in1[1*3+1] * in2[1*3+1] + in1[1*3+2] * in2[2*3+1];
	out[1*3+2] = in1[1*3+0] * in2[0*3+2] + in1[1*3+1] * in2[1*3+2] + in1[1*3+2] * in2[2*3+2];
	out[2*3+0] = in1[2*3+0] * in2[0*3+0] + in1[2*3+1] * in2[1*3+0] + in1[2*3+2] * in2[2*3+0];
	out[2*3+1] = in1[2*3+0] * in2[0*3+1] + in1[2*3+1] * in2[1*3+1] + in1[2*3+2] * in2[2*3+1];
	out[2*3+2] = in1[2*3+0] * in2[0*3+2] + in1[2*3+1] * in2[1*3+2] + in1[2*3+2] * in2[2*3+2];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms (const float in1[3*4], const float in2[3*4], float out[3*4])
{
	out[0*4+0] = in1[0*4+0] * in2[0*4+0] + in1[0*4+1] * in2[1*4+0] + in1[0*4+2] * in2[2*4+0];
	out[0*4+1] = in1[0*4+0] * in2[0*4+1] + in1[0*4+1] * in2[1*4+1] + in1[0*4+2] * in2[2*4+1];
	out[0*4+2] = in1[0*4+0] * in2[0*4+2] + in1[0*4+1] * in2[1*4+2] + in1[0*4+2] * in2[2*4+2];
	out[0*4+3] = in1[0*4+0] * in2[0*4+3] + in1[0*4+1] * in2[1*4+3] + in1[0*4+2] * in2[2*4+3] + in1[0*4+3];
	out[1*4+0] = in1[1*4+0] * in2[0*4+0] + in1[1*4+1] * in2[1*4+0] + in1[1*4+2] * in2[2*4+0];
	out[1*4+1] = in1[1*4+0] * in2[0*4+1] + in1[1*4+1] * in2[1*4+1] + in1[1*4+2] * in2[2*4+1];
	out[1*4+2] = in1[1*4+0] * in2[0*4+2] + in1[1*4+1] * in2[1*4+2] + in1[1*4+2] * in2[2*4+2];
	out[1*4+3] = in1[1*4+0] * in2[0*4+3] + in1[1*4+1] * in2[1*4+3] + in1[1*4+2] * in2[2*4+3] + in1[1*4+3];
	out[2*4+0] = in1[2*4+0] * in2[0*4+0] + in1[2*4+1] * in2[1*4+0] + in1[2*4+2] * in2[2*4+0];
	out[2*4+1] = in1[2*4+0] * in2[0*4+1] + in1[2*4+1] * in2[1*4+1] + in1[2*4+2] * in2[2*4+1];
	out[2*4+2] = in1[2*4+0] * in2[0*4+2] + in1[2*4+1] * in2[1*4+2] + in1[2*4+2] * in2[2*4+2];
	out[2*4+3] = in1[2*4+0] * in2[0*4+3] + in1[2*4+1] * in2[1*4+3] + in1[2*4+2] * in2[2*4+3] + in1[2*4+3];
}


void Mathlib_Init(void)
{
	int a;

	// LordHavoc: setup 1.0f / N table for quick recipricols of integers
	ixtable[0] = 0;
	for (a = 1;a < 4096;a++)
		ixtable[a] = 1.0f / a;
}

