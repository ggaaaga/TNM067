#include "utils/structs.glsl"

uniform sampler2D inport;
uniform ImageParameters inportParameters;
uniform ImageParameters outportParameters;

float passThrough(vec2 coord){
    return texture(inport, coord).x;
}

float magnitude( vec2 coord ){
    //TASK 1: find the magnitude of the vectorfield at the position coords

    vec2 velo = texture(inport,coord.xy).xy;
    
    return sqrt(velo.x*velo.x + velo.y*velo.y); // for 3d add + coord.z^2
}

// Helper funcs //

vec2 dVx(vec2 coord, vec2 dist){
    vec2 newCoord1 = vec2(coord.x+dist.x, coord.y);
    vec2 newCoord2 = vec2(coord.x-dist.x, coord.y);

    return (texture(inport,newCoord1.xy).xy - texture(inport,newCoord2.xy).xy)/(2*dist.x);
}

vec2 dVy(vec2 coord, vec2 dist){
    vec2 newCoord1 = vec2(coord.x, coord.y+dist.y);
    vec2 newCoord2 = vec2(coord.x, coord.y-dist.y);

    return (texture(inport,newCoord1.xy).xy - texture(inport,newCoord2.xy).xy)/(2*dist.y);
}
////////////////////

float divergence(vec2 coord){
    //TASK 2: find the divergence of the vectorfield at the position coords
    vec2 pixSz = inportParameters.reciprocalDimensions;

    return dVx(coord,pixSz).x + dVy(coord,pixSz).y; // for 3D add + dVz/dz, nabla*V(x,y,z)
}

float rotation(vec2 coord){
    //TASK 3: find the curl of the vectorfield at the position coords
    vec2 pixSz = inportParameters.reciprocalDimensions;
    return dVy(coord,pixSz).x - dVx(coord,pixSz).y; // 3D would be a 3d vector field bc of cross product operation
}

void main(void) {
    vec2 texCoords = gl_FragCoord.xy * outportParameters.reciprocalDimensions;

    float v = OUTPUT(texCoords);

    FragData0 = vec4(v);
}
