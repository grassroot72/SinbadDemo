
#ifndef _Demo_SinbadHlmsListener_H_
#define _Demo_SinbadHlmsListener_H_

#include "OgrePrerequisites.h"
#include "OgreHlmsListener.h"

namespace Ogre
{
    class CompositorShadowNode;
}

namespace Demo
{
    class SinbadHlmsListener : public Ogre::HlmsListener
    {
    public:
        //Comunicate stuffs to the PBS shader
        Ogre::uint32 getPassBufferSize( const Ogre::CompositorShadowNode *shadowNode, bool casterPass,
                                        bool dualParaboloid, Ogre::SceneManager *sceneManager ) const;

        float* preparePassBuffer( const Ogre::CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, Ogre::SceneManager *sceneManager,
                                  float *passBufferPtr );
    };

}

#endif
