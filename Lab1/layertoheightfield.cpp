/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2024 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <inviwo/tnm067lab1/processors/layertoheightfield.h>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo LayerToHeightfield::processorInfo_{
    "org.inviwo.LayerToHeightfield",  // Class identifier
    "Layer To Heightfield",           // Display name
    "TNM067",                         // Category
    CodeState::Stable,                // Code state
    Tags::None,                       // Tags
    R"(Creating a heightfield from a layer.)"_unindentHelp};

const ProcessorInfo& LayerToHeightfield::getProcessorInfo() const { return processorInfo_; }

LayerToHeightfield::LayerToHeightfield()
    : Processor{}
    , layerInport_("layerInport")
    , meshOutport_("meshOutport")
    , heightScaleFactor_("heightScaleFactor", "Height Scale Factor", 1.0f, 0.001f, 2.0f, 0.001f)
    , numColors_("numColors", "Number of colors", 2, 1, 10)
    , colors_(util::make_array<10>([](auto n) {
        return FloatVec4Property{fmt::format("color{}", n + 1), std::format("Color {}", n + 1),
                                 util::ordinalColor(n == 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 1.0f)};
    })) {

    addPorts(layerInport_, meshOutport_);
    addProperty(heightScaleFactor_);

    addProperty(numColors_);
    for (auto& c : colors_) {
        addProperty(c);
    }

    auto colorVisibility = [&]() {
        for (size_t i = 0; i < 10; i++) {
            colors_[i].setVisible(i < numColors_);
        }
    };

    numColors_.onChange(colorVisibility);
    colorVisibility();
}

namespace {
using HFMesh = TypedMesh<buffertraits::PositionsBuffer, buffertraits::NormalBuffer,
                         buffertraits::ColorsBuffer>;

void addFace(std::vector<HFMesh::Vertex>& vertices, std::vector<unsigned int>& indices,
             const vec3& c1, const vec3& c2, const vec3& c3, const vec3& c4, const vec3& normal,
             const vec4& color) {

    unsigned int startID = static_cast<unsigned int>(vertices.size());
    vertices.emplace_back(c1, normal, color);
    vertices.emplace_back(c2, normal, color);
    vertices.emplace_back(c3, normal, color);
    vertices.emplace_back(c4, normal, color);

    indices.insert(indices.end(),
                   {startID + 0, startID + 1, startID + 2, startID + 0, startID + 2, startID + 3});
}

std::shared_ptr<Mesh> buildMesh(const LayerRAM& image, const ScalarToColorMapping& map,
                                float scaleFactor) {
    const auto dims = image.getDimensions();

    auto mesh = std::make_shared<HFMesh>();
    auto& indices =
        mesh->addIndexBuffer(DrawType::Triangles, ConnectivityType::None)->getDataContainer();

    std::vector<HFMesh::Vertex> vertices;

    const auto bufferSize = 24 * dims.x * dims.y;
    indices.reserve(bufferSize);
    vertices.reserve(bufferSize);

    const vec2 cellSize = 1.0f / vec2(dims);
    util::forEachPixel(image, [&](const size2_t& pos) {
        const vec2 origin2D = vec2(pos) * cellSize;
        const vec3 origin(origin2D.x, 0.0f, origin2D.y);

        // TODO: sample image
        const float imageValue = image.getAsDouble(pos);

        const vec4 color = map.sample(imageValue);

        const float height = imageValue*scaleFactor;

        // Box Corners
        const auto zero = origin + vec3(0.0f, 0.0f, 0.0f);
        const auto px = origin + vec3(cellSize.x, 0.0f, 0.0f);
        const auto pz = origin + vec3(0.0f, 0.0f, cellSize.y);
        const auto py = origin + vec3(0.0f, height, 0.0f);
        const auto pxpy = origin + vec3(cellSize.x, height, 0.0f);
        const auto pxpz = origin + vec3(cellSize.x, 0.0f, cellSize.y);
        const auto pypz = origin + vec3(0.0f, height, cellSize.y);
        const auto pxpypz = origin + vec3(cellSize.x, height, cellSize.y);

        // Box Normals
        constexpr auto down = vec3(0.0f, -1.0f, 0.0f);
        constexpr auto up = vec3(0.0f, 1.0f, 0.0f);
        constexpr auto left = vec3(-1.0f, 0.0f, 0.0f);
        constexpr auto right = vec3(1.0f, 0.0f, 0.0f);
        constexpr auto front = vec3(0.0f, 0.0f, -1.0f);
        constexpr auto back = vec3(0.0f, 0.0f, 1.0f);

        addFace(vertices, indices, zero, px, pxpz, pz, down, color);       // Bottom face
        addFace(vertices, indices, py, pypz, pxpypz, pxpy, up, color);     // Top face
        addFace(vertices, indices, zero, pz, pypz, py, left, color);       // Left face
        addFace(vertices, indices, px, pxpy, pxpypz, pxpz, right, color);  // Right face
        addFace(vertices, indices, zero, py, pxpy, px, front, color);      // Front face
        addFace(vertices, indices, pz, pxpz, pxpypz, pypz, back, color);   // Back face
    });

    mesh->addVertices(vertices);

    return mesh;
}

}  // namespace

void LayerToHeightfield::process() {
    const auto layer = layerInport_.getData()->getRepresentation<LayerRAM>();

    ScalarToColorMapping map;
    for (size_t i = 0; i < numColors_.get(); i++) {
        map.addBaseColors(colors_[i].get());
    }

    const auto mesh = buildMesh(*layer, map, heightScaleFactor_);

    meshOutport_.setData(mesh);
}

}  // namespace inviwo
