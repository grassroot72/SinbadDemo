
#include "GrassMesh.h"
#include "MeshBuilder.h"

#include "OgreMesh.h"
#include "OgreMeshManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "OgreMesh2Serializer.h"

#include "OgreRoot.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"


using namespace Demo;

namespace Demo
{
    Ogre::IndexBufferPacked* MeshBuilder::createIndexBuffer( Ogre::VaoManager *vaoManager,
                                                             void *indices,
                                                             int sizeI )
	{
        Ogre::IndexBufferPacked *indexBuffer = 0;
		try
		{
            indexBuffer = vaoManager->createIndexBuffer( Ogre::IndexBufferPacked::IT_16BIT,
                                                         sizeI,
                                                         Ogre::BT_IMMUTABLE,
                                                         indices, false );
		}
        catch( Ogre::Exception &e )
		{
			// When keepAsShadow = true, the memory will be freed when the index buffer is destroyed.
			// However if for some weird reason there is an exception raised, the memory will
			// not be freed, so it is up to us to do so.
			// The reasons for exceptions are very rare. But we're doing this for correctness.
            OGRE_FREE_SIMD( indexBuffer, Ogre::MEMCATEGORY_GEOMETRY );
			indexBuffer = nullptr;
			throw e;
		}
		return indexBuffer;
	}

    Ogre::VertexBufferPacked* MeshBuilder::createVertexBuffer( Ogre::VaoManager *vaoManager,
                                                               void *vertices,
                                                               int sizeV )
	{
        // Vertex declaration
		Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT3, Ogre::VES_POSITION ) );
        vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT3, Ogre::VES_NORMAL ) );
        vertexElements.push_back( Ogre::VertexElement2( Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES ) );

        Ogre::VertexBufferPacked *vertexBuffer = 0;
		try
		{
			//Create the actual vertex buffer.
            vertexBuffer = vaoManager->createVertexBuffer( vertexElements, sizeV,
                                                           Ogre::BT_DEFAULT,
                                                           vertices, false );
		}
        catch( Ogre::Exception &e )
		{
            OGRE_FREE_SIMD( vertexBuffer, Ogre::MEMCATEGORY_GEOMETRY );
			vertexBuffer = nullptr;
			throw e;
		}
		return vertexBuffer;
	}


    Ogre::MeshPtr MeshBuilder::buildGrassMesh( Ogre::VaoManager *vaoManager, GrassMesh *grassMesh )
    {
        //Create the mesh
        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(
                    "GrassMesh",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        //Create one submesh
        Ogre::SubMesh *subMesh = mesh->createSubMesh();

        Ogre::VertexBufferPacked *vertexBuffer = createVertexBuffer( vaoManager,
                                                                     grassMesh->mVertices,
                                                                     grassMesh->mVertexCount );

        // Now the Vao. We'll just use one vertex buffer source (multi-source not working yet)
        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back( vertexBuffer );
        Ogre::IndexBufferPacked *indexBuffer = createIndexBuffer( vaoManager,
                                                                  grassMesh->mIndices,
                                                                  grassMesh->mIndexCount );

        Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject(
                    vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST );

        // Each Vao pushed to the vector refers to an LOD level.
        // Must be in sync with mesh->mLodValues & mesh->mNumLods if you use more than one level
        subMesh->mVao[Ogre::VpNormal].push_back( vao );
        //Use the same geometry for shadow casting.
        subMesh->mVao[Ogre::VpShadow].push_back( vao );

        // Set the bounds to get frustum culling and LOD to work correctly.
        mesh->_setBounds( Ogre::Aabb( Ogre::Vector3::ZERO, Ogre::Vector3::UNIT_SCALE ), false );
        mesh->_setBoundingSphereRadius( 1.732f );

        return mesh;
    }
}
