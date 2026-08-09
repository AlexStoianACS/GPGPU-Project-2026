#version 330

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

// Per-instance attributes
layout(location = 3) in mat4 instanceModel;
layout(location = 7) in vec3 instanceColor;

uniform mat4 View;
uniform mat4 Projection;

out vec3 world_position;
flat out vec3 world_normal;
flat out vec3 object_color;

void main()
{
    world_position = vec3(instanceModel * vec4(v_position, 1.0));    
    world_normal = normalize(mat3(instanceModel) * v_normal);
    object_color = instanceColor;
    gl_Position = Projection * View * vec4(world_position, 1.0);
}
