
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

#define NUM_PBSDATABLOCKS 20           // number of total pbs datablocks
#define NUM_DISTORTIONSPHERES 5
#define NUM_NINJAS 28
#define NUM_BARRELS 28

namespace Demo
{
    class SinbadCharacterController;
    class SinbadHlmsListener;
    class GrassMesh;

    enum WallOrientations
    {
        SOUTH,
        NORTH,
        WEST,
        EAST,
        NUM_WALLS
    };

    class SinbadDemoGameState : public BasicGameState
    {
    private:
        Ogre::SceneNode *mGroundNode;
        Ogre::SceneNode *mRainNode;
        Ogre::SceneNode *mSnowNode;
        Ogre::SceneNode *mGreenyNimbusNode;
        Ogre::SceneNode *mAureolaNode;
        Ogre::SceneNode *mDistortionSceneNode[NUM_DISTORTIONSPHERES];
        Ogre::SceneNode *mLightNodes[5];

        GrassMesh *mOriginalGrassMesh;
        Ogre::MeshPtr mGrassMesh;

        float mFogExpDensity;
        SinbadHlmsListener *mHlmsListener;

        float mDistortionStrenght;
        Ogre::Pass *mDistortionPass;

        // current light preset
        Ogre::uint32 mCurrentLightPreset;

        int mCurrentCameraMode;
        SinbadCharacterController *mSinbadCharater;


        Ogre::SceneNode *mHeadPivotNode;


        // fake time of the day
        float mFakeTimeOfDay;

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

        // ninjas
        Ogre::SceneNode* mNinjaNode[NUM_NINJAS];
        Ogre::Item* mNinjaItem[NUM_NINJAS];

        btCollisionShape* mNinjaShape[NUM_NINJAS];
        btRigidBody* mNinjaBody[NUM_NINJAS];
        int currentNinjaIndex;


        // barrels
        Ogre::SceneNode* mBarrelNode[NUM_BARRELS];
        Ogre::Item* mBarrelItem[NUM_BARRELS];

        btCollisionShape* mBarrelShape[NUM_BARRELS];
        btRigidBody* mBarrelBody[NUM_BARRELS];
        int currentBarrelIndex;


        // walls
        Ogre::SceneNode* mWallNode[NUM_WALLS];
        Ogre::Item* mWallItem[NUM_WALLS];

        btCollisionShape* mWallShape[NUM_WALLS];
        btRigidBody* mWallBody[NUM_WALLS];


        // introduction message index
        Ogre::uint32 mCurrentIntroMsgIndex;


        // physics related functions
        void setupBulletPhysics();
        void destroyBulletPhysics();

        void phyCreateGround();

        void phyCreateWall( int index );
        void phyCreateWalls();

        void phyCreateNinja( int index );
        void phyCreateNinjas();
        void phyShowOneNinja();

        void phyCreateBarrel( int index );
        void phyCreateBarrels();
        void phyShowOneBarrel();


        // load all meshes
        void loadMeshes();

        void createGround( Ogre::SceneManager *sceneManager );

        void createWall( Ogre::SceneManager *sceneManager, int index, Ogre::Vector3 pos, Ogre::Real yRot );
        void createWalls( Ogre::SceneManager *sceneManager );

        void createNinja( Ogre::SceneManager *sceneManager, int index );
        void createNinjas( Ogre::SceneManager *sceneManager );

        void createBarrel( Ogre::SceneManager *sceneManager, int index );
        void createBarrels( Ogre::SceneManager *sceneManager );

        void createGrass( Ogre::SceneManager *sceneManage );
        void createRain( Ogre::SceneManager *sceneManager );
        void createSnow( Ogre::SceneManager *sceneManager );
        void createGreenyNimbus( Ogre::SceneManager *sceneManager );
        void createAureola( Ogre::SceneManager *sceneManager );
        void createSmokeHead( Ogre::SceneManager *sceneManager );
        void createFog( Ogre::SceneManager *sceneManager );
        void createDistortion( Ogre::SceneManager *sceneManager );

        void createLights( Ogre::SceneManager *sceneManager );


        // resources
        void setBlinnPhong( Ogre::HlmsPbs *hlmsPbs );
        void setShadowConstantBias( Ogre::HlmsPbs *hlmsPbs, float bias );
        void setTransparencyToMaterial( Ogre::HlmsPbs *hlmsPbs, const Ogre::String &datablockName, const float transparenyValue );

        void switchLightPreset( int direction = 1 );
        void switchCameraMode( int direction );
        void changeFog( int direction );
        void changeDistortionStrength( int direction );


        // load shader cache from disk
        virtual void loadShadersFromCache( const Ogre::String &cacheName );
        // save shader cache to disk
        virtual void saveShadersToCache( const Ogre::String &cacheName );


        // debugging
        virtual void generateDebugText(float timeSinceLast, Ogre::String &outText );

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
