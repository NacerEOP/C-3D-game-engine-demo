#version 450 core

in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    // Store actual distance from fragment to light in the color buffer (as a float in .r)
    float lightDistance = length(FragPos - lightPos);
    FragColor = vec4(lightDistance, 0.0, 0.0, 1.0);
}
