// LordHavoc: transform code for purposes of transpoly, wallpoly, etc

#include "quakedef.h"

vec3_t softwaretransform_x;
vec3_t softwaretransform_y;
vec3_t softwaretransform_z;
vec_t softwaretransform_scale;
vec3_t softwaretransform_offset;

// set to different transform code depending on complexity of transform
void (*softwaretransform) (vec3_t in, vec3_t out);

// the real deal
void softwaretransform_dorotatescaletranslate (vec3_t in, vec3_t out)
{
	out[0] = (in[0] * softwaretransform_x[0] + in[1] * softwaretransform_y[0] + in[2] * softwaretransform_z[0]) * softwaretransform_scale + softwaretransform_offset[0];
	out[1] = (in[0] * softwaretransform_x[1] + in[1] * softwaretransform_y[1] + in[2] * softwaretransform_z[1]) * softwaretransform_scale + softwaretransform_offset[1];
	out[2] = (in[0] * softwaretransform_x[2] + in[1] * softwaretransform_y[2] + in[2] * softwaretransform_z[2]) * softwaretransform_scale + softwaretransform_offset[2];
}

void softwaretransform_doscaletranslate (vec3_t in, vec3_t out)
{
	out[0] = in[0] * softwaretransform_scale + softwaretransform_offset[0];
	out[1] = in[1] * softwaretransform_scale + softwaretransform_offset[1];
	out[2] = in[2] * softwaretransform_scale + softwaretransform_offset[2];
}

void softwaretransform_dorotatetranslate (vec3_t in, vec3_t out)
{
	out[0] = (in[0] * softwaretransform_x[0] + in[1] * softwaretransform_y[0] + in[2] * softwaretransform_z[0]) + softwaretransform_offset[0];
	out[1] = (in[0] * softwaretransform_x[1] + in[1] * softwaretransform_y[1] + in[2] * softwaretransform_z[1]) + softwaretransform_offset[1];
	out[2] = (in[0] * softwaretransform_x[2] + in[1] * softwaretransform_y[2] + in[2] * softwaretransform_z[2]) + softwaretransform_offset[2];
}

void softwaretransform_dotranslate (vec3_t in, vec3_t out)
{
	out[0] = in[0] + softwaretransform_offset[0];
	out[1] = in[1] + softwaretransform_offset[1];
	out[2] = in[2] + softwaretransform_offset[2];
}

void softwaretransform_dorotatescale (vec3_t in, vec3_t out)
{
	out[0] = (in[0] * softwaretransform_x[0] + in[1] * softwaretransform_y[0] + in[2] * softwaretransform_z[0]) * softwaretransform_scale;
	out[1] = (in[0] * softwaretransform_x[1] + in[1] * softwaretransform_y[1] + in[2] * softwaretransform_z[1]) * softwaretransform_scale;
	out[2] = (in[0] * softwaretransform_x[2] + in[1] * softwaretransform_y[2] + in[2] * softwaretransform_z[2]) * softwaretransform_scale;
}

void softwaretransform_doscale (vec3_t in, vec3_t out)
{
	out[0] = in[0] * softwaretransform_scale + softwaretransform_offset[0];
	out[1] = in[1] * softwaretransform_scale + softwaretransform_offset[1];
	out[2] = in[2] * softwaretransform_scale + softwaretransform_offset[2];
}

void softwaretransform_dorotate (vec3_t in, vec3_t out)
{
	out[0] = (in[0] * softwaretransform_x[0] + in[1] * softwaretransform_y[0] + in[2] * softwaretransform_z[0]);
	out[1] = (in[0] * softwaretransform_x[1] + in[1] * softwaretransform_y[1] + in[2] * softwaretransform_z[1]);
	out[2] = (in[0] * softwaretransform_x[2] + in[1] * softwaretransform_y[2] + in[2] * softwaretransform_z[2]);
}

void softwaretransform_docopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void softwareuntransform (vec3_t in, vec3_t out)
{
	vec3_t v;
	float s = 1.0f / softwaretransform_scale;
	v[0] = in[0] - softwaretransform_offset[0];
	v[1] = in[1] - softwaretransform_offset[1];
	v[2] = in[2] - softwaretransform_offset[2];
	out[0] = (v[0] * softwaretransform_x[0] + v[1] * softwaretransform_x[1] + v[2] * softwaretransform_x[2]) * s;
	out[1] = (v[0] * softwaretransform_y[0] + v[1] * softwaretransform_y[1] + v[2] * softwaretransform_y[2]) * s;
	out[2] = (v[0] * softwaretransform_z[0] + v[1] * softwaretransform_z[1] + v[2] * softwaretransform_z[2]) * s;
}

// to save time on transforms, choose the appropriate function
void softwaretransform_classify(void)
{
	if (softwaretransform_offset[0] != 0 || softwaretransform_offset[1] != 0 || softwaretransform_offset[2] != 0)
	{
		if (softwaretransform_scale != 1)
		{
			if (softwaretransform_x[0] != 1 || softwaretransform_x[1] != 0 || softwaretransform_x[2] != 0 ||
				softwaretransform_y[0] != 0 || softwaretransform_y[1] != 1 || softwaretransform_y[2] != 0 ||
				softwaretransform_z[0] != 0 || softwaretransform_z[1] != 0 || softwaretransform_z[2] != 1)
				softwaretransform = &softwaretransform_dorotatescaletranslate;
			else
				softwaretransform = &softwaretransform_doscaletranslate;
		}
		else
		{
			if (softwaretransform_x[0] != 1 || softwaretransform_x[1] != 0 || softwaretransform_x[2] != 0 ||
				softwaretransform_y[0] != 0 || softwaretransform_y[1] != 1 || softwaretransform_y[2] != 0 ||
				softwaretransform_z[0] != 0 || softwaretransform_z[1] != 0 || softwaretransform_z[2] != 1)
				softwaretransform = &softwaretransform_dorotatetranslate;
			else
				softwaretransform = &softwaretransform_dotranslate;
		}
	}
	else
	{
		if (softwaretransform_scale != 1)
		{
			if (softwaretransform_x[0] != 1 || softwaretransform_x[1] != 0 || softwaretransform_x[2] != 0 ||
				softwaretransform_y[0] != 0 || softwaretransform_y[1] != 1 || softwaretransform_y[2] != 0 ||
				softwaretransform_z[0] != 0 || softwaretransform_z[1] != 0 || softwaretransform_z[2] != 1)
				softwaretransform = &softwaretransform_dorotatescale;
			else
				softwaretransform = &softwaretransform_doscale;
		}
		else
		{
			if (softwaretransform_x[0] != 1 || softwaretransform_x[1] != 0 || softwaretransform_x[2] != 0 ||
				softwaretransform_y[0] != 0 || softwaretransform_y[1] != 1 || softwaretransform_y[2] != 0 ||
				softwaretransform_z[0] != 0 || softwaretransform_z[1] != 0 || softwaretransform_z[2] != 1)
				softwaretransform = &softwaretransform_dorotate;
			else
				softwaretransform = &softwaretransform_docopy;
		}
	}
}

void softwaretransformidentity(void)
{
	softwaretransform_offset[0] = softwaretransform_offset[1] = softwaretransform_offset[2] = softwaretransform_x[1] = softwaretransform_x[2] = softwaretransform_y[0] = softwaretransform_y[2] = softwaretransform_z[0] = softwaretransform_z[1] = 0;
	softwaretransform_x[0] = softwaretransform_y[1] = softwaretransform_z[2] = 1;
	softwaretransform_scale = 1;
	// we know what it is
	softwaretransform = &softwaretransform_docopy;
}

void softwaretransformset (vec3_t origin, vec3_t angles, vec_t scale)
{
	VectorCopy(origin, softwaretransform_offset);
	AngleVectors(angles, softwaretransform_x, softwaretransform_y, softwaretransform_z);
	softwaretransform_y[0] = -softwaretransform_y[0];
	softwaretransform_y[1] = -softwaretransform_y[1];
	softwaretransform_y[2] = -softwaretransform_y[2];
	softwaretransform_scale = scale;
	// choose best transform code
	softwaretransform_classify();
}

void softwaretransformforentity (entity_render_t *r)
{
	vec3_t angles;
	angles[0] = -r->angles[0];
	angles[1] = r->angles[1];
	angles[2] = r->angles[2];
	softwaretransformset(r->origin, angles, r->scale);
}

// brush entities are not backwards like models and sprites are
void softwaretransformforbrushentity (entity_render_t *r)
{
	softwaretransformset(r->origin, r->angles, r->scale);
}
