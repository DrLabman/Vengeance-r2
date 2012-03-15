int		R_LightPoint (vec3_t p);
//int		*R_LightPoint (vec3_t p);
void	GL_SubdivideSurface (msurface_t *fa);
void	GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);
void	R_DrawBrushModel (entity_t *e);
void	R_DrawBrush (entity_t *e, float *modeorg); //used by r_drawburshmodel and sky draing
void	RotatePointAroundVector( vec3_t dst, vec3_t dir, const vec3_t point, float degrees );
void	R_AnimateLight (void);
void	V_CalcBlend (void);
void	R_DrawWorld (void);
void	R_RenderDlights (void);
void	R_DrawParticles (void);
void	R_DrawAliasModel (entity_t *e);
void	R_DrawSpriteModel (entity_t *e);
void	R_InitParticles (void);
void	R_ClearParticles (void);
void LoadParticleScript(char *script);//particle script
void LoadParticleScript_f(void);	//particle script
void	R_ClearBeams (void);
void	GL_BuildLightmaps (void);
void	EmitWaterPolys (msurface_t *fa);
void	EmitSkyPolys (msurface_t *fa);
void	EmitBothSkyLayers (msurface_t *fa);
void	R_DrawWaterChain (msurface_t *s);
void	R_DrawSkyChain (msurface_t *s);

qboolean	R_CullBox (vec3_t mins, vec3_t maxs);
qboolean	SV_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace);

void	R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void	R_RotateForEntity (entity_t *e);
void	R_BlendedRotateForEntity (entity_t *e);
void	R_BlendedRotateRotateForEntity (entity_t *e);
void	R_StoreEfrags (efrag_t **ppefrag);
void	GL_Set2D (void);
LONG	CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	IN_Accumulate (void);
