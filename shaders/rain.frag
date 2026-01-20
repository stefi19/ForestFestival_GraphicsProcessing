#version 410 core

in vec2 UV;
out vec4 color;

uniform float time;
uniform vec3 camPos;
uniform float intensity; // 0..1
uniform vec3 rainColor;
uniform float dropWidth; // width of individual drops/streaks
uniform float fallSpeed; // multiplier for fall speed
uniform float columnScale; // controls density of drop columns
uniform float dropRotationDeg; // rotate drop direction (degrees)

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = UV;

    float scale = columnScale;
    vec2 s = uv * scale;
    float theta = radians(dropRotationDeg);
    float c = cos(theta);
    float sn = sin(theta);
    vec2 dir = vec2(-sn, c);
    float n = hash(floor(s));
    float t = time * fallSpeed + n * 8.0;
    float pos = dot(s, dir);
    float drop = fract(pos + t);
    float width = dropWidth;
    float alpha = smoothstep(width, 0.0, abs(drop - 0.5));
    float fade = smoothstep(1.0, 0.0, uv.y);
    float a = alpha * intensity * fade;
    vec3 col = rainColor;
    a *= 0.8 * smoothstep(0.0, 1.0, fract(pos * 2.0 + t));
    a = clamp(a, 0.0, 1.0);
    color = vec4(col, a);
    if (color.a <= 0.001) discard;
}
