#ifndef _Demo_ImguiManager_H_
#define _Demo_ImguiManager_H_

#include "imgui.h"
#include "ImguiRenderable.h"

#include "OgreSingleton.h"
#include "OgreTexture.h"
#include "OgreFastArray.h"
#include "OgreFrameListener.h"


namespace Demo
{
    class ImguiManager : public Ogre::Singleton<ImguiManager>, public Ogre::FrameListener
    {
    public:
        ImguiManager();
        ~ImguiManager();

        //call once before using ImGui
        void init( Ogre::SceneManager* mgr );

        //call every frame before using any Imgui functions
        void newFrame( float deltaTime );
        //call at the end of the frame, the gui gets rendered inside of this function
        void render();

        bool frameRenderingQueued( const Ogre::FrameEvent& evt );


        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static ImguiManager& getSingleton( void );
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static ImguiManager* getSingletonPtr( void );

    private:
        void createFontTexture();
        void createMaterial();

        Ogre::FastArray<ImguiRenderable*> mRenderables;

        Ogre::SceneManager *mSceneMgr;

        Ogre::Pass *mPass;
        Ogre::TexturePtr mFontTex;

        int mLastRenderedFrame;
        bool mFrameEnded;
    };
}

#endif
