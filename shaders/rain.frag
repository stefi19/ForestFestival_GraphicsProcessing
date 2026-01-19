#version 410 core

in vec2 UV;
out vec4 color;

uniform float time;
uniform vec3 camPos;
uniform float intensity; // 0..1
uniform vec3 rainColor;

// simple random
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = UV;

    // scale UV to get more streaks
    float scale = 60.0;
    vec2 s = uv * scale;

    // vertical falling (no diagonal slant)
    vec2 dir = vec2(0.0, 1.0);

    // base noise to offset columns
    float n = hash(floor(s));
    float t = time * 3.0 + n * 10.0;

    // compute position along streak direction
    float pos = dot(s, dir);

    // make periodic drops
    float drop = fract(pos + t);
    float width = 0.02; // thin streak
    float alpha = smoothstep(width, 0.0, abs(drop - 0.5));

    // fade out based on vertical uv to simulate perspective (less at top)
    float fade = smoothstep(1.0, 0.0, uv.y);

    // intensity and color
    float a = alpha * intensity * fade;
    vec3 col = rainColor;

    // soften edges
    a *= smoothstep(0.0, 1.0, fract(pos * 2.0 + t));

    // clamp
    a = clamp(a, 0.0, 1.0);

    color = vec4(col, a);
    // discard fully transparent fragments to avoid unnecessary blending
    if (color.a <= 0.001) discard;
}
