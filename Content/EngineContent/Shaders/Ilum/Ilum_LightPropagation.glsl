#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 2) in vec2 a_TexCoord;
out vec2 UV;

void main(void)
{
	gl_Position = vec4(a_Position, 1.0);
	UV = a_TexCoord;
}

#type fragment
#version 430 core

in vec2 UV;
uniform sampler2D u_Accumulator;
uniform sampler2D u_SurfelDirectLight;
uniform sampler2D u_SurfelWorldPos;
uniform sampler2D u_SurfelWorldNormal;
uniform int u_Seed;
uniform vec3 u_LightGridPos;
uniform vec3 u_LightGridStep;

out vec3 out_Irradiance;

float rand(vec2 co)
{
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}
vec2 Remap(vec2 value, vec2 in1, vec2 in2, vec2 out1, vec2 out2)
{
	return out1 + (value - in1) * (out2 - out1) / (in2 - in1);
}
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 direction)
{
    vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float DistFunc(float dist)
{
	return pow(2.718281828, dist * -0.3);
}
float DistUV(vec2 eqUV, vec3 N)
{
	vec2 uv = SampleSphericalMap(N);
	float dist = distance(uv, eqUV);
	float dist2 = distance(vec2(uv.x - 1.0, uv.y), eqUV); if (dist2 < dist) dist = dist2;
	float dist3 = distance(vec2(uv.x + 1.0, uv.y), eqUV); if (dist3 < dist) dist = dist3;
	float dist4 = distance(vec2(uv.x, uv.y - 1.0), eqUV); if (dist4 < dist) dist = dist4;
	float dist5 = distance(vec2(uv.x, uv.y + 1.0), eqUV); if (dist5 < dist) dist = dist5;
	
	return pow(2.718281828, dist * -5.0);
}
vec3 SampleOnce(vec3 probePos, int sampleCount, vec2 equirectangularUV)
{
	vec3 surfelRadiance;
	vec3 surfelPos = vec3(0.0);
	float surfelDistance;

	int J = 0;
	while (surfelPos == vec3(0.0))
	{
		vec2 _uv = vec2(rand(vec2(u_Seed + J++) - probePos.xy), rand(vec2(u_Seed - J) - 2.5 * probePos.xz));

		surfelRadiance = texture(u_SurfelDirectLight, _uv).rgb;
		surfelPos = texture(u_SurfelWorldPos, _uv).xyz;
		surfelDistance = distance(probePos, surfelPos);

		if (J >= sampleCount) return vec3(0.0);
	}

	for (int It = 0; It < sampleCount - J; It++)
	{
		vec2 _uv = vec2(rand(vec2(u_Seed - It) + probePos.yz), rand(vec2(u_Seed + 2 * It) + probePos.xz));
		vec3 newSurfelPos = texture(u_SurfelWorldPos, _uv).xyz;

		if (dot(normalize(surfelPos - probePos), normalize(newSurfelPos - probePos)) >= -1.0 && newSurfelPos != vec3(0.0))
		{
			float newSurfelDistance = distance(probePos, newSurfelPos);
			if (newSurfelDistance < surfelDistance)
			{
				surfelRadiance = texture(u_SurfelDirectLight, _uv).rgb;
				surfelPos = newSurfelPos;
				surfelDistance = newSurfelDistance;
			}
		}
	}

	return surfelRadiance * DistFunc(surfelDistance) * DistUV(equirectangularUV, normalize(surfelPos - probePos));
}

void main(void)
{
	vec3 irradiance = vec3(0.0);

	ivec2 coords = ivec2(vec2(UV * 2048.0));
	coords /= 32;
	int probeID = coords.x + coords.y * 64;
	ivec3 probeGridPos = ivec3 (probeID / 16 / 16,
								probeID / 16 % 16,
								probeID % 16 % 16);


	vec3 probePos = (u_LightGridPos * u_LightGridStep) - (7.5 * u_LightGridStep) + (vec3(probeGridPos.x, probeGridPos.y, probeGridPos.z) * u_LightGridStep);
	vec2 eqUV = Remap(UV, vec2(0.015625 * float(coords.x), 0.015625 * float(coords.y)), vec2(0.015625 * float(coords.x + 1), 0.015625 * float(coords.y + 1)), vec2(0.0), vec2(1.0));

	irradiance += SampleOnce(probePos, 64, eqUV);

	out_Irradiance = mix(texture(u_Accumulator, UV).rgb, irradiance, 0.025);
}