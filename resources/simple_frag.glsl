#version 330 core 
in vec3 fragNor;
in vec3 WPos;
//to send the color to a frame buffer
layout(location = 0) out vec4 color;

uniform vec3 LPos;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float shine;

/* Very simple Diffuse shader */
void main()
{
	vec3 Dcolor, Scolor;

	/* Diffuse */
    /*vec3 Dlight = vec3(1, 1, 1);*/
    vec3 Dlight = LPos - WPos;
	vec3 normal = normalize(fragNor);

	/* Specular */
	vec3 V = normalize(-WPos);
	vec3 L = normalize(Dlight);
	vec3 H = normalize(L + V);
	float specAngle = max(dot(H, normal), 0.0);

	Dcolor = MatDif * max(dot(normalize(L), normal), 0) + MatAmb;
	Scolor = MatSpec * pow(specAngle, shine);

	color = vec4(Dcolor + Scolor, 1.0);
}
