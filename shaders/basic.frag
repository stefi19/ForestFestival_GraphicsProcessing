#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fFragPosLightSpace;

out vec4 fColor;

// shader uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 spotPos[2];
uniform vec3 spotDir[2];
uniform float spotConstant[2];
uniform float spotLinear[2];
uniform float spotQuadratic[2];
uniform float spotCutoffCos[2];
uniform float spotIntensity[2];
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogRadius;
uniform float fogRadiusX;
uniform float fogStretchDown;
uniform vec3 hatCenterWorld;
uniform int fogEnabled;
uniform float fogTime;
uniform int flatShading;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform vec3 materialDiffuse;
uniform int hasDiffuseTexture;
uniform int hasSpecularTexture;
uniform sampler2D shadowMap;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

// Shadow calculation helper
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normalEye) {
    // perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    // if outside shadow map, not in shadow
    if (projCoords.z > 1.0) return 0.0;
    // get closest depth from light's perspective (using [0,1] coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // current depth
    float currentDepth = projCoords.z;
    // reduce shadow acne
    float bias = max(0.0025 * (1.0 - dot(normalEye, normalize(vec3(view * vec4(lightDir,0.0))))), 0.0005);
    // PCF sampling
    float shadow = 0.0;
    float texelSize = 1.0 / 2048.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() 
{
    // compute fragment positions
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 fragPosWorld = vec3(model * vec4(fPosition, 1.0));

    // choose normal
    vec3 normalEye;
    if (flatShading == 1) {
        // compute geometric normal in world space using derivatives
        vec3 geomNormalWorld = normalize(cross(dFdx(fragPosWorld), dFdy(fragPosWorld)));
        normalEye = normalize(normalMatrix * geomNormalWorld);
    } else {
        normalEye = normalize(normalMatrix * fNormal);
    }

    // normalize directional light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    // view direction
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
        vec3 spotPosEye = vec3(view * vec4(spotPos[i], 1.0));
        vec3 spotDirEye = normalize(vec3(view * vec4(spotDir[i], 0.0)));
        vec3 lightDirSpot = normalize(spotPosEye - fPosEye.xyz);
        float theta = dot(normalize(spotDirEye), lightDirSpot);
        float dist = length(spotPosEye - fPosEye.xyz);
        float att = 1.0 / (spotConstant[i] + spotLinear[i] * dist + spotQuadratic[i] * (dist * dist));
        if (theta > spotCutoffCos[i]) {
            float spotEffect = pow(theta, 20.0);
            spotAmbient += att * ambientStrength * lightColor * spotEffect * spotIntensity[i];
            float diff = max(dot(normalEye, lightDirSpot), 0.0);
            spotDiffuse += att * diff * lightColor * spotEffect * spotIntensity[i];
            vec3 reflectSpot = reflect(-lightDirSpot, normalEye);
            float specC = pow(max(dot(viewDir, reflectSpot), 0.0), 32);
            spotSpecular += att * specularStrength * specC * lightColor * spotEffect * spotIntensity[i];
        }
    }

    // determine diffuse color
    vec3 diffCol = hasDiffuseTexture == 1 ? texture(diffuseTexture, fTexCoords).rgb : materialDiffuse;
    vec3 specCol = hasSpecularTexture == 1 ? texture(specularTexture, fTexCoords).rgb : vec3(1.0);

    // combine directional and spot contributions
    vec3 totalAmbient = ambient + spotAmbient;
    vec3 totalDiffuse = diffuse + spotDiffuse;
    vec3 totalSpecular = specular + spotSpecular;

    // compute shadow and final color
    float shadow = ShadowCalculation(fFragPosLightSpace, normalEye);
    vec3 lit = (totalAmbient + (1.0 - shadow) * totalDiffuse) * diffCol + (1.0 - shadow) * totalSpecular * specCol;
    vec3 color = min(lit, 1.0f);

    // compute ellipsoidal mask around hat center
    vec3 delta = fragPosWorld - hatCenterWorld;
    float dx = delta.x;
    float dy = delta.y;
    float dz = delta.z;

    float rx = fogRadiusX;
    float rz = fogRadius;
    float ry = dy < 0.0 ? fogRadius * fogStretchDown : fogRadius * 0.9;
    float wobble = sin(fogTime * 1.2 + (dx + dz) * 0.5) * 0.25;
    dy += wobble;
    float nd = sqrt((dx*dx)/(rx*rx) + (dz*dz)/(rz*rz) + (dy*dy)/(ry*ry));
    float mask = clamp(1.0 - nd, 0.0, 1.0);

    float fogStrength = clamp(mask * fogDensity, 0.0, 1.0) * float(fogEnabled);

    // swirling modulation
    float swirl = 0.85 + 0.15 * sin(3.0 * (dx + dz) + fogTime * 2.0);
    fogStrength *= swirl;

    vec4 litColor = vec4(color, 1.0);
    fColor = mix(litColor, vec4(fogColor, 1.0), fogStrength);
}
