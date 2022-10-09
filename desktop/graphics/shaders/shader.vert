#version 450

layout(set = 0, binding = 0) uniform Projection {
    mat4 m;
} projection;
layout(set = 0, binding = 1) uniform View {
    mat4 m;
} view;

layout(push_constant) uniform PushContants {
    layout(offset = 0) mat3x4 model;
} push;

layout(set = 2, binding = 0) uniform Light {
    vec4 color;
    vec4 from;
    float contrast;
    float softness;
} light;

layout(location = 0) in  vec3 inPosition;
layout(location = 1) in  vec4 inColor;
layout(location = 2) in  vec3 inNormal;
layout(location = 3) in  vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    mat4 model = mat4(push.model);
    
    gl_Position = projection.m * view.m * model * vec4(inPosition, 1);
    
    fragColor.a = inColor.a;
    
    float exposure = dot(light.from.xyz, (model * vec4(inNormal, 1)).xyz);
    if (exposure < -light.softness) {
        fragColor.rgb = inColor.rgb * (
            (exposure + 1) * ((0.5 - light.contrast) / (1 - light.softness))
        );
    } else if (exposure < light.softness) {
        // FIXME
        fragColor.rgb =
            inColor.rgb
            * (
                0.5 + exposure * light.contrast / light.softness
            )
            * (
                (+exposure / light.contrast + 1) / 2 * light.color.rgb +
                (-exposure / light.contrast + 1) / 2 * vec3(1, 1, 1)
            );
    } else {
        fragColor.rgb =
            inColor.rgb
            * (
                0.5 + light.contrast + (exposure - light.softness) * ((0.5 - light.contrast) / (1 - light.softness))
            )
            * light.color.rgb;
    }
    
    fragTexCoord = inTexCoord;
}
