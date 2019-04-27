#include "GrassMesh.h"
#include "OgreMath.h"

#include "OgreSubMesh2.h"


using namespace Demo;

namespace Demo
{
    GrassMesh::GrassMesh( int _vertexCount, int _indexCount ) :
        mVertexCount( _vertexCount ),
        mIndexCount( _indexCount )
    {
        mVertices = reinterpret_cast<GrassVertex*>( OGRE_MALLOC_SIMD( sizeof(GrassVertex) * mVertexCount,
                                                                      Ogre::MEMCATEGORY_GEOMETRY ) );
        mIndices = reinterpret_cast<Ogre::uint16*>( OGRE_MALLOC_SIMD( sizeof(Ogre::uint16) * mIndexCount,
                                                                      Ogre::MEMCATEGORY_GEOMETRY ) );

        GrassVertex vertices[12];
        Ogre::uint16 indices[18];

        fillVertices( vertices );
        fillIndices( indices );

        memcpy( mVertices, vertices, sizeof(GrassVertex) * 12 );
        memcpy( mIndices, indices, sizeof(indices) );
    }

    GrassMesh::~GrassMesh()
    {
        OGRE_FREE_SIMD( mVertices, Ogre::MEMCATEGORY_GEOMETRY );
        OGRE_FREE_SIMD( mIndices, Ogre::MEMCATEGORY_GEOMETRY );
    }

    void GrassMesh::fillVertices( GrassVertex *vertices )
    {
        for( unsigned int i = 0; i < 3; i++ )  // each grass mesh consists of 3 planes
        {
            // planes intersect along the Y axis with 60 degrees between them
            Ogre::Real x = Ogre::Math::Cos( Ogre::Degree( i * 60 ) ) * GRASS_WIDTH / 2.0f;
            Ogre::Real z = Ogre::Math::Sin( Ogre::Degree( i * 60 ) ) * GRASS_WIDTH / 2.0f;

            for( unsigned int j = 0; j < 4; j++ )  // each plane has 4 vertices
            {
                GrassVertex &vertex = vertices[i * 4 + j];

                vertex.px = j < 2 ? -x : x;
                vertex.py = j % 2 ? 0 : GRASS_HEIGHT;
                vertex.pz = j < 2 ? -z : z;

                // all normals point straight up
                vertex.nx = 0;
                vertex.ny = 1;
                vertex.nz = 0;

                vertex.u = j < 2 ? 0 : 1;
                vertex.v = j % 2;
            }
        }
    }

    void GrassMesh::fillIndices( Ogre::uint16 *indices )
    {
        Ogre::uint16 *index = indices;

        for( unsigned int i = 0; i < 3; i++ )   // each grass mesh consists of 3 planes
        {
            unsigned int off = i * 4;           // each plane consists of 2 triangles

            *index++ = 0 + off;
            *index++ = 3 + off;
            *index++ = 1 + off;

            *index++ = 0 + off;
            *index++ = 2 + off;
            *index++ = 3 + off;
        }
    }

    void GrassMesh::update( Ogre::MeshPtr grassMesh, Ogre::Real deltaTime )
    {
        mRotationTime += deltaTime;

        mRotationTime = fmod( mRotationTime, Ogre::Math::PI * 2.0f );

        const float cosAlpha = cosf( mRotationTime ) * 0.2f;
        const float sinAlpha = sinf( mRotationTime ) * 0.2f;

        {
            //Partial update the buffer's vertices.
            Ogre::VertexBufferPacked *partialVertexBuffer = grassMesh->getSubMesh( 0 )->
                    mVao[Ogre::VpNormal][0]->getVertexBuffers()[0];

            GrassVertex newVertex0( mVertices[0] );
            newVertex0.px += cosAlpha;
            //newVertex0.py += sinAlpha;
            newVertex0.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex0, 0, 1 );

            GrassVertex newVertex2( mVertices[2] );
            newVertex2.px += cosAlpha;
            //newVertex2.py += sinAlpha;
            newVertex2.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex2, 2, 1 );

            GrassVertex newVertex4( mVertices[4] );
            newVertex4.px += cosAlpha;
            //newVertex4.py += sinAlpha;
            newVertex4.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex4, 4, 1 );

            GrassVertex newVertex6( mVertices[6] );
            newVertex6.px += cosAlpha;
            //newVertex6.py += sinAlpha;
            newVertex6.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex6, 6, 1 );

            GrassVertex newVertex8( mVertices[8] );
            newVertex8.px += cosAlpha;
            //newVertex8.py += sinAlpha;
            newVertex8.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex8, 8, 1 );

            GrassVertex newVertex10( mVertices[10] );
            newVertex10.px += cosAlpha;
            //newVertex10.py += sinAlpha;
            newVertex10.pz += sinAlpha;
            partialVertexBuffer->upload( &newVertex10, 10, 1 );
        }
    }
}
