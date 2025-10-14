#include <inviwo/tnm067lab3/processors/marchingtetrahedra.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/util/indexmapper.h>
#include <inviwo/core/util/assertion.h>
#include <inviwo/core/network/networklock.h>
#include <inviwo/tnm067lab1/util/interpolationmethods.h>
#include <iostream>
#include <fstream>

namespace inviwo {

const ProcessorInfo MarchingTetrahedra::processorInfo_{
    "org.inviwo.MarchingTetrahedra",  // Class identifier
    "Marching Tetrahedra",            // Display name
    "TNM067",                         // Category
    CodeState::Stable,                // Code state
    Tags::CPU,                        // Tags
};
const ProcessorInfo& MarchingTetrahedra::getProcessorInfo() const { return processorInfo_; }

MarchingTetrahedra::MarchingTetrahedra()
    : Processor()
    , volume_("volume")
    , mesh_("mesh")
    , isoValue_("isoValue", "ISO value", 0.5f, 0.0f, 1.0f) {

    addPort(volume_);
    addPort(mesh_);

    addProperty(isoValue_);

    isoValue_.setSerializationMode(PropertySerializationMode::All);

    volume_.onChange([&]() {
        if (!volume_.hasData()) {
            return;
        }
        NetworkLock lock(getNetwork());
        float iso = (isoValue_.get() - isoValue_.getMinValue()) /
                    (isoValue_.getMaxValue() - isoValue_.getMinValue());
        const auto vr = volume_.getData()->dataMap.valueRange;
        isoValue_.setMinValue(static_cast<float>(vr.x));
        isoValue_.setMaxValue(static_cast<float>(vr.y));
        isoValue_.setIncrement(static_cast<float>(glm::abs(vr.y - vr.x) / 50.0));
        isoValue_.set(static_cast<float>(iso * (vr.y - vr.x) + vr.x));
        isoValue_.setCurrentStateAsDefault();
    });
}


void MarchingTetrahedra::process() {
    auto volume = volume_.getData()->getRepresentation<VolumeRAM>();
    MeshHelper mesh(volume_.getData());

    const auto& dims = volume->getDimensions();

    const float iso = isoValue_.get();

    util::IndexMapper3D mapVolPosToIndex(dims);

    static constexpr std::array<std::array<size_t, 4>, 6> tetrahedraIds = {
        {{0, 1, 2, 5}, {1, 3, 2, 5}, {3, 2, 5, 7}, {0, 2, 4, 5}, {6, 4, 2, 5}, {6, 7, 5, 2}}};

    size3_t pos{};
    for (pos.z = 0; pos.z < dims.z - 1; ++pos.z) {
        for (pos.y = 0; pos.y < dims.y - 1; ++pos.y) {
            for (pos.x = 0; pos.x < dims.x - 1; ++pos.x) {
                // The DataPoint index should be the 1D-index for the DataPoint in the cell
                // Use volume->getAsDouble to query values from the volume
                // Spatial position should be between 0 and 1

                // TODO: TASK 2: create a nested for loop to construct the cell
                Cell c;

                for (size_t z = 0; z <= 1; z++) {
                    for (size_t y = 0; y <= 1; y++) {
                        for (size_t x = 0; x <= 1; x++) {

                            vec3 globalPos(x + pos.x, y + pos.y, z + pos.z);

                            auto pstn = calculateDataPointPos(pos, ivec3(x, y, z), dims);
                            auto val = volume->getAsDouble(globalPos);
                            auto iiv = mapVolPosToIndex(globalPos);

                            auto vert = calculateDataPointIndexInCell(ivec3(x, y, z));

                            c.dataPoints[vert].pos = pstn;
                            c.dataPoints[vert].value = val;
                            c.dataPoints[vert].indexInVolume = iiv;

                        }
                    }
                }

                // TODO: TASK 3: Subdivide cell into 6 tetrahedra (hint: use tetrahedraIds)
                std::vector<Tetrahedra> tetrahedras;

                // för varje datapoint finns det bara de antal terahedras möjliga som finns i tetrahedrIDS

                for (size_t i = 0; i < tetrahedraIds.size(); i++) { // gå igenom tetrahedra ids
                    Tetrahedra curr{};
                    for (size_t j = 0; j < tetrahedraIds[0].size(); j++) { // gå igenom varje vert i arr gör till datapoint
                        curr.dataPoints[j] = c.dataPoints[tetrahedraIds[i][j]];
                    }

                    tetrahedras.push_back(curr);
                }


                for (const Tetrahedra& tetrahedra : tetrahedras) {
                    // TODO: TASK 4: Calculate case id for each tetrahedra, and add triangles for
                    // each case (use MeshHelper)

                    // Calculate for tetra case index
                    int caseId = (tetrahedra.dataPoints[0].value >= iso ? 1 : 0)
                    + (tetrahedra.dataPoints[1].value >= iso ? 2 : 0)
                    + (tetrahedra.dataPoints[2].value >= iso ? 4 : 0)
                    + (tetrahedra.dataPoints[3].value >= iso ? 8 : 0);
                    
                    // Extract triangles
                    const auto& l0 = tetrahedra.dataPoints[0];
                    const auto& l1 = tetrahedra.dataPoints[1];
                    const auto& l2 = tetrahedra.dataPoints[2];
                    const auto& l3 = tetrahedra.dataPoints[3];

                    switch (caseId) {
                        case 0:
                        case 15: 
                            break;
                        case 1:
                        case 14: { // mellan 0 och 1,2,3

                            auto v1 = addVhelp(iso, l0, l1, mesh);
                            auto v2 = addVhelp(iso, l0, l2, mesh);
                            auto v3 = addVhelp(iso, l0, l3, mesh);

                            if (caseId == 1) mesh.addTriangle(v2, v1, v3);
                            else mesh.addTriangle(v3, v1, v2);

                            break;
                        }
                        case 2:
                        case 13: {
                            
                            auto v3 = addVhelp(iso, l1, l3, mesh);
                            auto v2 = addVhelp(iso, l1, l2, mesh);
                            auto v0 = addVhelp(iso, l1, l0, mesh);

                            if (caseId == 2)
                                mesh.addTriangle(v2,v3,v0);
                            else
                                mesh.addTriangle(v0,v3,v2);

                            break;
                        }
                        case 3:
                        case 12: {

                            auto v3 = addVhelp(iso, l1, l3, mesh);
                            auto v2 = addVhelp(iso, l1, l2, mesh);
                            auto v0 = addVhelp(iso, l0, l3, mesh);

                            if (caseId == 3) mesh.addTriangle(v3,v0,v2);
                            else mesh.addTriangle(v2,v0,v3);

                            v3 = addVhelp(iso, l0, l3, mesh);
                            v2 = addVhelp(iso, l0, l2, mesh);
                            auto v1 = addVhelp(iso, l1, l2, mesh);

                            if (caseId == 12) mesh.addTriangle(v2, v3, v1);
                            else mesh.addTriangle(v1, v3, v2);

                            break;
                        }
                        case 4:
                        case 11: {

                            auto v3 = addVhelp(iso, l2, l3, mesh);
                            auto v0 = addVhelp(iso, l2, l0, mesh);
                            auto v1 = addVhelp(iso, l2, l1, mesh);

                            if (caseId == 4) mesh.addTriangle(v1, v0, v3);
                            else mesh.addTriangle(v3, v0, v1);

                            break;
                        }
                        case 5:
                        case 10: {

                            auto v1 = addVhelp(iso, l0, l1, mesh);
                            auto v2 = addVhelp(iso, l2, l1, mesh);
                            auto v3 = addVhelp(iso, l0, l3, mesh);

                            if (caseId == 5)
                                mesh.addTriangle(v1,v3,v2);
                            else
                                mesh.addTriangle(v2,v3,v1);

                            v1 = addVhelp(iso, l0, l3, mesh);
                            v2 = addVhelp(iso, l2, l3, mesh);
                            v3 = addVhelp(iso, l2, l1, mesh);

                            if (caseId == 5)
                                mesh.addTriangle(v1,v2,v3);
                            else
                                mesh.addTriangle(v3,v2,v1);

                            break;
                        }
                        case 6:
                        case 9: {

                            auto v0 = addVhelp(iso, l1, l0, mesh);
                            auto v3 = addVhelp(iso, l1, l3, mesh);
                            auto v2 = addVhelp(iso, l2, l0, mesh);

                            if (caseId == 9)
                                mesh.addTriangle(v0,v3,v2);
                            else
                                mesh.addTriangle(v2, v3, v0);

                            auto v20 = addVhelp(iso, l2, l0, mesh);
                            auto v23 = addVhelp(iso, l2, l3, mesh);
                            auto v13 = addVhelp(iso, l1, l3, mesh);

                            if (caseId == 6)
                                mesh.addTriangle(v20,v23,v13);
                            else
                                mesh.addTriangle(v13,v23,v20);
                            break;
                        }
                        case 7:
                        case 8: {

                            auto v2 = addVhelp(iso, l3, l2, mesh);
                            auto v1 = addVhelp(iso, l3, l1, mesh);
                            auto v0 = addVhelp(iso, l3, l0, mesh);

                            if (caseId == 8)
                                mesh.addTriangle(v1, v2, v0);
                            else
                                mesh.addTriangle(v0, v2, v1);

                            break;
                        }
                    }
                }
            }
        }
    }

    mesh_.setData(mesh.toBasicMesh());
}

vec3 MarchingTetrahedra::linInterp(const float iso, const DataPoint& A, const DataPoint& B) {
    auto t = abs((iso - A.value) / (B.value - A.value));
    return A.pos * (vec3(1.0) - t) + B.pos * t;
}

uint32_t MarchingTetrahedra::addVhelp(const float iso, const DataPoint& A, const DataPoint& B, MeshHelper& mesh) {
    return mesh.addVertex(linInterp(iso, A, B), A.indexInVolume, B.indexInVolume);
}

int MarchingTetrahedra::calculateDataPointIndexInCell(ivec3 index3D) {

    return index3D.x + 2 * index3D.y + 4 * index3D.z; // eq found online, x+y*xmax+z*xmax*ymax
}

vec3 MarchingTetrahedra::calculateDataPointPos(size3_t posVolume, ivec3 posCell, ivec3 dims) {
    auto x = (posVolume[0] + posCell[0]) / (dims[0] - 1.0);
    auto y = (posVolume[1] + posCell[1]) / (dims[1] - 1.0);
    auto z = (posVolume[2] + posCell[2]) / (dims[2] - 1.0);

    return vec3(x, y, z);
}

MarchingTetrahedra::MeshHelper::MeshHelper(std::shared_ptr<const Volume> vol)
    : edgeToVertex_()
    , vertices_()
    , mesh_(std::make_shared<BasicMesh>())
    , indexBuffer_(mesh_->addIndexBuffer(DrawType::Triangles, ConnectivityType::None)) {
    mesh_->setModelMatrix(vol->getModelMatrix());
    mesh_->setWorldMatrix(vol->getWorldMatrix());
}

void MarchingTetrahedra::MeshHelper::addTriangle(size_t i0, size_t i1, size_t i2) {
    IVW_ASSERT(i0 != i1, "i0 and i1 should not be the same value");
    IVW_ASSERT(i0 != i2, "i0 and i2 should not be the same value");
    IVW_ASSERT(i1 != i2, "i1 and i2 should not be the same value");

    indexBuffer_->add(static_cast<glm::uint32_t>(i0));
    indexBuffer_->add(static_cast<glm::uint32_t>(i1));
    indexBuffer_->add(static_cast<glm::uint32_t>(i2));

    const auto a = std::get<0>(vertices_[i0]);
    const auto b = std::get<0>(vertices_[i1]);
    const auto c = std::get<0>(vertices_[i2]);

    const vec3 n = glm::normalize(glm::cross(b - a, c - a));
    std::get<1>(vertices_[i0]) += n;
    std::get<1>(vertices_[i1]) += n;
    std::get<1>(vertices_[i2]) += n;
}

std::shared_ptr<BasicMesh> MarchingTetrahedra::MeshHelper::toBasicMesh() {
    for (auto& vertex : vertices_) {
        // Normalize the normal of the vertex
        std::get<1>(vertex) = glm::normalize(std::get<1>(vertex));
    }
    mesh_->addVertices(vertices_);
    return mesh_;
}

std::uint32_t MarchingTetrahedra::MeshHelper::addVertex(vec3 pos, size_t i, size_t j) {
    IVW_ASSERT(i != j, "i and j should not be the same value");
    if (j < i) std::swap(i, j);

    auto [edgeIt, inserted] = edgeToVertex_.try_emplace(std::make_pair(i, j), vertices_.size());
    if (inserted) {
        vertices_.push_back({pos, vec3(0, 0, 0), pos, vec4(0.7f, 0.7f, 0.7f, 1.0f)});
    }
    return static_cast<std::uint32_t>(edgeIt->second);
}

}  // namespace inviwo
