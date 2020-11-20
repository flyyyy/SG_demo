#version 330 core
layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gDiffuse;
layout (location = 3) out vec3 gSpecular;
layout (location = 4) out float gMask;
layout (location = 5) out vec3 gMaterial;

in vec3 Pos;
in vec3 Normal;
in vec2 TexCoords;
in float Mask;

uniform float metallic;
uniform float roughness;

uniform vec3 kD;
uniform vec3 kS;

uniform bool hasTexture;

uniform sampler2D diffuseTexture0;
uniform sampler2D specularTexture0;

void main()
{
    gPos = Pos;
    gNormal = normalize(Normal);
    gMask = Mask;
    gMaterial = vec3(metallic, roughness, 0.0);
    if (hasTexture)
    {
        gDiffuse = kD * texture(diffuseTexture0, TexCoords).rgb;
        gSpecular = kS * texture(specularTexture0, TexCoords).rgb;
    }
    else
    {
        gDiffuse = kD;
        gSpecular = kS;
    }
}
