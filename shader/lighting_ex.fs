#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float scale;
uniform float surfaceY;
uniform float lightWeight[16];

uniform bool dispSurface;

uniform vec3 eye;
uniform vec3 lightDir[4];

uniform mat4 lightMat[4];
uniform mat4 projView;

uniform sampler2D ssaoMap;
uniform sampler2D gPos;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecular;
uniform sampler2DArray shadowMap;
uniform sampler2D occMap;
uniform sampler2D background;
uniform sampler2D gMask;
uniform sampler2D gMaterial;

const float pi = 3.14159265;

struct SG
{
    vec3 weight;
    vec3 u;
    float alpha;
};

uniform SG SG_light[3];

vec3 evalSG(SG sg, vec3 dir)
{
    float cosDis = dot(dir, sg.u);
    return sg.weight * exp(sg.alpha * (cosDis - 1.0));
}

vec3 innerProductSG(SG x, SG y)
{
    float len = length(x.alpha * x.u + y.alpha * y.u);
    vec3 expo = exp(len - x.alpha - y.alpha) * x.weight * y.weight;
    float other = 1.0 - exp(-2.0 * len);
    return (2.0 * pi * expo * other) / len;
}

SG NDF(vec3 dir, float roughness)
{
    SG ndf;
    ndf.u = dir;
    float m2 = roughness * roughness;
    ndf.alpha = 2.0 / m2;
    ndf.weight = vec3(1.0 / (pi * m2));
    return ndf;
}

SG warp(SG ndf, vec3 v)
{
    SG wNdf;
    wNdf.u = reflect(-v, ndf.u);
    wNdf.weight = ndf.weight;
    wNdf.alpha = ndf.alpha / (4.0 * max(dot(ndf.u, v), 0.001));
    return wNdf;
}

vec3 diffuseSG(SG sg, vec3 normal)
{
    SG cosine = SG(vec3(1.17), normal, 2.133);
    return max(innerProductSG(sg, cosine), vec3(0.0));
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

vec3 specularSG(SG sg, vec3 normal, vec3 view, vec3 albedo, float roughness)
{
    SG ndf = NDF(normal, roughness);
    ndf = warp(ndf, view);
    vec3 color = innerProductSG(ndf, sg);
    color *= GeoSmith(normal, view, ndf.u, roughness);
    vec3 h = normalize(ndf.u + view);
    float p = pow(1.0 - max(dot(ndf.u, h), 0.0), 5.0);
    color *= albedo + (1.0 - albedo) * p;
    return color * max(dot(ndf.u, normal), 0.0);
}

float calcShadow(vec3 pos, vec3 N)
{
    float shadow = 0.0;
    for (uint i = 0u; i < 4u; ++i)
    {
        if (lightDir[i].y <= 0.0)
            continue;
        vec4 pPos = lightMat[i] * vec4(pos, 1.0);
        vec3 projPos = pPos.xyz / pPos.w;
        projPos = projPos * 0.5 + 0.5;
        if (projPos.z > 1.0)
            return 0.0;
        float bias = max(0.05 * (1.0 - dot(N, lightDir[i])), 0.005);
        vec2 ratio = 1.0 / textureSize(shadowMap, 0).xy;
        float curShadow = 0.0;
        for (int x = -3; x <= 3; ++x)
            for (int y = -3; y <= 3; ++y)
                if (projPos.z > texture(shadowMap, vec3(projPos.xy + vec2(x, y) * ratio, float(i))).r + bias)
                    curShadow += 1.0;
        shadow += curShadow / 49.0 * lightWeight[i];
    }
    return shadow;
}

vec3 fresnel(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 evalColor(vec2 uv)
{
    vec3 albedo = texture(gDiffuse, uv).rgb;
    vec3 bgColor = texture(background, uv).xyz;
    vec3 pos = texture(gPos, uv).xyz;
    vec3 N = texture(gNormal, uv).xyz;
    vec3 V = normalize(eye - pos);
    float metallic = texture(gMaterial, uv).x;
    float roughness = texture(gMaterial, uv).y;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 F = fresnel(max(dot(N, V), 0.01), F0, roughness);
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallic;

    // lighting
    vec3 color = vec3(0.0);

    // diffuse  SH
    for (uint i = 0u; i < 16u; ++i)
        color += kD * diffuseSG(light[i], N) * albedo;

    // specular SG
    for (uint i = 0u; i < 3u; ++i)
        color += specularSG(light[i], N, V, F0, roughness);

    color *= scale;

    // color.r = color.g = color.b = 0.30 * color.r + 0.59 * color.g + 0.11 * color.b;

    float shadow = calcShadow(pos, N);
    float ao = texture(ssaoMap, uv).r;

    // background or shadow surface
    if (N == vec3(0.0))
        return bgColor;
    else if (texture(gMask, uv).r != 0.0)
    {
        if (!dispSurface)
            return bgColor * ao * vec3(1.0 - shadow * 0.5, 1.0 - shadow * 0.5, 1.0 - shadow * 0.5);
        else
            return vec3(ao * (1.0 - shadow)) * 0.5 + bgColor * 0.5;
    }
    else return pow(ao * (1.0 - shadow * 0.5) * color, vec3(0.45));
}

void main()
{
    FragColor = vec4(evalColor(TexCoords), 1.0);
}
