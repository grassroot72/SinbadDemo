
#include "SinbadHlmsListener.h"
#include "OgreSceneManager.h"


using namespace Demo;

namespace Demo
{
    Ogre::uint32 SinbadHlmsListener::getPassBufferSize( const Ogre::CompositorShadowNode *shadowNode,
                                                        bool casterPass, bool dualParaboloid,
                                                        Ogre::SceneManager *sceneManager ) const
    {
        return 32; // ( vec4 fogParams + vec4 fogColor )*4
    }

    float* SinbadHlmsListener::preparePassBuffer( const Ogre::CompositorShadowNode *shadowNode,
                                                  bool casterPass, bool dualParaboloid,
                                                  Ogre::SceneManager *sceneManager,
                                                  float *passBufferPtr )
    {
        // Fog parameters
        *passBufferPtr++ = sceneManager->getFogStart();
        *passBufferPtr++ = sceneManager->getFogEnd();
        *passBufferPtr++ = sceneManager->getFogDensity();
        *passBufferPtr++ = 0.0f;

        *passBufferPtr++ = sceneManager->getFogColour().r;
        *passBufferPtr++ = sceneManager->getFogColour().g;
        *passBufferPtr++ = sceneManager->getFogColour().b;
        *passBufferPtr++ = 1.0f;

        return passBufferPtr;
    }

}
