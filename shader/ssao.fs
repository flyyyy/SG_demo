#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPos;
uniform sampler2D gNormal;
uniform sampler2D gMask;
uniform sampler2D texNoise;

uniform vec3 samples[64];

uniform vec2 noiseScale;

uniform mat4 view;
uniform mat4 projection;

const float radius = 0.3;
const float bias = 0.025;

void main()
{
    // get pos & normal, transform them to view space
    vec3 pos = texture(gPos, TexCoords).xyz;
    pos = (view * vec4(pos, 1.0)).xyz;
    vec3 normal = texture(gNormal, TexCoords).xyz;
    normal = transpose(inverse(mat3(view))) * normal;
    if (normal == vec3(0.0))
    {
        FragColor = 1.0;
        return;
    }
    // get TBN matrix
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // calc ao
    float occ = 0.0;
    for (uint i = 0u; i < 64u; i++)
    {
        vec3 sample = TBN * samples[i];
        sample = sample * radius + pos;
        vec4 offset = vec4(sample, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        float sampleDepth = (view * vec4(texture(gPos, offset.xy).xyz, 1.0)).z;
        float range = smoothstep(0.0, 1.0, radius / abs(pos.z - sampleDepth));
        if (texture(gMask, offset.xy).r != 0.0)
            range = 0.0;
        occ += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * range;
    }
    occ = 1.0 - occ / 64.0;
    FragColor = occ;
}
