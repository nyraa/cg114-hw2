#include "readstl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint32_t readBinSTL(const char* filename, float** triangles, float** normals)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }
    // Skip the 80-byte header
    fseek(file, 80, SEEK_SET);
    
    uint32_t numTriangles;
    fread(&numTriangles, sizeof(uint32_t), 1, file);
    *triangles = (float*)malloc(numTriangles * sizeof(float) * 9);
    *normals = (float*)malloc(numTriangles * sizeof(float) * 3);
    if (!*triangles || !*normals) {
        perror("Failed to allocate memory");
        fclose(file);
        return 0;
    }

    char buffer[50];
    for (uint32_t i = 0; i < numTriangles; i++)
    {
        fread(buffer, 50, 1, file);
        float* tri = &((*triangles)[i * 9]);
        float* norm = &((*normals)[i * 3]);
        memcpy(norm, buffer, 12);
        memcpy(tri, buffer + 12, 36);
    }
    fclose(file);
    return numTriangles;
}

uint32_t readAsciiSTL(const char* filename, struct Triangle** triangles)
{
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    char line[256];
    uint32_t numTriangles = 0;
    uint32_t capacity = 1000; // Initial capacity
    *triangles = (struct Triangle*)malloc(capacity * sizeof(struct Triangle));
    if (!*triangles) {
        perror("Failed to allocate memory");
        fclose(file);
        return 0;
    }

    // read header (solid ...)
    if (!fgets(line, sizeof(line), file)) {
        perror("Failed to read header");
        free(*triangles);
        fclose(file);
        return 0;
    }

    // Token-based parsing: look for 'facet normal' then read three floats,
    // then expect 'outer loop' and three 'vertex' lines each with three floats.
    while (1) {
        char token[32];
        // read next token (e.g., 'facet' or 'endsolid')
        if (fscanf(file, " %31s", token) != 1) break; // EOF

        if (strcmp(token, "endsolid") == 0) {
            // consume rest of line
            if (fgets(line, sizeof(line), file) == NULL) break;
            continue;
        }

        if (strcmp(token, "facet") == 0) {
            // expect 'normal'
            if (fscanf(file, " %31s", token) != 1 || strcmp(token, "normal") != 0) {
                fprintf(stderr, "Expected 'normal' after 'facet'\n");
                free(*triangles);
                fclose(file);
                return 0;
            }
            // read three floats for normal
            if (fscanf(file, " %f %f %f", 
                       &(*triangles)[numTriangles].normal[0],
                       &(*triangles)[numTriangles].normal[1],
                       &(*triangles)[numTriangles].normal[2]) != 3) {
                fprintf(stderr, "Failed to read facet normal floats\n");
                free(*triangles);
                fclose(file);
                return 0;
            }

            // read until end of line
            if (fgets(line, sizeof(line), file) == NULL) {
                fprintf(stderr, "Unexpected EOF after facet normal\n");
                free(*triangles);
                fclose(file);
                return 0;
            }

            // expect 'outer loop'
            if (fscanf(file, " %31s", token) != 1 || strcmp(token, "outer") != 0) {
                fprintf(stderr, "Expected 'outer loop'\n");
                free(*triangles);
                fclose(file);
                return 0;
            }
            if (fscanf(file, " %31s", token) != 1 || strcmp(token, "loop") != 0) {
                fprintf(stderr, "Expected 'loop' after 'outer'\n");
                free(*triangles);
                fclose(file);
                return 0;
            }

            // read three vertex lines
            for (int v = 0; v < 3; v++) {
                if (fscanf(file, " %31s", token) != 1) {
                    fprintf(stderr, "Unexpected EOF when reading vertex\n");
                    free(*triangles);
                    fclose(file);
                    return 0;
                }
                if (strcmp(token, "vertex") != 0) {
                    fprintf(stderr, "Expected 'vertex' token, got '%s'\n", token);
                    free(*triangles);
                    fclose(file);
                    return 0;
                }
                float x, y, z;
                if (fscanf(file, " %f %f %f", &x, &y, &z) != 3) {
                    fprintf(stderr, "Failed to read vertex floats\n");
                    free(*triangles);
                    fclose(file);
                    return 0;
                }
                // assign to the correct vertex array
                if (v == 0) {
                    (*triangles)[numTriangles].vertex1[0] = x;
                    (*triangles)[numTriangles].vertex1[1] = y;
                    (*triangles)[numTriangles].vertex1[2] = z;
                } else if (v == 1) {
                    (*triangles)[numTriangles].vertex2[0] = x;
                    (*triangles)[numTriangles].vertex2[1] = y;
                    (*triangles)[numTriangles].vertex2[2] = z;
                } else {
                    (*triangles)[numTriangles].vertex3[0] = x;
                    (*triangles)[numTriangles].vertex3[1] = y;
                    (*triangles)[numTriangles].vertex3[2] = z;
                }
                // consume rest of line
                if (fgets(line, sizeof(line), file) == NULL) {
                    fprintf(stderr, "Unexpected EOF after vertex\n");
                    free(*triangles);
                    fclose(file);
                    return 0;
                }
            }

            // expect 'endloop' and 'endfacet'
            if (fscanf(file, " %31s", token) != 1 || strcmp(token, "endloop") != 0) {
                fprintf(stderr, "Expected 'endloop'\n");
                free(*triangles);
                fclose(file);
                return 0;
            }
            if (fscanf(file, " %31s", token) != 1 || strcmp(token, "endfacet") != 0) {
                fprintf(stderr, "Expected 'endfacet'\n");
                free(*triangles);
                fclose(file);
                return 0;
            }

            numTriangles++;
            if (numTriangles >= capacity) {
                capacity *= 2;
                *triangles = (struct Triangle*)realloc(*triangles, capacity * sizeof(struct Triangle));
                if (!*triangles) {
                    perror("Failed to reallocate memory");
                    fclose(file);
                    return 0;
                }
            }
        } else {
            // unknown token: skip the rest of the line
            if (fgets(line, sizeof(line), file) == NULL) break;
            continue;
        }
    }
    fclose(file);
    return numTriangles;
}