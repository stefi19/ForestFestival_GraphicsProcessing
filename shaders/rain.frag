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

// simple random
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453123);
}

void main() {
    vec2 uv = UV;

    // scale UV to control drop column density (larger -> more columns)
    // lower value = fewer columns (rarer rain)
    float scale = columnScale;
    vec2 s = uv * scale;

    // base vertical direction (pointing up in UV space) then rotated by uniform
    float theta = radians(dropRotationDeg);
    float c = cos(theta);
    float sn = sin(theta);
    // rotate base vector (0,1) by theta using 2D rotation matrix
    vec2 dir = vec2(-sn, c);

    // base noise to offset columns
    float n = hash(floor(s));
    // fall speed controlled by uniform for slower/faster rain
    float t = time * fallSpeed + n * 8.0;

    // compute position along streak direction
    float pos = dot(s, dir);

    // make periodic drops
    float drop = fract(pos + t);
    float width = dropWidth; // configurable width of streaks/drops
    // create a soft triangular profile for each drop (center more opaque)
    float alpha = smoothstep(width, 0.0, abs(drop - 0.5));

    // fade out based on vertical uv to simulate perspective (less at top)
    float fade = smoothstep(1.0, 0.0, uv.y);

    // intensity and color
    float a = alpha * intensity * fade;
    vec3 col = rainColor;

    // soften edges slightly and add subtle variation per-column
    a *= 0.8 * smoothstep(0.0, 1.0, fract(pos * 2.0 + t));

    // clamp
    a = clamp(a, 0.0, 1.0);

    color = vec4(col, a);
    // discard fully transparent fragments to avoid unnecessary blending
    if (color.a <= 0.001) discard;
}
