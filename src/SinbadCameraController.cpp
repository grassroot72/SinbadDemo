
#include "SinbadCameraController.h"
#include "SinbadCharacterController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreRenderWindow.h"

#include "OgreCamera.h"


using namespace Demo;

namespace Demo
{
    SinbadCameraController::SinbadCameraController( GraphicsSystem *graphicsSystem ) :
        mSpeedMofifier( false ),
        mCameraYaw( 0 ),
        mCameraPitch( 0 ),
        mCameraBaseSpeed( 5 ),
        mCameraSpeedBoost( 2.5 ),
        mGraphicsSystem( graphicsSystem )
    {
        memset( mWASD, 0, sizeof(mWASD) );
        memset( mSlideUpDown, 0, sizeof(mSlideUpDown) );

        // get root node
        Ogre::SceneNode *rootNode = mGraphicsSystem->getSceneManager()->getRootSceneNode();
        Ogre::Camera *camera = mGraphicsSystem->getCamera();

        mCameraNode = rootNode->createChildSceneNode();
        // create a pivot at roughly the character's shoulder
        mCameraPivotNode = rootNode->createChildSceneNode();
        // this is where the camera should be soon, and it spins around the pivot
        mCameraGoalNode = mCameraPivotNode->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                  Ogre::Vector3( 0.0f, 0.0f, 0.3f ) );

        // by default cameras are attached to the root scene node on creation, so we
        // detach the camera first and then attach to our own scene node
        camera->detachFromParent();
        mCameraNode->attachObject( camera );
    }

    void SinbadCameraController::updateCameraGoal( Ogre::Real deltaYaw,
                                                   Ogre::Real deltaPitch,
                                                   Ogre::Real deltaZoom )
    {
        mCameraPivotNode->yaw( Ogre::Degree( deltaYaw ), Ogre::Node::TS_PARENT );

        // bound the pitch
        if( !( mPivotPitch + deltaPitch > 30.0f && deltaPitch > 0.0f ) &&
            !( mPivotPitch + deltaPitch < -30.0f && deltaPitch < 0.0f ) )
        {
            mCameraPivotNode->pitch( Ogre::Degree( deltaPitch ), Ogre::Node::TS_LOCAL );
            mPivotPitch += deltaPitch;
        }


        // for debugging only ----------
        mCameraGoalNode->_getFullTransformUpdated();
        // for debugging only ----------

        Ogre::Real dist = mCameraGoalNode->_getDerivedPosition().distance( mCameraPivotNode->_getDerivedPosition() );
        Ogre::Real distChange = deltaZoom * dist;

        // bound the zoom
        if( !( dist + distChange < 0.2f && distChange < 0.0f ) &&
            !( dist + distChange > 3.5f && distChange > 0.0f ) )
        {
            mCameraGoalNode->translate( 0.0f, 0.0f, distChange, Ogre::Node::TS_LOCAL );
        }
    }

    void SinbadCameraController::update( Ogre::Real deltaTime )
    {
        if( mCameraMode == CAM_GOD_MODE )
        {
            if( mCameraYaw || mCameraPitch )
            {
                // Update now as yaw needs the derived orientation.
                mCameraNode->_getFullTransformUpdated();
                mCameraNode->yaw( Ogre::Radian( mCameraYaw ), Ogre::Node::TS_WORLD );
                mCameraNode->pitch( Ogre::Radian( mCameraPitch ) );

                mCameraYaw   = 0.0f;
                mCameraPitch = 0.0f;
            }

            int camMovementZ = mWASD[2] - mWASD[0];
            int camMovementX = mWASD[3] - mWASD[1];
            int slideUpDown = mSlideUpDown[0] - mSlideUpDown[1];

            if( camMovementZ || camMovementX || slideUpDown )
            {
                Ogre::Vector3 camMovementDir( camMovementX, slideUpDown, camMovementZ );
                camMovementDir.normalise();
                camMovementDir *= deltaTime * mCameraBaseSpeed * (1 + mSpeedMofifier * mCameraSpeedBoost);

                mCameraNode->translate( camMovementDir, Ogre::Node::TS_LOCAL );
            }
        }
        else if( mCameraMode == CAM_TPS_MODE )
        {
            // place the camera pivot roughly at the character's shoulder
            mCameraPivotNode->setPosition( mCharacterController->getBodyNode()->getPosition() +
                                           Ogre::Vector3::UNIT_Y * CAM_HEIGHT );


            // for debugging only ----------
            mCameraGoalNode->_getFullTransformUpdated();
            // for debugging only ----------

            // move the camera smoothly to the goal
            Ogre::Vector3 goalOffset = mCameraGoalNode->_getDerivedPosition() - mCameraNode->getPosition();
            mCameraNode->translate( goalOffset * deltaTime * 9.0f );

            // always look at the pivot
            mCameraNode->lookAt( mCameraPivotNode->_getDerivedPosition(), Ogre::Node::TS_PARENT );
        }
    }

    void SinbadCameraController::mouseMoved( const SDL_Event &evt )
    {
        if( mCameraMode == CAM_GOD_MODE )
        {
            float width  = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
            float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

            mCameraYaw   += -evt.motion.xrel / width;
            mCameraPitch += -evt.motion.yrel / height;
        }
        else if( mCameraMode == CAM_TPS_MODE )
        {
            if( evt.wheel.y == 1 || evt.wheel.y == -1 )
            {
                // update camera goal based on mouse wheel
                updateCameraGoal( 0.0f, 0.0f, -0.05f*evt.wheel.y );
            }
            else
            {
                // update camera goal based on mouse movement
                updateCameraGoal( -0.05f*evt.motion.xrel, -0.05f*evt.motion.yrel, 0.0f );
            }
        }
    }

    //-----------------------------------------------------------------------------------
    bool SinbadCameraController::keyPressed( const SDL_KeyboardEvent &evt )
    {
        if( mCameraMode == CAM_GOD_MODE )
        {
            SDL_Keycode key = evt.keysym.sym;

            if( key == SDLK_LSHIFT )
                mSpeedMofifier = true;

            if( key == SDLK_w )
                mWASD[0] = true;
            else if( key == SDLK_a )
                mWASD[1] = true;
            else if( key == SDLK_s )
                mWASD[2] = true;
            else if( key == SDLK_d )
                mWASD[3] = true;
            else if( key == SDLK_PAGEUP )
                mSlideUpDown[0] = true;
            else if( key == SDLK_PAGEDOWN )
                mSlideUpDown[1] = true;
            else
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    bool SinbadCameraController::keyReleased( const SDL_KeyboardEvent &evt )
    {
        if( mCameraMode == CAM_GOD_MODE )
        {
            SDL_Keycode key = evt.keysym.sym;

            if( key == SDLK_LSHIFT )
                mSpeedMofifier = false;

            if( key == SDLK_w )
                mWASD[0] = false;
            else if( key == SDLK_a )
                mWASD[1] = false;
            else if( key == SDLK_s )
                mWASD[2] = false;
            else if( key == SDLK_d )
                mWASD[3] = false;
            else if( key == SDLK_PAGEUP )
                mSlideUpDown[0] = false;
            else if( key == SDLK_PAGEDOWN )
                mSlideUpDown[1] = false;
            else
                return false;
        }

        return true;
    }

    int SinbadCameraController::getCameraMode()
    {
        return mCameraMode;
    }

    void SinbadCameraController::setCameraMode( int mode )
    {
        mCameraMode = mode;

        if( mode == CAM_TPS_MODE )
        {
            mCameraPivotNode->setPosition( mCharacterController->getBodyNode()->getPosition() +
                                           Ogre::Vector3::UNIT_Y * CAM_HEIGHT );

            mCameraPivotNode->setFixedYawAxis( true );
            mCameraGoalNode->setFixedYawAxis( true );
            mCameraNode->setFixedYawAxis( true );

            mPivotPitch = 0.0f;
        }
    }

    void SinbadCameraController::hookCharacterController( SinbadCharacterController *characterController )
    {
        mCharacterController = characterController;
    }
}

