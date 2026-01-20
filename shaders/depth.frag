#version 410 core

out vec4 fColor;

void main() {
    // depth-only pass (dummy color)
    fColor = vec4(1.0);
}
