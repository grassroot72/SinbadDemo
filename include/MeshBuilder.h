#ifndef _Demo_MeshBuilder_H_
#define _Demo_MeshBuilder_H_

#include "OgrePrerequisites.h"

namespace Demo
{
  class GrassMesh;

  class MeshBuilder
  {
    static Ogre::IndexBufferPacked* createIndexBuffer( Ogre::VaoManager *vaoManager, void *indices, int sizeI );
    static Ogre::VertexBufferPacked* createVertexBuffer( Ogre::VaoManager *vaoManager, void *vertices, int sizeV );

  public:
    static Ogre::MeshPtr buildGrassMesh( Ogre::VaoManager *vaoManager, GrassMesh *grassMesh );
  };
}

#endif
