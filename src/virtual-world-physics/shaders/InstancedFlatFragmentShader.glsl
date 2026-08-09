#version 330

in vec3 world_position;
flat in vec3 world_normal;
flat in vec3 object_color;

uniform vec3 light_position;
uniform vec3 light_direction;
uniform vec3 eye_position;

uniform float material_kd;
uniform float material_ks;
uniform int material_shininess;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 N = normalize(world_normal);
    vec3 L = normalize(light_position - world_position);
    vec3 V = normalize(eye_position - world_position);
    vec3 H = normalize(L + V);

    float ambient_light = 0.25;
    vec3 ambient_color = object_color * ambient_light;

    float diffuse_light = material_kd * max(dot(N, L), 0.0);
    vec3 diffuse_color = object_color * diffuse_light;

    float specular_light = 0.0;
    if (diffuse_light > 0.0) {
        specular_light = material_ks * pow(max(dot(N, H), 0.0), material_shininess);
    }
    vec3 specular_color = vec3(1.0) * specular_light;

    vec3 color = ambient_color + diffuse_color + specular_color;
    out_color = vec4(color, 1.0);
}
