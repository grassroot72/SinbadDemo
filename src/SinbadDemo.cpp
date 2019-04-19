
#include "GraphicsSystem.h"
#include "SinbadDemoGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <pwd.h>
    #include <errno.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    #include "shlobj.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
    class SinbadDemoGraphicsSystem : public GraphicsSystem
    {
    private:
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
            const Ogre::RenderSystemCapabilities *caps = renderSystem->getCapabilities();

            Ogre::String compositorName = "SmaaDistortionWorkspace";
            if( mRenderWindow->getFSAA() > 1u && caps->hasCapability( Ogre::RSC_EXPLICIT_FSAA_RESOLVE ) )
                compositorName = "SinbadDemoWorkspace";

            return compositorManager->addWorkspace( mSceneManager, mRenderWindow, mCamera,
                                                    compositorName, true );
        }

        virtual void setupResources(void)
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( mResourcePath + "resources2.cfg" );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = "./";
            else if( *(originalDataFolder.end() - 1) != '/' )
                originalDataFolder += "/";

            const char *c_locations[13] =
            {
                "Materials/SMAA",
                "Materials/SMAA/GLSL",
                "Materials/SMAA/HLSL",
                "Materials/SMAA/Metal",
                "Materials/Demo",
                "Materials/Demo/floor",
                "Materials/Demo/wall",
                "Materials/Demo/barrel",
                "Materials/Demo/ninja",
                "Materials/Demo/grass",
                "Materials/Demo/sinbad",
                "Materials/Distortion",
                "Particle"
            };

            for( size_t i=0; i<13; ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, "FileSystem", "Popular" );
            }
        }

    public:
        SinbadDemoGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
            mResourcePath = "../Data/";

            //It's recommended that you set this path to:
            //	%APPDATA%/SinbadDemo/ on Windows
            //	~/.config/SinbadDemo/ on Linux
            //	macCachePath() + "/SinbadDemo/" (NSCachesDirectory) on Apple -> Important because
            //	on iOS your app could be rejected from App Store when they see iCloud
            //	trying to backup your Ogre.log & ogre.cfg auto-generated without user
            //	intervention. Also convenient because these settings will be deleted
            //	if the user removes cached data from the app, so the settings will be
            //	reset.

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            mWriteAccessFolder =  + "/";
            TCHAR path[MAX_PATH];
            if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL,
                                            SHGFP_TYPE_CURRENT, path ) != S_OK ) )
            {
                //Need to convert to OEM codepage so that fstream can
                //use it properly on international systems.
        #if defined(_UNICODE) || defined(UNICODE)
                int size_needed = WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                                       NULL, 0, NULL, NULL );
                mWriteAccessFolder = std::string( size_needed, 0 );
                WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                     &mWriteAccessFolder[0], size_needed, NULL, NULL );
        #else
                TCHAR oemPath[MAX_PATH];
                CharToOem( path, oemPath );
                mWriteAccessFolder = std::string( oemPath );
        #endif
                mWriteAccessFolder += "/SinbadDemo/";

                //Attempt to create directory where config files go
                if( !CreateDirectoryA( mWriteAccessFolder.c_str(), NULL ) &&
                    GetLastError() != ERROR_ALREADY_EXISTS )
                {
                    //Couldn't create directory (no write access?),
                    //fall back to current working dir
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            const char *homeDir = getenv("HOME");
            if( homeDir == 0 )
                homeDir = getpwuid( getuid() )->pw_dir;
            mWriteAccessFolder = homeDir;
            mWriteAccessFolder += "/.config";
            int result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            int errorReason = errno;

            //Create "~/.config"
            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
            else
            {
                //Create "~/.config/SinbadDemo"
                mWriteAccessFolder += "/SinbadDemo/";
                result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
                errorReason = errno;

                if( result && errorReason != EEXIST )
                {
                    printf( "Error. Failing to create path '%s'. Do you have access rights?",
                            mWriteAccessFolder.c_str() );
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            mWriteAccessFolder = macCachePath() + "/SinbadDemo/";
            //Create "pathToCache/SinbadDemo"
            mWriteAccessFolder += "/SinbadDemo/";
            result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            errorReason = errno;

            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
#endif
        }
    };



    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                                   GraphicsSystem **outGraphicsSystem,
                                                   GameState **outLogicGameState,
                                                   LogicSystem **outLogicSystem )
    {
        SinbadDemoGameState *gfxGameState = new SinbadDemoGameState( "Sinbad Charactor Demo" );

        GraphicsSystem *graphicsSystem = new SinbadDemoGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                                    GraphicsSystem *graphicsSystem,
                                                    GameState *logicGameState,
                                                    LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Sinbad Charactor Demo";
    }
}
