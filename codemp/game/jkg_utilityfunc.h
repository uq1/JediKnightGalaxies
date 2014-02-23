// Jedi Knight Galaxies Utility functions

int JKG_ModifyCredits(playerState_t&, int);
void JKG_RotateBBox(vec3_t mins,vec3_t maxs, vec3_t angles);
void JKG_GetAutoBoxForModel(const char *model, vec3_t angles, float scale, vec3_t mins, vec3_t maxs);