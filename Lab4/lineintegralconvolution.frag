#include "utils/structs.glsl"

uniform sampler2D inport;
uniform sampler2D noiseTexture;

uniform ImageParameters inportParameters;
uniform ImageParameters outportParameters;

uniform int nSteps;
uniform float stepSize;

in vec3 texCoord_;

/*
* Traverse the vector field and sample the noise image
* @param posF Starting position
* @param stepSize length of each step
* @param nSteps the number of steps to traverse
* @param accVal the accumulated value from sampling the noise image
* @param nSamples the number of samples used for v
*/
void traverse(vec2 posF, float stepSize, int nSteps, inout float accVal, inout int nSamples){
    // traverse the vectorfield staring at `posF` for `nSteps` using `stepSize` and sample the 
    //noiseColor texture for each position
    // store the accumulated value in `accVal` and the amount of samples in `nSamples`

    vec2 currPos = posF;
    
    for(int i = 0; i < nSteps; i++){
        // normalisera magnitud för riktning
        float currPosVal = texture(noiseTexture, currPos.xy).r;
        accVal += currPosVal;
        nSamples += 1;

        //Integration 
        
        vec2 currPosVecNorm = normalize(texture(inport, currPos.xy).xy); // v(xn)
        
        // Euler
        //currPos += stepSize * currPosVecNorm;

        // Rk4
        vec2 k1 = currPosVecNorm;
        vec2 k2 = normalize(texture(inport, (currPos+k1*stepSize/2.0).xy).xy);
        vec2 k3 = normalize(texture(inport, (currPos+k2*stepSize/2.0).xy).xy);
        vec2 k4 = normalize(texture(inport, (currPos+k3*stepSize).xy).xy);

        currPos += (stepSize/6.0) * (k1 + 2*k2 + 2*k3 + k4);
        
    }
    
}

void main(void) {
    float accVal = texture(noiseTexture, texCoord_.xy).r;
    int nSamples = 1;
    
    //traverse the vector field both forward and backwards to calculate the output color
    traverse(texCoord_.xy, stepSize, nSteps, accVal, nSamples);
    traverse(texCoord_.xy, -stepSize, nSteps, accVal, nSamples);
    
    accVal /= nSamples;
    

    FragData0 = vec4(accVal, accVal, accVal, 1);
}
