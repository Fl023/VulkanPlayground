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
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // A 0
        {{0.5f, -0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},  // B 1
        {{0.5f,  0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},  // C 2
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},  // D 3
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // E 4
        {{0.5f, -0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},   // F 5
        {{0.5f,  0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},   // G 6
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},   // H 7
                                                                
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // D 8
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},  // A 9
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},  // E 10
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},  // H 11
        {{0.5f, -0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},   // B 12
        {{0.5f,  0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},   // C 13
        {{0.5f,  0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},   // G 14
        {{0.5f, -0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},   // F 15

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // A 16
        {{0.5f, -0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},   // B 17
        {{0.5f, -0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},   // F 18
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},  // E 19
        {{0.5f,  0.5f, -0.5f }, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // C 20
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},  // D 21
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},  // H 22
        {{0.5f,  0.5f,  0.5f }, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}  // G 23
};                               


const std::vector<uint16_t> cubeIndices = {
    // front and back
    0, 3, 2,
    2, 1, 0,
    4, 5, 6,
    6, 7 ,4,
    // left and right
    11, 8, 9,
    9, 10, 11,
    12, 13, 14,
    14, 15, 12,
    // bottom and top
    16, 17, 18,
    18, 19, 16,
    20, 21, 22,
    22, 23, 20
};


const std::vector<Vertex> squareVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> squareIndices = {
    0, 1, 2, 2, 3, 0
};

