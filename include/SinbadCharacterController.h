#ifndef __Demo_SinbadCharacterController_H__
#define __Demo_SinbadCharacterController_H__

#include "OgrePrerequisites.h"
#include "InputListeners.h"
#include "OgreVector3.h"


#define NUM_ANIMS 13            // number of animations the character has
#define CHAR_HEIGHT 1.0f        // height of character's center of mass above ground
#define RUN_SPEED 5.0f          // character running speed in units per second
#define TURN_SPEED 500.0f       // character turning in degrees per second
#define ANIM_FADE_SPEED 80.0f   // animation crossfade speed in % of full weight per second
#define JUMP_ACCEL 12.8f        // character jump acceleration in upward units per squared second
#define GRAVITY 80.0f           // gravity in downward units per squared second

namespace Ogre
{
  class SkeletonAnimation;
}

namespace Demo
{
  class SinbadCameraController;

  class SinbadCharacterController
  {
    // all the animations sinbad character has
    enum AnimID
    {
      ANIM_IDLE_BASE,
      ANIM_IDLE_TOP,
      ANIM_RUN_BASE,
      ANIM_RUN_TOP,
      ANIM_HANDS_CLOSED,
      ANIM_HANDS_RELAXED,
      ANIM_DRAW_SWORDS,
      ANIM_SLICE_VERTICAL,
      ANIM_SLICE_HORIZONTAL,
      ANIM_DANCE,
      ANIM_JUMP_START,
      ANIM_JUMP_LOOP,
      ANIM_JUMP_END,
      ANIM_NONE
    };

    Ogre::SkeletonAnimation *mAnims[NUM_ANIMS];     // master animation list
    AnimID mBaseAnimID;                             // current base (full- or lower-body) animation
    AnimID mTopAnimID;                              // current top (upper-body) animation
    bool mFadingIn[NUM_ANIMS];          // which animations are fading in
    bool mFadingOut[NUM_ANIMS];         // which animations are fading out
    bool mSwordsDrawn;
    Ogre::Real mTimer;                  // general timer to see how long animations have been playing

    Ogre::Vector3 mKeyDirection;        // player's local intended direction based on WASD keys
    Ogre::Vector3 mGoalDirection;       // actual intended direction in world-space
    Ogre::Real mWeight[NUM_ANIMS];      // weight influence on animation
    Ogre::Real mVerticalVelocity;       // for jumping


    Ogre::SceneNode *mBodyNode;
    Ogre::SceneNode *mSheathLNode;
    Ogre::SceneNode *mSheathRNode;
    Ogre::SceneNode *mHandleLNode;
    Ogre::SceneNode *mHandleRNode;
    Ogre::Item *mBodyItem;
    Ogre::Item *mSwordLItem;
    Ogre::Item *mSwordRItem;

    SinbadCameraController *mCameraController;


    void createBody( Ogre::SceneManager *sceneMgr );
    void createSwords( Ogre::SceneManager *sceneMgr );
    void setupBones( Ogre::SceneManager *sceneMgr );

    void setupAnimations();
    void setBaseAnimation( AnimID id, bool reset = false );
    void setTopAnimation( AnimID id, bool reset = false );
    void fadeAnimations( Ogre::Real deltaTime );

    void updateBody( Ogre::Real deltaTime );
    void updateAnimations( Ogre::Real deltaTime );

  public:
    SinbadCharacterController( Ogre::SceneManager *sceneMgr );

    void update( float timeSinceLast );

    bool keyPressed( const SDL_KeyboardEvent &evt );
    bool keyReleased( const SDL_KeyboardEvent &evt );

    void mousePressed( const SDL_MouseButtonEvent &evt, const Ogre::uint8 id );

    void hookCameraController( SinbadCameraController *camController );
    Ogre::SceneNode* getBodyNode();
  };
}

#endif
