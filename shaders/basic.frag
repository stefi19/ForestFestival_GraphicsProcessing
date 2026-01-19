#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
// fog (localized around hat)
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogRadius; // depth (Z) radius
uniform float fogRadiusX; // left/right (X) radius
uniform float fogStretchDown;
uniform vec3 hatCenterWorld;
uniform int fogEnabled;
uniform float fogTime;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform vec3 materialDiffuse;
uniform int hasDiffuseTexture;
uniform int hasSpecularTexture;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

void main() 
{
    computeDirLight();

    // determine diffuse color (texture if present, otherwise material)
    vec3 diffCol = hasDiffuseTexture == 1 ? texture(diffuseTexture, fTexCoords).rgb : materialDiffuse;
    vec3 specCol = hasSpecularTexture == 1 ? texture(specularTexture, fTexCoords).rgb : vec3(1.0);

    //compute final vertex color
    vec3 color = min((ambient + diffuse) * diffCol + specular * specCol, 1.0f);

    // compute fragment world position
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));
    // compute ellipsoidal mask around hat center, stretched downward to cover the scene
    vec3 delta = fragPosWorld - hatCenterWorld;
    float dx = delta.x;
    float dy = delta.y;
    float dz = delta.z;

    // horizontal radii: X can be stretched separately from Z to extend left/right
    float rx = fogRadiusX;
    float rz = fogRadius;
    // vertical radius: larger downward, slightly larger upward to make fog extend a bit up
    float ry = dy < 0.0 ? fogRadius * fogStretchDown : fogRadius * 0.9;

    // add a subtle vertical wobble so the fog appears to float
    float wobble = sin(fogTime * 1.2 + (dx + dz) * 0.5) * 0.25; // amplitude ~0.25 units
    // apply wobble to vertical delta so the ellipsoid moves up/down locally
    dy += wobble;

    // normalized ellipsoid distance
    float nd = sqrt((dx*dx)/(rx*rx) + (dz*dz)/(rz*rz) + (dy*dy)/(ry*ry));
    // mask: 1 at center, 0 at or beyond ellipsoid
    float mask = clamp(1.0 - nd, 0.0, 1.0);

    float fogStrength = clamp(mask * fogDensity, 0.0, 1.0) * float(fogEnabled);

    // subtle swirling modulation to make fog look more organic
    float swirl = 0.85 + 0.15 * sin(3.0 * (dx + dz) + fogTime * 2.0);
    fogStrength *= swirl;

    vec4 litColor = vec4(color, 1.0);
    fColor = mix(litColor, vec4(fogColor, 1.0), fogStrength);
}
