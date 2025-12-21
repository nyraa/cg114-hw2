#include <stdint.h>

struct Triangle
{
    float normal[3];
    float vertex1[3];
    float vertex2[3];
    float vertex3[3];
    uint16_t attrByteCount;
};

uint32_t readBinSTL(const char* filename, float** triangles, float** normals);
uint32_t readAsciiSTL(const char* filename, struct Triangle** triangles);