#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoMap;

void main()
{
    vec2 ratio = 1.0 / vec2(textureSize(ssaoMap, 0));
    float blur = 0.0;
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            vec2 offset = vec2(float(dx), float(dy)) * ratio;
            blur += texture(ssaoMap, TexCoords + offset).r;
        }
    }
    FragColor = blur / 9.0;
}
