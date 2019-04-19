
#include "SinbadCharacterController.h"
#include "SinbadCameraController.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint.h"
#include "Animation/OgreSkeletonAnimation.h"


namespace Demo
{
    SinbadCharacterController::SinbadCharacterController( Ogre::SceneManager *sceneMgr ) :
        mKeyDirection( Ogre::Vector3::ZERO ),
        mVerticalVelocity( 0.0f )
    {
        createBody( sceneMgr );
        createSwords( sceneMgr );
        setupBones( sceneMgr );
        setupAnimations();
    }

    void SinbadCharacterController::createBody( Ogre::SceneManager *sceneMgr )
    {
        // load mesh
        Ogre::MeshManager::getSingleton().load( "Sinbad_v2.mesh",
                                                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        // create main model
        Ogre::SceneNode *sceneNode = sceneMgr->getRootSceneNode()->createChildSceneNode();
        Ogre::Item *item = sceneMgr->createItem( "Sinbad_v2.mesh",
                                     Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                     Ogre::SCENE_DYNAMIC );

        sceneNode->scale( 0.2f, 0.2f, 0.2f );
        sceneNode->setPosition( Ogre::Vector3( 0.0f, CHAR_HEIGHT, 0.0f ) );
        sceneNode->attachObject( item );
        // save them in the member variables
        mBodyItem = item;
        mBodyNode = sceneNode;
    }

    void SinbadCharacterController::createSwords( Ogre::SceneManager *sceneMgr )
    {
        // load mesh
        Ogre::MeshManager::getSingleton().load( "Sword_v2.mesh",
                                                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        Ogre::Item *item;
        // left sword
        item = sceneMgr->createItem( "Sword_v2.mesh",
                                     Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                     Ogre::SCENE_DYNAMIC );
        mSwordLItem = item;

        // right sword
        item = sceneMgr->createItem( "Sword_v2.mesh",
                                     Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                     Ogre::SCENE_DYNAMIC );
        mSwordRItem = item;
    }

    void SinbadCharacterController::setupBones( Ogre::SceneManager *sceneMgr )
    {
        Ogre::TagPoint *tagPoint;
        Ogre::Bone *bone;

        Ogre::SkeletonInstance *skeletonInstance = mBodyItem->getSkeletonInstance();

        // left sheath
        tagPoint = sceneMgr->createTagPoint();
        bone = skeletonInstance->getBone( "Sheath.L" );
        bone->addTagPoint( tagPoint );
        mSheathLNode = tagPoint;

        // right sheath
        tagPoint = sceneMgr->createTagPoint();
        bone = skeletonInstance->getBone( "Sheath.R" );
        bone->addTagPoint( tagPoint );
        mSheathRNode = tagPoint;

        // left handle
        tagPoint = sceneMgr->createTagPoint();
        bone = skeletonInstance->getBone( "Handle.L" );
        bone->addTagPoint( tagPoint );
        mHandleLNode = tagPoint;

        // right handle
        tagPoint = sceneMgr->createTagPoint();
        bone = skeletonInstance->getBone( "Handle.R" );
        bone->addTagPoint( tagPoint );
        mHandleRNode = tagPoint;

        // show the swords, they are in the sheaths at the beginning
        mSheathLNode->attachObject( mSwordLItem );
        mSheathRNode->attachObject( mSwordRItem );
    }

    void SinbadCharacterController::updateBody( Ogre::Real deltaTime )
    {
        if( mCameraController->getCameraMode() == CAM_GOD_MODE )
        {
            return;
        }
        else if( mCameraController->getCameraMode() == CAM_TPS_MODE )
        {
            Ogre::SceneNode *camNode = mCameraController->mCameraNode;

            mGoalDirection = Ogre::Vector3::ZERO;   // we will calculate this

            if( mKeyDirection != Ogre::Vector3::ZERO && mBaseAnimID != ANIM_DANCE )
            {
                // calculate actually goal direction in world based on player's key directions
                mGoalDirection += mKeyDirection.z * camNode->getOrientation().zAxis();
                mGoalDirection += mKeyDirection.x * camNode->getOrientation().xAxis();
                mGoalDirection.y = 0.0f;
                mGoalDirection.normalise();

                Ogre::Quaternion toGoal = mBodyNode->getOrientation().zAxis().getRotationTo( mGoalDirection );

                // calculate how much the character has to turn to face goal direction
                Ogre::Real yawToGoal = toGoal.getYaw().valueDegrees();
                // this is how much the character CAN turn this frame
                Ogre::Real yawAtSpeed = yawToGoal / Ogre::Math::Abs( yawToGoal ) * deltaTime * TURN_SPEED;
                // reduce "turnability" if we're in midair
                if( mBaseAnimID == ANIM_JUMP_LOOP ) yawAtSpeed *= 0.2f;

                // turn as much as we can, but not more than we need to
                if( yawToGoal < 0.0f ) yawToGoal =
                        std::min<Ogre::Real>( 0.0f, std::max<Ogre::Real>( yawToGoal, yawAtSpeed ) ); //yawToGoal = Math::Clamp<Real>(yawToGoal, yawAtSpeed, 0);
                else if( yawToGoal > 0.0f ) yawToGoal =
                        std::max<Ogre::Real>( 0.0f, std::min<Ogre::Real>( yawToGoal, yawAtSpeed ) ); //yawToGoal = Math::Clamp<Real>(yawToGoal, 0, yawAtSpeed);

                mBodyNode->yaw( Ogre::Degree( yawToGoal ) );

                // move in current body direction (not the goal direction)
                mBodyNode->translate( 0.0f, 0.0f, deltaTime * RUN_SPEED * mWeight[mBaseAnimID], Ogre::Node::TS_LOCAL );
            }

            if( mBaseAnimID == ANIM_JUMP_LOOP )
            {
                // if we're jumping, add a vertical offset too, and apply gravity
                mBodyNode->translate( 0.0f, mVerticalVelocity * deltaTime, 0.0f, Ogre::Node::TS_LOCAL );
                mVerticalVelocity -= GRAVITY * deltaTime;

                Ogre::Vector3 pos = mBodyNode->getPosition();
                if( pos.y <= CHAR_HEIGHT )
                {
                    // if we've hit the ground, change to landing state
                    pos.y = CHAR_HEIGHT;
                    mBodyNode->setPosition( pos );

                    setBaseAnimation( ANIM_JUMP_END, true );
                    mTimer = 0.0f;
                }
            }
        }
    }

    void SinbadCharacterController::setupAnimations()
    {
        // animation
        Ogre::SkeletonInstance *skeletonInstance = mBodyItem->getSkeletonInstance();
        skeletonInstance->addAnimationsFromSkeleton(
                    "Sinbad.skeleton", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        Ogre::String animNames[NUM_ANIMS] =
        { "IdleBase", "IdleTop", "RunBase", "RunTop", "HandsClosed", "HandsRelaxed", "DrawSwords",
          "SliceVertical", "SliceHorizontal", "Dance", "JumpStart", "JumpLoop", "JumpEnd" };

        // populate our animation list
        for( int i = 0; i < NUM_ANIMS; i++ )
        {
            mAnims[i] = skeletonInstance->getAnimation( animNames[i] );
            mAnims[i]->setLoop( true );
            mFadingIn[i] = false;
            mFadingOut[i] = false;
        }

        // start off in the idle state (top and bottom together)
        setBaseAnimation( ANIM_IDLE_BASE );
        setTopAnimation( ANIM_IDLE_TOP );

        // don't update these animation !!! it will cause dump
        // they are: ANIM_HANDS_RELAXED and ANIM_HANDS_CLOSED
        // setEnable them is what we can do
        mAnims[ANIM_HANDS_RELAXED]->setEnabled( true );

        // swords are not in hands
        mSwordsDrawn = false;
    }

    void SinbadCharacterController::updateAnimations( Ogre::Real deltaTime )
    {
        Ogre::Real baseAnimSpeed = 1.0f;
        Ogre::Real topAnimSpeed = 1.0f;

        mTimer += deltaTime;

        if( mTopAnimID == ANIM_DRAW_SWORDS )
        {
            // flip the draw swords animation if we need to put it back
            topAnimSpeed = mSwordsDrawn ? -1.0f : 1.0f;

            if( mTimer >= mAnims[mTopAnimID]->getDuration() / 2.0f &&
                mTimer - deltaTime <= mAnims[mTopAnimID]->getDuration() / 2.0f )
            {
                if( !mSwordsDrawn )
                {
                    // take out swords
                    mSheathLNode->detachObject( mSwordLItem );
                    mSheathRNode->detachObject( mSwordRItem );

                    mHandleLNode->attachObject( mSwordLItem );
                    mHandleRNode->attachObject( mSwordRItem );
                }
                else
                {
                    // put back swords
                    mHandleLNode->detachObject( mSwordLItem );
                    mHandleRNode->detachObject( mSwordRItem );

                    mSheathLNode->attachObject( mSwordLItem );
                    mSheathRNode->attachObject( mSwordRItem );
                }

                mAnims[ANIM_HANDS_CLOSED]->setEnabled( !mSwordsDrawn );
                mAnims[ANIM_HANDS_RELAXED]->setEnabled( mSwordsDrawn );
            }

            if( mTimer >= mAnims[mTopAnimID]->getDuration() )
            {
                if( mBaseAnimID == ANIM_IDLE_BASE ) setTopAnimation( ANIM_IDLE_TOP );
                else
                {
                    setTopAnimation( ANIM_RUN_TOP );
                    mAnims[ANIM_RUN_TOP]->setTime( mAnims[ANIM_RUN_BASE]->getCurrentTime() );
                }

                mSwordsDrawn = !mSwordsDrawn;
            }
        }

        else if( mTopAnimID == ANIM_SLICE_VERTICAL || mTopAnimID == ANIM_SLICE_HORIZONTAL )
        {
            if( mTimer >= mAnims[mTopAnimID]->getDuration() )
            {
                // animation is finished, so return to what we were doing before
                if( mBaseAnimID == ANIM_IDLE_BASE ) setTopAnimation( ANIM_IDLE_TOP );
                else
                {
                    setTopAnimation( ANIM_RUN_TOP );
                    mAnims[ANIM_RUN_TOP]->setTime( mAnims[ANIM_RUN_BASE]->getCurrentTime() );
                }
            }

            // don't sway hips from side to side when slicing. that's just embarrassing.
            if( mBaseAnimID == ANIM_IDLE_BASE ) baseAnimSpeed = 0.0f;
        }

        else if( mBaseAnimID == ANIM_JUMP_START )
        {
            if( mTimer >= mAnims[mBaseAnimID]->getDuration() )
            {
                // takeoff animation finished, so time to leave the ground!
                setBaseAnimation( ANIM_JUMP_LOOP, true );
                // apply a jump acceleration to the character
                mVerticalVelocity = JUMP_ACCEL;
            }
        }
        else if( mBaseAnimID == ANIM_JUMP_END )
        {
            if( mTimer >= mAnims[mBaseAnimID]->getDuration() )
            {
                // safely landed, so go back to running or idling
                if( mKeyDirection == Ogre::Vector3::ZERO )
                {
                    setBaseAnimation( ANIM_IDLE_BASE );
                    setTopAnimation( ANIM_IDLE_TOP );
                }
                else
                {
                    setBaseAnimation( ANIM_RUN_BASE, true );
                    setTopAnimation( ANIM_RUN_TOP, true );
                }
            }
        }

        if( mBaseAnimID != ANIM_NONE ) mAnims[mBaseAnimID]->addTime( deltaTime*baseAnimSpeed );
        if( mTopAnimID != ANIM_NONE ) mAnims[mTopAnimID]->addTime( deltaTime*topAnimSpeed );

        // apply smooth transitioning between our animations
        fadeAnimations( deltaTime );
    }

    void SinbadCharacterController::setBaseAnimation( AnimID id, bool reset )
    {
        if( mBaseAnimID >= 0 && mBaseAnimID < NUM_ANIMS )
        {
            // if we have an old animation, fade it out
            mFadingIn[mBaseAnimID] = false;
            mFadingOut[mBaseAnimID] = true;
        }

        mBaseAnimID = id;

        if( id != ANIM_NONE )
        {
            // if we have a new animation, enable it and fade it in
            mAnims[id]->setEnabled( true );
            mWeight[id] = 0.0f;
            mFadingOut[id] = false;
            mFadingIn[id] = true;
            if( reset ) mAnims[id]->setTime( 0.0f );
        }
    }

    void SinbadCharacterController::setTopAnimation( AnimID id, bool reset )
    {
        if( mTopAnimID >= 0 && mTopAnimID < NUM_ANIMS )
        {
            // if we have an old animation, fade it out
            mFadingIn[mTopAnimID] = false;
            mFadingOut[mTopAnimID] = true;
        }

        mTopAnimID = id;

        if( id != ANIM_NONE )
        {
            // if we have a new animation, enable it and fade it in
            mAnims[id]->setEnabled( true );
            mWeight[id] = 0.0f;
            mFadingOut[id] = false;
            mFadingIn[id] = true;
            if( reset ) mAnims[id]->setTime( 0.0f );
        }
    }

    void SinbadCharacterController::fadeAnimations( Ogre::Real deltaTime )
    {
        for( int i = 0; i < NUM_ANIMS; i++ )
        {
            if( mFadingIn[i] )
            {
                // slowly fade this animation in until it has full weight
                Ogre::Real newWeight = mWeight[i] + deltaTime*ANIM_FADE_SPEED;
                mWeight[i] = Ogre::Math::Clamp<Ogre::Real>( newWeight, 0.0f, 1.0f );
                if( newWeight >= 1.0f ) mFadingIn[i] = false;
            }
            else if( mFadingOut[i] )
            {
                // slowly fade this animation out until it has no weight, and then disable it
                Ogre::Real newWeight = mWeight[i] - deltaTime*ANIM_FADE_SPEED;
                mWeight[i] = Ogre::Math::Clamp<Ogre::Real>( newWeight, 0.0f, 1.0f );
                if( newWeight <= 0.0f )
                {
                    mAnims[i]->setEnabled( false );
                    mFadingOut[i] = false;
                }
            }
        }
    }

    void SinbadCharacterController::update( float timeSinceLast )
    {
        updateBody( timeSinceLast );
        updateAnimations( timeSinceLast );
    }

    bool SinbadCharacterController::keyPressed( const SDL_KeyboardEvent &evt )
    {
        if( mCameraController->getCameraMode() == CAM_GOD_MODE )
        {
            return false;
        }
        else if( mCameraController->getCameraMode() == CAM_TPS_MODE )
        {
            SDL_Keycode key = evt.keysym.sym;

            // draw swords
            if( key == 'q' && ( mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP ) )
            {
                // take swords out (or put them back, since it's the same animation but reversed)
                setTopAnimation( ANIM_DRAW_SWORDS, true );
                mTimer = 0.0f;
            }

            // dance
            else if( key == 'e' && !mSwordsDrawn )
            {
                if( mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP )
                {
                    // start dancing
                    setBaseAnimation( ANIM_DANCE, true );
                    setTopAnimation( ANIM_NONE );
                    // disable hand animation because the dance controls hands
                    mAnims[ANIM_HANDS_RELAXED]->setEnabled( false );
                }
                else if( mBaseAnimID == ANIM_DANCE )
                {
                    // stop dancing
                    setBaseAnimation( ANIM_IDLE_BASE );
                    setTopAnimation( ANIM_IDLE_TOP );
                    // re-enable hand animation
                    mAnims[ANIM_HANDS_RELAXED]->setEnabled( true );
                }
            }

            // keep track of the player's intended direction
            else if( key == 'w' ) mKeyDirection.z = -1.0f;
            else if( key == 'a' ) mKeyDirection.x = -1.0f;
            else if( key == 's' ) mKeyDirection.z = 1.0f;
            else if( key == 'd' ) mKeyDirection.x = 1.0f;

            // run
            else if( key == SDLK_SPACE && ( mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP ) )
            {
                // jump if on ground
                setBaseAnimation( ANIM_JUMP_START, true );
                setTopAnimation( ANIM_NONE );
                mTimer = 0.0f;
            }

            if( !mKeyDirection.isZeroLength() && mBaseAnimID == ANIM_IDLE_BASE )
            {
                // start running if not already moving and the player wants to move
                setBaseAnimation( ANIM_RUN_BASE, true );
                if( mTopAnimID == ANIM_IDLE_TOP ) setTopAnimation( ANIM_RUN_TOP, true );
            }
        }

        return true;
    }

    bool SinbadCharacterController::keyReleased( const SDL_KeyboardEvent &evt )
    {
        if( mCameraController->getCameraMode() == CAM_GOD_MODE )
        {
            return false;
        }
        else if( mCameraController->getCameraMode() == CAM_TPS_MODE )
        {
            SDL_Keycode key = evt.keysym.sym;

            // keep track of the player's intended direction
            if( key == 'w' && mKeyDirection.z == -1.0f ) mKeyDirection.z = 0.0f;
            else if( key == 'a' && mKeyDirection.x == -1.0f ) mKeyDirection.x = 0.0f;
            else if( key == 's' && mKeyDirection.z == 1.0f ) mKeyDirection.z = 0.0f;
            else if( key == 'd' && mKeyDirection.x == 1.0f ) mKeyDirection.x = 0.0f;

            if( mKeyDirection.isZeroLength() && mBaseAnimID == ANIM_RUN_BASE )
            {
                // stop running if already moving and the player doesn't want to move
                setBaseAnimation( ANIM_IDLE_BASE );
                if( mTopAnimID == ANIM_RUN_TOP ) setTopAnimation( ANIM_IDLE_TOP );
            }
        }

        return true;
    }

    void SinbadCharacterController::mousePressed( const SDL_MouseButtonEvent &evt, const Ogre::uint8 id )
    {
        if( mSwordsDrawn && ( mTopAnimID == ANIM_IDLE_TOP || mTopAnimID == ANIM_RUN_TOP ) )
        {
            // if swords are out, and character's not doing something weird, then SLICE!
            if ( evt.button == SDL_BUTTON_LEFT ) setTopAnimation( ANIM_SLICE_VERTICAL );
            else if ( evt.button == SDL_BUTTON_RIGHT ) setTopAnimation( ANIM_SLICE_HORIZONTAL );
            mTimer = 0.0f;
        }
    }

    void SinbadCharacterController::hookCameraController( SinbadCameraController *camController )
    {
        mCameraController = camController;
    }

    Ogre::SceneNode* SinbadCharacterController::getBodyNode()
    {
        return mBodyNode;
    }
}
