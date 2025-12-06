#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D debugTexture;

void main() {
    vec4 c = texture(debugTexture, TexCoords);
    // sRGB8 texture will be auto-linearized by GPU, framebuffer will apply gamma at output
    FragColor = c;
}
