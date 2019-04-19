
#ifndef _Demo_BasicGameState_H_
#define _Demo_BasicGameState_H_

#include "OgrePrerequisites.h"
#include "GameState.h"

namespace Ogre
{
    namespace v1
    {
        class TextAreaOverlayElement;
    }
    class HlmsPbs;
    class HlmsUnlit;
}

namespace Demo
{
    class GraphicsSystem;
    class SinbadCameraController;

    /// Base game state for the tutorials. All it does is show a little text on screen :)
    class BasicGameState : public GameState
    {
    protected:
        GraphicsSystem *mGraphicsSystem;

        /// Optional, for controlling the camera with WASD and the mouse
        SinbadCameraController *mSinbadCameraController;

        Ogre::String mHelpDescription;
        Ogre::uint16 mDisplayHelpMode;
        Ogre::uint16 mNumDisplayHelpModes;

        Ogre::v1::TextAreaOverlayElement *mDebugText;
        Ogre::v1::TextAreaOverlayElement *mDebugTextShadow;

        virtual void createDebugTextOverlay(void);
        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        // some handy pointers
        Ogre::HlmsPbs* getHlmsPbs();
        Ogre::HlmsUnlit* getHlmsUnlit();
        // enable or disable glsl, hlsl script output on file system
        void enableDebugScript( bool debugSwitch );

    public:
        BasicGameState( const Ogre::String &helpDescription );
        virtual ~BasicGameState();

        void _notifyGraphicsSystem( GraphicsSystem *graphicsSystem );

        virtual void createScene01(void);

        virtual void update( float timeSinceLast );

        virtual void keyPressed( const SDL_KeyboardEvent &evt );
        virtual void keyReleased( const SDL_KeyboardEvent &evt );

        virtual void mouseMoved( const SDL_Event &evt );
    };
}

#endif
