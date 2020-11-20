#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform vec2 rayDir;
uniform sampler2D depth;

float depthOcc(vec2 uv, vec2 dir)
{
    float ratio = 1.5 / textureSize(depth, 0).x;
    float cDepth = texture(depth, uv).r;
    float occ = 0.0;
    for (int theta = -10; theta <= 10; theta += 5)
    {
        float t = float(theta) / 180.0 * 3.14159265359;
        mat2 mat = mat2(vec2(cos(t), sin(t)), vec2(-sin(t), cos(t)));
        vec2 curDir = mat * dir;
        curDir *= ratio;
        float tmp = 0.0;
        for (int i = 0; i < 16; i++)
        {
            float d = texture(depth, uv + curDir * float(i)).r;
            tmp += max(sign(cDepth - d - 0.02), 0.0);
        }
        occ += sign(tmp);
    }
    return occ / 5.0;
}

void main()
{
    FragColor = depthOcc(TexCoords, normalize(rayDir));
}
