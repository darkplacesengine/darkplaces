
#include "quakedef.h"

static int R_SpriteSetup (const entity_render_t *ent, int type, float org[3], float left[3], float up[3])
{
	float scale;

	// nudge it toward the view to make sure it isn't in a wall
	org[0] = ent->matrix.m[0][3] - r_viewforward[0];
	org[1] = ent->matrix.m[1][3] - r_viewforward[1];
	org[2] = ent->matrix.m[2][3] - r_viewforward[2];
	switch(type)
	{
	case SPR_VP_PARALLEL_UPRIGHT:
		// flames and such
		// vertical beam sprite, faces view plane
		scale = ent->scale / sqrt(r_viewforward[0]*r_viewforward[0]+r_viewforward[1]*r_viewforward[1]);
		left[0] = -r_viewforward[1] * scale;
		left[1] = r_viewforward[0] * scale;
		left[2] = 0;
		up[0] = 0;
		up[1] = 0;
		up[2] = ent->scale;
		break;
	case SPR_FACING_UPRIGHT:
		// flames and such
		// vertical beam sprite, faces viewer's origin (not the view plane)
		scale = ent->scale / sqrt((org[0] - r_vieworigin[0])*(org[0] - r_vieworigin[0])+(org[1] - r_vieworigin[1])*(org[1] - r_vieworigin[1]));
		left[0] = (org[1] - r_vieworigin[1]) * scale;
		left[1] = -(org[0] - r_vieworigin[0]) * scale;
		left[2] = 0;
		up[0] = 0;
		up[1] = 0;
		up[2] = ent->scale;
		break;
	default:
		Con_Printf("R_SpriteSetup: unknown sprite type %i\n", type);
		// fall through to normal sprite
	case SPR_VP_PARALLEL:
		// normal sprite
		// faces view plane
		left[0] = r_viewleft[0] * ent->scale;
		left[1] = r_viewleft[1] * ent->scale;
		left[2] = r_viewleft[2] * ent->scale;
		up[0] = r_viewup[0] * ent->scale;
		up[1] = r_viewup[1] * ent->scale;
		up[2] = r_viewup[2] * ent->scale;
		break;
	case SPR_ORIENTED:
		// bullet marks on walls
		// ignores viewer entirely
		left[0] = ent->matrix.m[0][1];
		left[1] = ent->matrix.m[1][1];
		left[2] = ent->matrix.m[2][1];
		up[0] = ent->matrix.m[0][2];
		up[1] = ent->matrix.m[1][2];
		up[2] = ent->matrix.m[2][2];
		break;
	case SPR_VP_PARALLEL_ORIENTED:
		// I have no idea what people would use this for...
		// oriented relative to view space
		// FIXME: test this and make sure it mimicks software
		left[0] = ent->matrix.m[0][1] * r_viewforward[0] + ent->matrix.m[1][1] * r_viewleft[0] + ent->matrix.m[2][1] * r_viewup[0];
		left[1] = ent->matrix.m[0][1] * r_viewforward[1] + ent->matrix.m[1][1] * r_viewleft[1] + ent->matrix.m[2][1] * r_viewup[1];
		left[2] = ent->matrix.m[0][1] * r_viewforward[2] + ent->matrix.m[1][1] * r_viewleft[2] + ent->matrix.m[2][1] * r_viewup[2];
		up[0] = ent->matrix.m[0][2] * r_viewforward[0] + ent->matrix.m[1][2] * r_viewleft[0] + ent->matrix.m[2][2] * r_viewup[0];
		up[1] = ent->matrix.m[0][2] * r_viewforward[1] + ent->matrix.m[1][2] * r_viewleft[1] + ent->matrix.m[2][2] * r_viewup[1];
		up[2] = ent->matrix.m[0][2] * r_viewforward[2] + ent->matrix.m[1][2] * r_viewleft[2] + ent->matrix.m[2][2] * r_viewup[2];
		break;
	}
	return false;
}

static void R_DrawSpriteImage (int additive, mspriteframe_t *frame, rtexture_t *texture, vec3_t origin, vec3_t up, vec3_t left, float red, float green, float blue, float alpha)
{
	// FIXME: negate left and right in loader
	R_DrawSprite(GL_SRC_ALPHA, additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA, texture, false, origin, left, up, frame->left, frame->right, frame->down, frame->up, red, green, blue, alpha);
}

void R_DrawSpriteModelCallback(const void *calldata1, int calldata2)
{
	const entity_render_t *ent = calldata1;
	int i;
	vec3_t left, up, org, color, diffusecolor, diffusenormal;
	mspriteframe_t *frame;
	vec3_t diff;
	float fog, ifog;

	if (R_SpriteSetup(ent, ent->model->sprite.sprnum_type, org, left, up))
		return;

	R_Mesh_Matrix(&r_identitymatrix);

	if ((ent->model->flags & EF_FULLBRIGHT) || (ent->effects & EF_FULLBRIGHT))
		color[0] = color[1] = color[2] = 1;
	else
	{
		R_CompleteLightPoint(color, diffusecolor, diffusenormal, ent->origin, true, NULL);
		VectorMA(color, 0.5f, diffusecolor, color);
	}

	if (fogenabled)
	{
		VectorSubtract(ent->origin, r_vieworigin, diff);
		fog = exp(fogdensity/DotProduct(diff,diff));
		if (fog > 1)
			fog = 1;
	}
	else
		fog = 0;
	ifog = 1 - fog;

	if (r_lerpsprites.integer)
	{
		// LordHavoc: interpolated sprite rendering
		for (i = 0;i < 4;i++)
		{
			if (ent->frameblend[i].lerp >= 0.01f)
			{
				frame = ent->model->sprite.sprdata_frames + ent->frameblend[i].frame;
				R_DrawSpriteImage((ent->effects & EF_ADDITIVE) || (ent->model->flags & EF_ADDITIVE), frame, frame->texture, org, up, left, color[0] * ifog, color[1] * ifog, color[2] * ifog, ent->alpha * ent->frameblend[i].lerp);
				if (fog * ent->frameblend[i].lerp >= 0.01f)
					R_DrawSpriteImage(true, frame, frame->fogtexture, org, up, left, fogcolor[0],fogcolor[1],fogcolor[2], fog * ent->alpha * ent->frameblend[i].lerp);
			}
		}
	}
	else
	{
		// LordHavoc: no interpolation
		frame = NULL;
		for (i = 0;i < 4 && ent->frameblend[i].lerp;i++)
			frame = ent->model->sprite.sprdata_frames + ent->frameblend[i].frame;

		R_DrawSpriteImage((ent->effects & EF_ADDITIVE) || (ent->model->flags & EF_ADDITIVE), frame, frame->texture, org, up, left, color[0] * ifog, color[1] * ifog, color[2] * ifog, ent->alpha);
		if (fog * ent->frameblend[i].lerp >= 0.01f)
			R_DrawSpriteImage(true, frame, frame->fogtexture, org, up, left, fogcolor[0],fogcolor[1],fogcolor[2], fog * ent->alpha);
	}
}

void R_Model_Sprite_Draw(entity_render_t *ent)
{
	if (ent->frameblend[0].frame < 0)
		return;

	c_sprites++;

	R_MeshQueue_AddTransparent(ent->origin, R_DrawSpriteModelCallback, ent, 0);
}

