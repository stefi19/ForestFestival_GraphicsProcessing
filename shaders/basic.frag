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
// spotlights
uniform vec3 spotPos[2];
uniform vec3 spotDir[2];
uniform float spotConstant[2];
uniform float spotLinear[2];
uniform float spotQuadratic[2];
uniform float spotCutoffCos[2]; // cosine of cutoff angle
uniform float spotIntensity[2];
// fog (localized around hat)
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogRadius; // depth (Z) radius
uniform float fogRadiusX; // left/right (X) radius
uniform float fogStretchDown;
uniform vec3 hatCenterWorld;
uniform int fogEnabled;
uniform float fogTime;
uniform int flatShading;
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

void main() 
{
    // compute fragment positions
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));

    // choose normal: geometric face normal when flat shading requested, otherwise interpolated normal
    vec3 normalEye;
    if (flatShading == 1) {
        // compute geometric normal in world space using derivatives
        vec3 geomNormalWorld = normalize(cross(dFdx(fragPosWorld), dFdy(fragPosWorld)));
        normalEye = normalize(normalMatrix * geomNormalWorld);
    } else {
        normalEye = normalize(normalMatrix * fNormal);
    }

    //normalize directional light direction (in eye space)
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin)
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute base directional ambient/diffuse/specular
    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;

    // add contributions from spotlights
    vec3 spotAmbient = vec3(0.0);
    vec3 spotDiffuse = vec3(0.0);
    vec3 spotSpecular = vec3(0.0);
    for (int i = 0; i < 2; ++i) {
        // convert spotlight position and direction to eye space
        vec3 spotPosEye = vec3(view * vec4(spotPos[i], 1.0));
        vec3 spotDirEye = normalize(vec3(view * vec4(spotDir[i], 0.0)));
        // direction from fragment to light (eye space)
        vec3 lightDirSpot = normalize(spotPosEye - fPosEye.xyz);
        // angle between spotlight direction and fragment direction
        float theta = dot(normalize(spotDirEye), lightDirSpot);
        // distance attenuation
        float dist = length(spotPosEye - fPosEye.xyz);
        float att = 1.0 / (spotConstant[i] + spotLinear[i] * dist + spotQuadratic[i] * (dist * dist));
        if (theta > spotCutoffCos[i]) {
            // optional smooth edge using pow of theta
            float spotEffect = pow(theta, 20.0);
            // ambient contribution
            spotAmbient += att * ambientStrength * lightColor * spotEffect * spotIntensity[i];
            // diffuse
            float diff = max(dot(normalEye, lightDirSpot), 0.0);
            spotDiffuse += att * diff * lightColor * spotEffect * spotIntensity[i];
            // specular
            vec3 reflectSpot = reflect(-lightDirSpot, normalEye);
            float specC = pow(max(dot(viewDir, reflectSpot), 0.0), 32);
            spotSpecular += att * specularStrength * specC * lightColor * spotEffect * spotIntensity[i];
        }
    }

    // determine diffuse color (texture if present, otherwise material)
    vec3 diffCol = hasDiffuseTexture == 1 ? texture(diffuseTexture, fTexCoords).rgb : materialDiffuse;
    vec3 specCol = hasSpecularTexture == 1 ? texture(specularTexture, fTexCoords).rgb : vec3(1.0);

    // combine directional + spot contributions
    vec3 totalAmbient = ambient + spotAmbient;
    vec3 totalDiffuse = diffuse + spotDiffuse;
    vec3 totalSpecular = specular + spotSpecular;

    // compute final vertex color
    vec3 color = min((totalAmbient + totalDiffuse) * diffCol + totalSpecular * specCol, 1.0f);

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
