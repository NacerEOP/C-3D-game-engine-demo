#version 450 core
out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Pure red
}
