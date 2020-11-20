#version 330 core
out vec2 FragColor;
in vec2 TexCoords;

const float pi = 3.14159265;

float radicalInverseVdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), radicalInverseVdc(i));
}

vec3 sampleGGX(vec2 xi, vec3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * pi * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    vec3 vec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(vec);
}

float GeoGGX(float NdotV, float roughness)
{
    float k = (roughness * roughness) / 2.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeoSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeoGGX(NdotL, roughness) * GeoGGX(NdotV, roughness);
}

vec2 integrateBRDF(float NdotV, float roughness)
{
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    float A = 0.0, B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    for (uint i = 0u; i < 1024u; ++i)
    {
        vec2 xi = hammersley(i, 1024u);
        vec3 H = sampleGGX(xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (L.z > 0.0)
        {
            float G = GeoSmith(N, V, L, roughness);
            float GVis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * GVis;
            B += Fc * GVis;
        }
    }
    return vec2(A / 1024.0, B / 1024.0);
}

void main()
{
    FragColor = integrateBRDF(TexCoords.x, TexCoords.y);
}
