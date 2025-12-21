#version 450

// Fullscreen quad vertex shader - generates vertices procedurally
// No vertex buffer needed, just use gl_VertexIndex

layout(location = 0) out vec2 v_texCoord;

void main() {
    // Generate fullscreen triangle that covers the screen
    // Vertex 0: (-1, -1), UV (0, 1)
    // Vertex 1: (3, -1),  UV (2, 1)
    // Vertex 2: (-1, 3),  UV (0, -1)
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2(3.0, -1.0),
        vec2(-1.0, 3.0)
    );

    vec2 texCoords[3] = vec2[3](
        vec2(0.0, 1.0),
        vec2(2.0, 1.0),
        vec2(0.0, -1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    v_texCoord = texCoords[gl_VertexIndex];
}
