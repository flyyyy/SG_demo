#version 330 core
out vec4 FragColor;

in vec3 Pos;

uniform samplerCube envmap;
uniform float roughness;

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

void main()
{
    vec3 N = normalize(Pos);
    vec3 R = N;
    vec3 V = R;
    float totWeight = 0.0;
    vec3 color = vec3(0.0);
    for (uint i = 0u; i < 1024u; ++i)
    {
        vec2 xi = hammersley(i, 1024u);
        vec3 H = sampleGGX(xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = dot(N, L);
        if (NdotL > 0)
        {
            color += texture(envmap, L).rgb * NdotL;
            totWeight += NdotL;
        }
    }
    color /= totWeight;
    FragColor = vec4(color, 1.0);
}
