#version 330 core

struct PointLight
{
	vec3 position;
	vec3 ambient;
	vec3 specular;
	vec3 diffuse;

	float constant;
	float linear;
	float quad;
};

struct Material
{
	vec3 ambient;
	vec3 specular;
	float shininess;
	sampler2D diffuseMap;
};

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
out vec4 frag_color;

uniform PointLight pointLight;
uniform Material material;
uniform vec3 viewPos;

vec3 calcPointLight(vec3 normal);

void main()
{
	vec3 normal = normalize(Normal);
	vec3 result = calcPointLight(normal);
	frag_color = vec4(result, 1.0f);
}

vec3 calcPointLight(vec3 normal)
{
	vec3 effect;

	// Ambient
	vec3 ambient = pointLight.ambient * material.ambient * vec3(texture(material.diffuseMap, TexCoord));

	// Diffuse
	float lightDist = length(pointLight.position - FragPos);
	float atten = 1.0 / (pointLight.constant + pointLight.linear * lightDist + pointLight.quad * lightDist * lightDist);
	vec3 lightDir = normalize(pointLight.position - FragPos);
	float NdotL = max(dot(normal, lightDir), 0.0f);
	vec3 diffuse = NdotL * vec3(texture(material.diffuseMap, TexCoord)) * pointLight.diffuse * atten;

	// Specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 halfVector = normalize(lightDir + viewDir);
	float HdotN = max(dot(normal, halfVector), 0.0f);
	vec3 specular = pointLight.specular * material.specular * pow(HdotN, material.shininess) * atten;

	effect += ambient + diffuse + specular;
	return effect;
}
