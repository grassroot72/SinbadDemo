#ifndef _Demo_GrassMesh_H_
#define _Demo_GrassMesh_H_

#include "OgrePrerequisites.h"
#include "OgreMesh2.h"

#define GRASS_WIDTH 2.0f
#define GRASS_HEIGHT 2.0f


namespace Demo
{
	struct GrassVertex
	{
		float px, py, pz;   // Position
		float nx, ny, nz;   // Normals
		float u,  v;        // Texture

		GrassVertex() {}
		GrassVertex( float _px, float _py, float _pz,
								 float _nx, float _ny, float _nz,
								 float _u,  float _v ) :
			px(_px), py(_py), pz(_pz),
			nx(_nx), ny(_ny), nz(_nz),
			u(_u),   v(_v)
		{
		}
	};

	class GrassMesh
	{
		Ogre::Real mRotationTime;

		void fillVertices( GrassVertex *vertices );
		void fillIndices( Ogre::uint16 *indices );

	public:
    GrassVertex *mVertices;
    int mVertexCount;

    Ogre::uint16 *mIndices;
    int mIndexCount;

    GrassMesh( int _vertexCount, int _indexCount );
    ~GrassMesh();

    void update( Ogre::MeshPtr grassMesh, Ogre::Real deltaTime );
	};
}

#endif
