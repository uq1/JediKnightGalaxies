uniform sampler2D u_DiffuseMap;

varying vec2 var_TexCoords;

void main(void)
{
	gl_FragColor = texture(u_DiffuseMap, var_TexCoords);
}
