#include <inviwo/tnm067lab2/processors/hydrogengenerator.h>
#include <inviwo/core/datastructures/volume/volume.h>
#include <inviwo/core/util/volumeramutils.h>
#include <modules/base/algorithm/dataminmax.h>
#include <inviwo/core/util/indexmapper.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <modules/base/algorithm/dataminmax.h>

#include <numbers>

namespace inviwo {

const ProcessorInfo HydrogenGenerator::processorInfo_{
    "org.inviwo.HydrogenGenerator",  // Class identifier
    "Hydrogen Generator",            // Display name
    "TNM067",                        // Category
    CodeState::Stable,               // Code state
    Tags::CPU,                       // Tags
};

const ProcessorInfo& HydrogenGenerator::getProcessorInfo() const { return processorInfo_; }

HydrogenGenerator::HydrogenGenerator()
    : Processor(), volume_("volume"), size_("size_", "Volume Size", 16, 4, 256) {
    addPort(volume_);
    addProperty(size_);
}

void HydrogenGenerator::process() {
    auto ram = std::make_shared<VolumeRAMPrecision<float>>(size3_t(size_));
    auto vol = std::make_shared<Volume>(ram);

    auto data = ram->getDataTyped();
    util::IndexMapper3D index(ram->getDimensions());

    util::forEachVoxel(*ram, [&](const size3_t& pos) {
        vec3 cartesian = idTOCartesian(pos);
        data[index(pos)] = static_cast<float>(eval(cartesian));
    });

    auto minMax = util::volumeMinMax(ram.get());
    vol->dataMap.dataRange = vol->dataMap.valueRange = dvec2(minMax.first.x, minMax.second.x);

    volume_.setData(vol);
}

vec3 HydrogenGenerator::cartesianToSpherical(vec3 cartesian) {
    vec3 sph;

    sph.x = sqrt(pow(cartesian.x, 2) + pow(cartesian.y, 2) + pow(cartesian.z, 2));
    if (sph.x == 0)
        sph.y = 0.0;
    else
        sph.y = acos(cartesian.z / sph.x);
    sph.z = atan2(cartesian.y, cartesian.x);

    return sph;
}

double HydrogenGenerator::eval(vec3 cartesian) {
    // Get spherical coordinates
    vec3 spherical = cartesianToSpherical(cartesian);

    // split wave func to 5 parts

    double waveFunc, Z = 1, a0 = 1, r = spherical.x, teta = spherical.y;
    waveFunc = 1 / (81 * sqrt(6 * M_PI));                // yellow
    waveFunc *= pow(Z / a0, 1.5);                        // red
    waveFunc *= (pow(Z, 2) * pow(r, 2)) / (pow(a0, 2));  // blue
    waveFunc *= exp((-1 * Z * r) / (3.0 * a0));          // green
    waveFunc *= 3 * pow(cos(teta), 2) - 1;               // pink

    return pow(abs(waveFunc), 2);
}

vec3 HydrogenGenerator::idTOCartesian(size3_t pos) {
    vec3 p(pos);
    p /= size_ - 1;
    return p * (36.0f) - 18.0f;
}

}  // namespace inviwo
