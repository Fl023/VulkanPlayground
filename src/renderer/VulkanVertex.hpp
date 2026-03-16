#pragma once

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> cubeVertices = {
    // --------------------------------------------------------
    // FRONT FACE (+Z)
    // Looking at the front: +Y is Top, -Y is Bottom, -X is Left, +X is Right
    // --------------------------------------------------------
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 0: Top-Left
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 1: Bottom-Left
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 2: Bottom-Right
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 3: Top-Right

    // --------------------------------------------------------
    // BACK FACE (-Z)
    // Looking at the back: +X is Left, -X is Right (because we are behind it)
    // --------------------------------------------------------
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 4: Top-Left (from back)
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 5: Bottom-Left
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 6: Bottom-Right
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 7: Top-Right

    // --------------------------------------------------------
    // LEFT FACE (-X)
    // Looking at the left side: +Z is Right, -Z is Left
    // --------------------------------------------------------
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 8: Top-Left (back corner)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 9: Bottom-Left
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 10: Bottom-Right (front corner)
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 11: Top-Right

    // --------------------------------------------------------
    // RIGHT FACE (+X)
    // Looking at the right side: +Z is Left, -Z is Right
    // --------------------------------------------------------
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 12: Top-Left (front corner)
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 13: Bottom-Left
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 14: Bottom-Right (back corner)
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 15: Top-Right

    // --------------------------------------------------------
    // TOP FACE (+Y)
    // Looking down from above: -Z is Up (Top of texture), +Z is Down
    // --------------------------------------------------------
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 16: Top-Left (back-left corner)
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 17: Bottom-Left (front-left)
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 18: Bottom-Right (front-right)
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 19: Top-Right (back-right)

    // --------------------------------------------------------
    // BOTTOM FACE (-Y)
    // Looking up from underneath: +Z is Up (Top of texture), -Z is Down
    // --------------------------------------------------------
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 20: Top-Left (front-left corner)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 21: Bottom-Left (back-left)
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 22: Bottom-Right (back-right)
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}  // 23: Top-Right (front-right)
};


const std::vector<uint16_t> cubeIndices = {
    // Front face
    0, 1, 2,   0, 2, 3,

    // Back face
    4, 5, 6,   4, 6, 7,

    // Left face
    8, 9, 10,  8, 10, 11,

    // Right face
    12, 13, 14, 12, 14, 15,

    // Top face
    16, 17, 18, 16, 18, 19,

    // Bottom face
    20, 21, 22, 20, 22, 23
};


const std::vector<Vertex> squareVertices = {
    // Index 0: Top-Left
    // +Y is Up, -X is Left                    // U=0 (Left), V=0 (Top)
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    // Index 1: Bottom-Left
    // -Y is Down, -X is Left                  // U=0 (Left), V=1 (Bottom)
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    // Index 2: Bottom-Right
    // -Y is Down, +X is Right                 // U=1 (Right), V=1 (Bottom)
    {{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    // Index 3: Top-Right
    // +Y is Up, +X is Right                   // U=1 (Right), V=0 (Top)
    {{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}
};

const std::vector<uint16_t> squareIndices = {
    0, 1, 2,  // First triangle (Counter-Clockwise)
    0, 2, 3   // Second triangle (Counter-Clockwise)
};

