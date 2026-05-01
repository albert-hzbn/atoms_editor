#include "graphics/CellSculptorShaders.h"

const char* cellSculptorPlaneVertexShader()
{
    return R"glsl(
#version 130
in vec3 position;
uniform mat4 projection;
uniform mat4 view;
out vec3 fragWorldPos;
void main() {
    fragWorldPos = position;
    gl_Position = projection * view * vec4(position, 1.0);
}
)glsl";
}

const char* cellSculptorPlaneFragmentShader()
{
    return R"glsl(
#version 130
uniform vec3 faceColor;
uniform vec3 faceNormal;
uniform vec3 lightPos;
uniform vec3 viewPos;
in vec3 fragWorldPos;
out vec4 color;
void main() {
    vec3 norm = normalize(faceNormal);
    if (!gl_FrontFacing) norm = -norm;
    vec3 viewDir  = normalize(viewPos - fragWorldPos);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 halfVec  = normalize(lightDir + viewDir);
    float diffuse  = max(dot(norm, lightDir), 0.0);
    diffuse = max(diffuse, 0.30);
    float specular = pow(max(dot(norm, halfVec), 0.0), 18.0);
    float fresnel  = pow(1.0 - max(dot(norm, viewDir), 0.0), 2.5);
    vec3 reflected = reflect(-viewDir, norm);
    float skyMix   = clamp(reflected.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 envColor  = mix(vec3(0.10, 0.11, 0.13), vec3(0.72, 0.78, 0.86),
                         pow(skyMix, 1.35));
    vec3 baseLit   = faceColor * (0.28 + 0.72 * diffuse);
    vec3 refl      = mix(baseLit, envColor, 0.12 * fresnel);
    vec3 final_col = refl + vec3(0.10 * specular + 0.07 * fresnel);
    color = vec4(final_col, 0.52);
}
)glsl";
}
