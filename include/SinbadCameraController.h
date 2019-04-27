#ifndef __Demo_SinbadCameraController_H__
#define __Demo_SinbadCameraController_H__

#include "OgrePrerequisites.h"
#include "GraphicsSystem.h"


#define CAM_HEIGHT 0.2f         // height of camera above character's center of mass

#define CAM_GOD_MODE 0
#define CAM_TPS_MODE 1
#define NUM_CAM_MODES 2

namespace Demo
{
  class SinbadCharacterController;

  class SinbadCameraController
  {
    int mCameraMode;

    bool mSpeedMofifier;
    bool mWASD[4];
    bool mSlideUpDown[2];
    float mCameraYaw;
    float mCameraPitch;
    public: float mCameraBaseSpeed;
    public: float mCameraSpeedBoost;


    Ogre::SceneNode *mCameraPivotNode;
    Ogre::SceneNode *mCameraGoalNode;
    Ogre::SceneNode *mCameraNode;
    Ogre::Real mPivotPitch;


    SinbadCharacterController *mCharacterController;    // 3rd person camera character

    void updateCameraGoal( Ogre::Real deltaYaw, Ogre::Real deltaPitch, Ogre::Real deltaZoom );

  private:
    GraphicsSystem *mGraphicsSystem;

  public:
    SinbadCameraController( GraphicsSystem *graphicsSystem );

    void update( Ogre::Real deltaTime );

    bool keyPressed( const SDL_KeyboardEvent &evt );
    bool keyReleased( const SDL_KeyboardEvent &evt );

    void mouseMoved( const SDL_Event &evt );

    int getCameraMode();
    void setCameraMode( int mode );

    void hookCharacterController( SinbadCharacterController *characterController );
  };
}


#endif
