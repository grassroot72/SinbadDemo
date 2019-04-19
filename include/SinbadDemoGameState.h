
#ifndef _Demo_SinbadDemoGameState_H_
#define _Demo_SinbadDemoGameState_H_

#include "OgrePrerequisites.h"
#include "BasicGameState.h"
#include "OgreMesh2.h"


class btDynamicsWorld;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;

class btRigidBody;
class btCollisionShape;

#define NUM_DATABLOCKS 13           // number of total datablocks

namespace Demo
{
    class SinbadCharacterController;
    class SinbadHlmsListener;
    class GrassMesh;

    class SinbadDemoGameState : public BasicGameState
    {
    private:
        Ogre::SceneNode *mGroundNode;
        Ogre::SceneNode *mRainNode;
        Ogre::SceneNode *mDistortionSceneNode[10];
        Ogre::SceneNode *mLightNodes[3];

        GrassMesh *mOriginalGrassMesh;
        Ogre::MeshPtr mGrassMesh;

        bool mRainVisible;

        float mFogExpDensity;
        SinbadHlmsListener *mHlmsListener;

        float mDistortionStrenght;
        Ogre::Pass *mDistortionPass;

        Ogre::uint32 mCurrentLightPreset;     // current light preset

        int mCurrentCameraMode;
        SinbadCharacterController *mSinbadCharater;



        // physics engine
        btDynamicsWorld* mPhyWorld;
        btBroadphaseInterface* mBroadphase;
        btDefaultCollisionConfiguration* mCollisionConfig;
        btCollisionDispatcher* mDispatcher;
        btSequentialImpulseConstraintSolver* mSolver;

        // physics objects
        Ogre::Item* mGroundItem;
        btRigidBody* mGroundBody;
        btCollisionShape* mGroundShape;

        Ogre::SceneNode* mSouthWallNode;
        Ogre::Item* mSouthWallItem;
        btRigidBody* mSouthWallBody;
        btCollisionShape* mSouthWallShape;

        Ogre::SceneNode* mNorthWallNode;
        Ogre::Item* mNorthWallItem;
        btRigidBody* mNorthWallBody;
        btCollisionShape* mNorthWallShape;

        Ogre::SceneNode* mWestWallNode;
        Ogre::Item* mWestWallItem;
        btRigidBody* mWestWallBody;
        btCollisionShape* mWestWallShape;

        Ogre::SceneNode* mEastWallNode;
        Ogre::Item* mEastWallItem;
        btRigidBody* mEastWallBody;
        btCollisionShape* mEastWallShape;


        Ogre::SceneNode* mNinjaNode;
        Ogre::Item* mNinjaItem;
        btRigidBody* mNinjaBody;
        btCollisionShape* mNinjaShape;

        Ogre::SceneNode* mBarrelNode;
        Ogre::Item* mBarrelItem;
        btRigidBody* mBarrelBody;
        btCollisionShape* mBarrelShape;

        // physics related functions
        void setupBulletPhysics();
        void destroyBulletPhysics();
        void createWalls( Ogre::SceneManager *sceneManager );
        void createNinja( Ogre::SceneManager *sceneManager );
        void createBarrel( Ogre::SceneManager *sceneManager );


        void createGround( Ogre::SceneManager *sceneManager );
        void createGrass( Ogre::SceneManager *sceneManage );
        void createRain( Ogre::SceneManager *sceneManager );
        void createFog( Ogre::SceneManager *sceneManager );
        void createDistortion( Ogre::SceneManager *sceneManager );
        void createLights( Ogre::SceneManager *sceneManager );

        void setBlinnPhong();
        void setShadowConstantBias();
        void setTransparencyToMaterial( const Ogre::String &datablockName, const float transparenyValue );

        void switchLightPreset( int direction = 1 );
        void switchCameraMode( int direction );
        void changeFog( int direction );
        void changeDistortionStrength( int direction );


        // load shader cache from disk
        virtual void loadShadersFromCache( const Ogre::String &cacheName );
        // save shader cache to disk
        virtual void saveShadersToCache( const Ogre::String &cacheName );


        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        SinbadDemoGameState( const Ogre::String &helpDescription );

        virtual void createScene01( void );
        virtual void destroyScene( void );

        virtual void update( float timeSinceLast );

        virtual void keyPressed( const SDL_KeyboardEvent &evt );
        virtual void keyReleased( const SDL_KeyboardEvent &evt );

        virtual void mouseMoved( const SDL_Event &evt );
        virtual void mousePressed( const SDL_MouseButtonEvent &evt, const Ogre::uint8 id );
    };
}

#endif
