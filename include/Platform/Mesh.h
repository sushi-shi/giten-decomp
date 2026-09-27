#ifndef GITEN_PLATFORM_MESH_H
#define GITEN_PLATFORM_MESH_H

#include <Ints.h>
#include <Platform/Direct3D.h>

// A transformed-and-lit vertex batch drawn as indexed triangles.
struct Mesh {
    D3DTLVERTEX* vertices;
    u16* indices;
    i32 vertexCount;
    i32 indexCount;
};

void BuildQuadMesh(Mesh* mesh);
void FreeMesh(Mesh* mesh);
void BuildRoomMesh(Mesh* mesh, i32 cols, i32 rows);
void AllocWallMesh(Mesh* mesh);
void RestoreTextures(void);

#endif // GITEN_PLATFORM_MESH_H
