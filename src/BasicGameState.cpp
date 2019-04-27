
#include "BasicGameState.h"
#include "SinbadCameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"

#include "OgreOverlayManager.h"
#include "OgreOverlay.h"
#include "OgreOverlayContainer.h"
#include "OgreTextAreaOverlayElement.h"

#include "OgreRoot.h"
#include "OgreFrameStats.h"

#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsCompute.h"
#include "OgreGpuProgramManager.h"

#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"

using namespace Demo;

namespace Demo
{
  BasicGameState::BasicGameState( const Ogre::String &helpDescription ) :
    mGraphicsSystem( 0 ),
    mSinbadCameraController( 0 ),
    mHelpDescription( helpDescription ),
    mDisplayHelpMode( 1 ),
    mNumDisplayHelpModes( 2 ),
    mDebugText( 0 )
  {
  }
  //-----------------------------------------------------------------------------------
  BasicGameState::~BasicGameState()
  {
    delete mSinbadCameraController;
    mSinbadCameraController = 0;
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::_notifyGraphicsSystem( GraphicsSystem *graphicsSystem )
  {
    mGraphicsSystem = graphicsSystem;
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::createScene01(void)
  {
    createDebugTextOverlay();
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::createDebugTextOverlay(void)
  {
    Ogre::v1::OverlayManager &overlayManager = Ogre::v1::OverlayManager::getSingleton();
    Ogre::v1::Overlay *overlay = overlayManager.create( "DebugText" );

    Ogre::v1::OverlayContainer *panel = static_cast<Ogre::v1::OverlayContainer*>(
        overlayManager.createOverlayElement( "Panel", "DebugPanel" ) );
    mDebugText = static_cast<Ogre::v1::TextAreaOverlayElement*>(
                overlayManager.createOverlayElement( "TextArea", "DebugText" ) );
    mDebugText->setFontName( "SentyWEN" );
    mDebugText->setColour( Ogre::ColourValue::Green );
    mDebugText->setCharHeight( 0.09f );
    mDebugText->setPosition( 0.03f, 0.02f );

    mDebugTextShadow= static_cast<Ogre::v1::TextAreaOverlayElement*>(
                overlayManager.createOverlayElement( "TextArea", "0DebugTextShadow" ) );
    mDebugTextShadow->setFontName( "SentyWEN" );
    mDebugTextShadow->setColour( Ogre::ColourValue::Black );
    mDebugTextShadow->setCharHeight( 0.09f );
    mDebugTextShadow->setPosition( 0.032f, 0.022f );

    panel->addChild( mDebugTextShadow );
    panel->addChild( mDebugText );
    overlay->add2D( panel );
    overlay->show();
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
  {
    if( mDisplayHelpMode == 0 )
    {
      outText = mHelpDescription;
      outText += "\n\nPress F1 to toggle help";
      outText += "\n\nProtip: Ctrl+F1 will reload PBS shaders (for real time template editing).\n"
                 "Ctrl+F2 reloads Unlit shaders.\n"
                 "Ctrl+F3 reloads Compute shaders.\n"
                 "Note: If the modified templates produce invalid shader code, "
                 "crashes or exceptions can happen.\n";
      return;
    }

    const Ogre::FrameStats *frameStats = mGraphicsSystem->getRoot()->getFrameStats();

    Ogre::String finalText;
//  finalText.reserve( 128 );
//  finalText  = "Frame time:\t";
//  finalText += Ogre::StringConverter::toString( timeSinceLast * 1000.0f );
//  finalText += " ms\n";
//  finalText += "Frame FPS:\t";
//  finalText += Ogre::StringConverter::toString( 1.0f / timeSinceLast );
//  finalText += "\nAvg time:\t";
//  finalText += Ogre::StringConverter::toString( frameStats->getAvgTime() );
//  finalText += " ms\n";
//  finalText += "Avg FPS:\t";
//  finalText += Ogre::StringConverter::toString( 1000.0f / frameStats->getAvgTime() );
//  finalText += "\n\nPress F1 to toggle help";
    finalText  = "Average FPS:\t";
    finalText += Ogre::StringConverter::toString( 1000.0f / frameStats->getAvgTime() );

    outText.swap( finalText );

    mDebugText->setCaption( finalText );
    mDebugTextShadow->setCaption( finalText );
  }
  //-----------------------------------------------------------------------------------
  Ogre::HlmsPbs* BasicGameState::getHlmsPbs()
  {
    Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

    assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
    Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

    return hlmsPbs;
  }
  //-----------------------------------------------------------------------------------
  Ogre::HlmsUnlit* BasicGameState::getHlmsUnlit()
  {
    Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

    assert( dynamic_cast<Ogre::HlmsUnlit*>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) ) );
    Ogre::HlmsUnlit *hlmsUnlit = static_cast<Ogre::HlmsUnlit*>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );

    return hlmsUnlit;
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::enableDebugScript( bool debugSwitch )
  {
    Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();
    Ogre::HlmsUnlit *hlmsUnlit = getHlmsUnlit();

    // disable spitting glsl files
    hlmsPbs->setDebugOutputPath( debugSwitch, debugSwitch );
    hlmsUnlit->setDebugOutputPath( debugSwitch, debugSwitch );
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::update( float timeSinceLast )
  {
    if( mDisplayHelpMode != 0 )
    {
      //Show FPS
      Ogre::String finalText;
      generateDebugText( timeSinceLast, finalText );
      mDebugText->setCaption( finalText );
      mDebugTextShadow->setCaption( finalText );
    }

    if( mSinbadCameraController )
        mSinbadCameraController->update( timeSinceLast );
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::keyPressed( const SDL_KeyboardEvent &evt  )
  {
    bool handledEvent = false;

    if( mSinbadCameraController )
        handledEvent = mSinbadCameraController->keyPressed( evt );

    if( !handledEvent )
        GameState::keyPressed( evt );
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::keyReleased( const SDL_KeyboardEvent &evt )
  {
    if( evt.keysym.sym == SDLK_F1 && (evt.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) == 0 )
    {
      mDisplayHelpMode = (mDisplayHelpMode + 1) % mNumDisplayHelpModes;

      Ogre::String finalText;
      generateDebugText( 0, finalText );
      mDebugText->setCaption( finalText );
      mDebugTextShadow->setCaption( finalText );
    }
    else if( evt.keysym.sym == SDLK_F1 && (evt.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
    {
      //Hot reload of PBS shaders. We need to clear the microcode cache
      //to prevent using old compiled versions.
      Ogre::Root *root = mGraphicsSystem->getRoot();
      Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

      Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
      Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
      hlms->reloadFrom( hlms->getDataFolder() );
    }
    else if( evt.keysym.sym == SDLK_F2 && (evt.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
    {
      //Hot reload of Unlit shaders.
      Ogre::Root *root = mGraphicsSystem->getRoot();
      Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

      Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_UNLIT );
      Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
      hlms->reloadFrom( hlms->getDataFolder() );
    }
    else if( evt.keysym.sym == SDLK_F3 && (evt.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
    {
      //Hot reload of Unlit shaders.
      Ogre::Root *root = mGraphicsSystem->getRoot();
      Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

      Ogre::Hlms *hlms = hlmsManager->getComputeHlms();
      Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
      hlms->reloadFrom( hlms->getDataFolder() );
    }
    else
    {
      bool handledEvent = false;

      if( mSinbadCameraController )
          handledEvent = mSinbadCameraController->keyReleased( evt );

      if( !handledEvent )
          GameState::keyReleased( evt );
    }
  }
  //-----------------------------------------------------------------------------------
  void BasicGameState::mouseMoved( const SDL_Event &evt )
  {
    if( mSinbadCameraController )
        mSinbadCameraController->mouseMoved( evt );

    GameState::mouseMoved( evt );
  }
}
