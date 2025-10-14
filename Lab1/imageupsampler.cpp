#include <inviwo/core/util/logcentral.h>
#include <modules/opengl/texture/textureutils.h>
#include <inviwo/tnm067lab1/processors/imageupsampler.h>
#include <inviwo/tnm067lab1/util/interpolationmethods.h>
#include <inviwo/core/datastructures/image/layerram.h>
#include <inviwo/core/datastructures/image/layerramprecision.h>
#include <inviwo/core/util/imageramutils.h>

namespace inviwo {

namespace detail {

template <typename T>
void upsample(ImageUpsampler::IntepolationMethod method, const LayerRAMPrecision<T>& inputImage,
              LayerRAMPrecision<T>& outputImage) {
    using F = typename float_type<T>::type;

    const size2_t inputSize = inputImage.getDimensions();
    const size2_t outputSize = outputImage.getDimensions();

    const T* inPixels = inputImage.getDataTyped();
    T* outPixels = outputImage.getDataTyped();

    auto inIndex = [&inputSize](auto pos) -> size_t {
        pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(inputSize - size2_t(1)));
        return pos.x + pos.y * inputSize.x;
    };
    auto outIndex = [&outputSize](auto pos) -> size_t {
        pos = glm::clamp(pos, decltype(pos)(0), decltype(pos)(outputSize - size2_t(1)));
        return pos.x + pos.y * outputSize.x;
    };

    util::forEachPixel(outputImage, [&](ivec2 outImageCoords) {
        // outImageCoords: Exact pixel coordinates in the output image currently writing to
        // inImageCoords: Relative coordinates of outImageCoords in the input image, might be
        // between pixels
        dvec2 inImageCoords =
            ImageUpsampler::convertCoordinate(outImageCoords, inputSize, outputSize);

        T finalColor(0);

        // DUMMY COLOR, remove or overwrite this bellow
        finalColor = inPixels[inIndex(
            glm::clamp(size2_t(outImageCoords), size2_t(0), size2_t(inputSize - size2_t(1))))];

        switch (method) {
            case ImageUpsampler::IntepolationMethod::PiecewiseConstant: {
                // Task 6
                finalColor = inPixels[inIndex(round(inImageCoords - dvec2(0.5)))];
                break;
            }
            case ImageUpsampler::IntepolationMethod::Bilinear: {
                auto shift = inImageCoords - dvec2(0.5);
                auto bl = floor(shift); 
                auto br = bl + dvec2(1,0);
                auto tl = bl + dvec2(0,1);
                auto tr = bl + dvec2(1,1);

                std::array<T, 4> vertices = {
                    inPixels[inIndex(bl)], 
                    inPixels[inIndex(br)],  
                    inPixels[inIndex(tl)],  
                    inPixels[inIndex(tr)],  
                };

                double deltaX = shift.x - bl.x;
                double deltaY = shift.y - bl.y;

                finalColor = TNM067::Interpolation::bilinear(vertices, deltaX, deltaY);
                break;
            }
            case ImageUpsampler::IntepolationMethod::Biquadratic: {
                auto shift = inImageCoords - dvec2(0.5);
                auto bl = floor(shift); 
                auto bm = bl + dvec2(1,0);
                auto br = bl + dvec2(2,0);

                auto ml = bl + dvec2(0,1);
                auto mm = bl + dvec2(1,1);
                auto mr = bl + dvec2(2,1);

                auto tl = bl + dvec2(0,2);
                auto tm = bl + dvec2(1,2);
                auto tr = bl + dvec2(2,2);

                std::array<T, 9> vertices = {
                    inPixels[inIndex(bl)],
                    inPixels[inIndex(bm)],
                    inPixels[inIndex(br)],
                    inPixels[inIndex(ml)], 
                    inPixels[inIndex(mm)], 
                    inPixels[inIndex(mr)],
                    inPixels[inIndex(tl)], 
                    inPixels[inIndex(tm)], 
                    inPixels[inIndex(tr)]
                };

                double deltaX = shift.x - bl.x;
                double deltaY = shift.y - bl.y; 
                
                finalColor = TNM067::Interpolation::biQuadratic(vertices, deltaX / 2.0, deltaY / 2.0);

                break;
            }
            case ImageUpsampler::IntepolationMethod::Barycentric: {
                auto shift = inImageCoords - dvec2(0.5);
                auto bl = floor(shift);
                auto br = bl + dvec2(1, 0);
                auto tl = bl + dvec2(0, 1);
                auto tr = bl + dvec2(1, 1);

                std::array<T, 4> vertices = {
                    inPixels[inIndex(bl)],
                    inPixels[inIndex(br)],
                    inPixels[inIndex(tl)],
                    inPixels[inIndex(tr)],
                };

                double deltaX = shift.x - bl.x;
                double deltaY = shift.y - bl.y;

                finalColor = TNM067::Interpolation::barycentric(vertices, deltaX, deltaY);

                break;
            }
            default:
                break;
        }

        outPixels[outIndex(outImageCoords)] = finalColor;
    });
}

}  // namespace detail

const ProcessorInfo ImageUpsampler::processorInfo_{
    "org.inviwo.imageupsampler",  // Class identifier
    "Image Upsampler",            // Display name
    "TNM067",                     // Category
    CodeState::Experimental,      // Code state
    Tags::CPU,                    // Tags
};
const ProcessorInfo& ImageUpsampler::getProcessorInfo() const { return processorInfo_; }

ImageUpsampler::ImageUpsampler()
    : Processor()
    , inport_("inport", true)
    , outport_("outport", true)
    , interpolationMethod_("interpolationMethod", "Interpolation Method",
                           {
                               {"piecewiseconstant", "Piecewise Constant (Nearest Neighbor)",
                                IntepolationMethod::PiecewiseConstant},
                               {"bilinear", "Bilinear", IntepolationMethod::Bilinear},
                               {"quadratic", "Quadratic", IntepolationMethod::Biquadratic},
                               {"barycentric", "Barycentric", IntepolationMethod::Barycentric},
                           }) {
    addPort(inport_);
    addPort(outport_);
    addProperty(interpolationMethod_);
}

void ImageUpsampler::process() {
    auto inputImage = inport_.getData();
    if (inputImage->getDataFormat()->getComponents() != 1) {
        LogError("The ImageUpsampler processor does only support single channel images");
    }

    auto inSize = inport_.getData()->getDimensions();
    auto outDim = outport_.getDimensions();

    auto outputImage = std::make_shared<Image>(outDim, inputImage->getDataFormat());
    outputImage->getColorLayer()->setSwizzleMask(inputImage->getColorLayer()->getSwizzleMask());
    outputImage->getColorLayer()
        ->getEditableRepresentation<LayerRAM>()
        ->dispatch<void, dispatching::filter::Scalars>([&](auto outRep) {
            auto inRep = inputImage->getColorLayer()->getRepresentation<LayerRAM>();
            detail::upsample(interpolationMethod_.get(), *(const decltype(outRep))(inRep), *outRep);
        });

    outport_.setData(outputImage);
}

dvec2 ImageUpsampler::convertCoordinate(ivec2 outImageCoords, size2_t inputSize,
                                        size2_t outputSize) {
    // TODO implement
    dvec2 c(outImageCoords);

    // TASK 5: Convert the outImageCoords to its coordinates in the input image

    return c* (dvec2(inputSize) / dvec2(outputSize)); ;
}

}  // namespace inviwo
