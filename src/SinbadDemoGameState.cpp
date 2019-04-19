
#include "SinbadDemoGameState.h"
#include "SinbadCharacterController.h"
#include "SinbadCameraController.h"
#include "GrassMesh.h"
#include "MeshBuilder.h"
#include "SinbadHlmsListener.h"
#include "GraphicsSystem.h"
#include "OgreGpuProgramManager.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "Utils/MeshUtils.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "Utils/SmaaUtils.h"

#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgrePass.h"

#include "OgreParticleSystem.h"

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "btBulletDynamicsCommon.h"


using namespace Demo;

namespace Demo
{
    SinbadDemoGameState::SinbadDemoGameState( const Ogre::String &helpDescription ) :
        BasicGameState( helpDescription ),
        mRainVisible( true ),
        mFogExpDensity( 0.01f ),
        mDistortionStrenght( 1.0f ),
        mCurrentCameraMode( CAM_TPS_MODE )
    {
    }

    void SinbadDemoGameState::createScene01( void )
    {
        // load shader cache
        loadShadersFromCache( "SinbadDemo" );

        SmaaUtils::initialize( mGraphicsSystem->getRoot()->getRenderSystem(),
                               SmaaUtils::SMAA_PRESET_ULTRA,
                               SmaaUtils::EdgeDetectionColour );

        enableDebugScript( false );

        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();
        hlmsPbs->setShadowSettings( Ogre::HlmsPbs::PCF_2x2 );
        //setBlinnPhong();

        // do some global shadow adjustment
        //sceneManager->setShadowFarDistance( 250.0f );
        // shadow adjustment to remove acne, default value is 0.01f
        //setShadowConstantBias();
        // set some material to transparent
        setTransparencyToMaterial( "Sinbad/Ruby", 0.95f );

        // scene manager
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        // physics: should be setup before creating physics objects
        setupBulletPhysics();

        // ground
        createGround( sceneManager );
        createWalls( sceneManager );
        createNinja( sceneManager );
        createBarrel( sceneManager );

        // grass
        createGrass( sceneManager );

        // create particle
        createRain( sceneManager );

        // create & set fog data
        createFog( sceneManager );

        // create the distortion sphere
        createDistortion( sceneManager );

        // create all the 5 lights
        createLights( sceneManager );
        // light preset 1 - ( on, off, off )
        switchLightPreset();

        // camera
        Ogre::Camera *cam = mGraphicsSystem->getCamera();
        cam->setPosition( Ogre::Vector3( 0.0f, 1.6f, 2.2f ) );
        cam->lookAt( Ogre::Vector3( 0.0f, 0.5f, 0.0f ) );
        // our model is quite small, so reduce the clipping planes
        //cam->setNearClipDistance( 0.1f );
        cam->setFarClipDistance( 50.0f );

        // sinbad - must create after the camera created
        mSinbadCameraController = new SinbadCameraController( mGraphicsSystem );
        mSinbadCharater = new SinbadCharacterController( sceneManager );

        mSinbadCameraController->hookCharacterController( mSinbadCharater );
        mSinbadCharater->hookCameraController( mSinbadCameraController );
        mSinbadCameraController->setCameraMode( CAM_TPS_MODE );

        BasicGameState::createScene01();
    }

    void SinbadDemoGameState::destroyScene()
    {
        saveShadersToCache( "SinbadDemo" );

        destroyBulletPhysics();

        delete mSinbadCharater;

        delete mHlmsListener;

        delete mOriginalGrassMesh;

        //If we don't do this, the smart pointers will try to
        //delete memory after Ogre has shutdown (and crash)
        mGrassMesh.setNull();
    }

    void SinbadDemoGameState::loadShadersFromCache( const Ogre::String &cacheName )
    {
        Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache( true );
        try
        {
            Ogre::String cacheFile = "./" + cacheName + ".cache";
            Ogre::DataStreamPtr shaderCacheFile = Ogre::Root::getSingleton().openFileStream( cacheFile );
            Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache( shaderCacheFile );
        }
        catch( std::exception& ) {}
    }

    void SinbadDemoGameState::saveShadersToCache( const Ogre::String &cacheName )
    {
        if( Ogre::GpuProgramManager::getSingleton().isCacheDirty() )
        {
            Ogre::String cacheFile = "./" + cacheName + ".cache";
            Ogre::DataStreamPtr shaderCacheFile = Ogre::Root::getSingleton().createFileStream( cacheFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
            Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache( shaderCacheFile );
        }
    }

    void SinbadDemoGameState::setupBulletPhysics()
    {
        //Initialize the Bullet Physics engine
        mBroadphase = new btDbvtBroadphase;
        mCollisionConfig = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher( mCollisionConfig );
        mSolver = new btSequentialImpulseConstraintSolver();
        mPhyWorld = new btDiscreteDynamicsWorld( mDispatcher, mBroadphase, mSolver, mCollisionConfig );
        mPhyWorld->setGravity( btVector3( 0.0f, -9.8f, 0.0f ) );
    }

    void SinbadDemoGameState::destroyBulletPhysics()
    {
        //Free rigid bodies
        mPhyWorld->removeRigidBody( mNinjaBody );
        delete mNinjaBody->getMotionState();
        delete mNinjaBody;
        delete mNinjaShape;

        mPhyWorld->removeRigidBody( mBarrelBody );
        delete mBarrelBody->getMotionState();
        delete mBarrelBody;
        delete mBarrelShape;

        mPhyWorld->removeRigidBody( mGroundBody );
        delete mGroundBody->getMotionState();
        delete mGroundBody;
        //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
        delete reinterpret_cast<btTriangleMeshShape*>(mGroundShape)->getMeshInterface();
        delete mGroundShape;

        mPhyWorld->removeRigidBody( mSouthWallBody );
        delete mSouthWallBody->getMotionState();
        delete mSouthWallBody;
        //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
        delete reinterpret_cast<btTriangleMeshShape*>(mSouthWallShape)->getMeshInterface();
        delete mSouthWallShape;

        mPhyWorld->removeRigidBody( mNorthWallBody );
        delete mNorthWallBody->getMotionState();
        delete mNorthWallBody;
        //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
        delete reinterpret_cast<btTriangleMeshShape*>(mNorthWallShape)->getMeshInterface();
        delete mNorthWallShape;

        mPhyWorld->removeRigidBody( mWestWallBody );
        delete mWestWallBody->getMotionState();
        delete mWestWallBody;
        //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
        delete reinterpret_cast<btTriangleMeshShape*>(mWestWallShape)->getMeshInterface();
        delete mWestWallShape;

        mPhyWorld->removeRigidBody( mEastWallBody );
        delete mEastWallBody->getMotionState();
        delete mEastWallBody;
        //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
        delete reinterpret_cast<btTriangleMeshShape*>(mEastWallShape)->getMeshInterface();
        delete mEastWallShape;

        delete mPhyWorld;

        delete mSolver;
        delete mDispatcher;
        delete mCollisionConfig;
        delete mBroadphase;
    }

    void SinbadDemoGameState::createGround( Ogre::SceneManager *sceneManager )
    {
        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        // get hlmsPbs datablock
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->getDatablock( "demo_floor" ) );

        for( size_t i=Ogre::PBSM_DIFFUSE; i<=Ogre::PBSM_ROUGHNESS; ++i )
        {
            Ogre::HlmsSamplerblock samplerblock;
            samplerblock.mU = Ogre::TAM_WRAP;
            samplerblock.mV = Ogre::TAM_WRAP;
            datablock->setSamplerblock( static_cast<Ogre::PbsTextureTypes>(i), samplerblock );
        }

        Ogre::v1::MeshManager::getSingleton().createPlane(
                    "DemoFloor",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ),
                    30.0f, 30.0f, 1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                    Ogre::v1::HardwareBuffer::HBU_STATIC,
                    Ogre::v1::HardwareBuffer::HBU_STATIC );

        MeshUtils::importV1Mesh( "DemoFloor", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        Ogre::Item *item;
        item = sceneManager->createItem(
                    "DemoFloor",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_STATIC );
        item->setDatablock( "demo_floor" );
        item->setCastShadows( false );

        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->setPosition( Ogre::Vector3( 0.0f, -1.0f, 0.0f ) );
        sceneNode->attachObject( item );

        mGroundItem = item;
        mGroundNode = sceneNode;


        // create trimesh for physics
        BtOgre::StaticMeshToShapeConverter converter( mGroundItem );
        mGroundShape = converter.createTrimesh();


        // Create MotionState (no need for BtOgre here, you can use it if you want to though).
        const auto groundState = new btDefaultMotionState(
                    btTransform( btQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ), btVector3( 0.0f, -1.0f, 0.0f ) ) );

        //auto groundState = new BtOgre::RigidBodyState( mGroundNode );

        //Create the Body.
        mGroundBody = new btRigidBody( 0, groundState, mGroundShape, btVector3( 0.0f, 0.0f, 0.0f ) );
        mPhyWorld->addRigidBody( mGroundBody );
    }

    void SinbadDemoGameState::createWalls( Ogre::SceneManager *sceneManager )
    {
        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        // get hlmsPbs datablock
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->getDatablock( "demo_wall" ) );

        for( size_t i=Ogre::PBSM_DIFFUSE; i<=Ogre::PBSM_ROUGHNESS; ++i )
        {
            Ogre::HlmsSamplerblock samplerblock;
            samplerblock.mU = Ogre::TAM_WRAP;
            samplerblock.mV = Ogre::TAM_WRAP;
            datablock->setSamplerblock( static_cast<Ogre::PbsTextureTypes>(i), samplerblock );
        }

        Ogre::v1::MeshManager::getSingleton().createPlane(
                    "DemoWall",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::Plane( Ogre::Vector3::UNIT_Z, 1.0f ),
                    30.0f, 2.0f, 1, 1, true, 1, 15.0f, 1.0f, Ogre::Vector3::UNIT_Y,
                    Ogre::v1::HardwareBuffer::HBU_STATIC,
                    Ogre::v1::HardwareBuffer::HBU_STATIC );

        MeshUtils::importV1Mesh( "DemoWall", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        Ogre::SceneNode *sceneNode;
        Ogre::Item *item;


        // south wall
        item = sceneManager->createItem(
                    "DemoWall",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_STATIC );
        item->setDatablock( "demo_wall" );
        item->setCastShadows( false );

        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->yaw( Ogre::Degree( 180.0f ) );
        sceneNode->setPosition( Ogre::Vector3( 0.0f, 1.0f, 16.0f ) );
        sceneNode->attachObject( item );

        mSouthWallItem = item;
        mSouthWallNode = sceneNode;

        BtOgre::StaticMeshToShapeConverter converter1( mSouthWallItem );
        mSouthWallShape = converter1.createTrimesh();

        auto southWallState = new BtOgre::RigidBodyState( mSouthWallNode );
        mSouthWallBody = new btRigidBody( 0, southWallState, mSouthWallShape, btVector3( 0.0f, 0.0f, 0.0f ) );
        mPhyWorld->addRigidBody( mSouthWallBody );


        // north wall
        item = sceneManager->createItem(
                    "DemoWall",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_STATIC );
        item->setDatablock( "demo_wall" );
        item->setCastShadows( false );

        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->setPosition( Ogre::Vector3( 0.0f, 1.0f, -16.0f ) );
        sceneNode->attachObject( item );

        mNorthWallItem = item;
        mNorthWallNode = sceneNode;

        BtOgre::StaticMeshToShapeConverter converter2( mNorthWallItem );
        mNorthWallShape = converter2.createTrimesh();

        auto northWallState = new BtOgre::RigidBodyState( mNorthWallNode );
        mNorthWallBody = new btRigidBody( 0, northWallState, mNorthWallShape, btVector3( 0.0f, 0.0f, 0.0f ) );
        mPhyWorld->addRigidBody( mNorthWallBody );


        // west wall
        item = sceneManager->createItem(
                    "DemoWall",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_STATIC );
        item->setDatablock( "demo_wall" );
        item->setCastShadows( false );

        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->yaw( Ogre::Degree( -90.0f ) );
        sceneNode->setPosition( Ogre::Vector3( 16.0f, 1.0f, 0.0f ) );
        sceneNode->attachObject( item );

        mWestWallItem = item;
        mWestWallNode = sceneNode;

        BtOgre::StaticMeshToShapeConverter converter3( mWestWallItem );
        mWestWallShape = converter3.createTrimesh();

        auto westhWallState = new BtOgre::RigidBodyState( mWestWallNode );
        mWestWallBody = new btRigidBody( 0, westhWallState, mWestWallShape, btVector3( 0.0f, 0.0f, 0.0f ) );
        mPhyWorld->addRigidBody( mWestWallBody );


        // east wall
        item = sceneManager->createItem(
                    "DemoWall",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_STATIC );
        item->setDatablock( "demo_wall" );
        item->setCastShadows( false );

        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        sceneNode->yaw( Ogre::Degree( 90.0f ) );
        sceneNode->setPosition( Ogre::Vector3( -16.0f, 1.0f, 0.0f ) );
        sceneNode->attachObject( item );

        mEastWallItem = item;
        mEastWallNode = sceneNode;

        BtOgre::StaticMeshToShapeConverter converter4( mEastWallItem );
        mEastWallShape = converter4.createTrimesh();

        auto EastWallState = new BtOgre::RigidBodyState( mEastWallNode );
        mEastWallBody = new btRigidBody( 0, EastWallState, mEastWallShape, btVector3( 0.0f, 0.0f, 0.0f ) );
        mPhyWorld->addRigidBody( mEastWallBody );
    }

    void SinbadDemoGameState::createNinja( Ogre::SceneManager *sceneManager )
    {
        // mesh - Ninja player
        MeshUtils::importV1Mesh( "Player.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        Ogre::MeshManager::getSingleton().load( "Player.mesh",
                                                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );


        // sceneNode & item
        Ogre::SceneNode *sceneNode;
        Ogre::Item *item;

        item = sceneManager->createItem(
                    "Player.mesh",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode(
                    Ogre::SCENE_DYNAMIC,
                    Ogre::Vector3( -1.8f, 18.0f, -1.0f ),
                    Ogre::Quaternion( 0.5f, 0.3f, 0.2f, 0.6f ) );

        sceneNode->setScale( 0.6f, 0.6f, 0.6f );
        sceneNode->attachObject( item );

        mNinjaItem = item;
        mNinjaNode = sceneNode;


        // physics
        btScalar mass = 5.0f;
        btVector3 inertia;

        // create shape.
        BtOgre::StaticMeshToShapeConverter converter( mNinjaItem );
        mNinjaShape = converter.createSphere();

        // calculate inertia.
        mNinjaShape->calculateLocalInertia( mass, inertia );

        // create BtOgre MotionState (connects Ogre and Bullet).
        auto ninjaState = new BtOgre::RigidBodyState( mNinjaNode );

        // create the Body.
        mNinjaBody = new btRigidBody( mass, ninjaState, mNinjaShape, inertia );
        mPhyWorld->addRigidBody( mNinjaBody );
    }

    void SinbadDemoGameState::createBarrel( Ogre::SceneManager *sceneManager )
    {
        // mesh - RustyBarrel
        MeshUtils::importV1Mesh( "Barrel.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        Ogre::MeshManager::getSingleton().load( "Barrel.mesh",
                                                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );


        // sceneNode & item
        Ogre::SceneNode *sceneNode;
        Ogre::Item *item;

        item = sceneManager->createItem(
                    "Barrel.mesh",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode(
                    Ogre::SCENE_DYNAMIC,
                    Ogre::Vector3( -1.8f, 8.0f, -1.0f ),
                    Ogre::Quaternion( 0.6f, 0.3f, 0.2f, 0.8f ) );

        sceneNode->setScale( 0.18f, 0.18f, 0.18f );
        sceneNode->attachObject( item );

        mBarrelItem = item;
        mBarrelNode = sceneNode;


        // physics
        btScalar mass = 6.0f;
        btVector3 inertia;

        // create shape
        BtOgre::StaticMeshToShapeConverter converter2( mBarrelItem );
        mBarrelShape = converter2.createCylinder();

        // calculate inertia.
        mBarrelShape->calculateLocalInertia( mass, inertia );

        // create BtOgre MotionState (connects Ogre and Bullet).
        auto barrelState = new BtOgre::RigidBodyState( mBarrelNode );

        // create the Body.
        mBarrelBody = new btRigidBody( mass, barrelState, mBarrelShape, inertia );
        mPhyWorld->addRigidBody( mBarrelBody );
    }

    void SinbadDemoGameState::createGrass( Ogre::SceneManager *sceneManager )
    {
        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->getDatablock( "grass" ) );

        // Create proper blend block for distortion objects
        Ogre::HlmsBlendblock blendBlock = Ogre::HlmsBlendblock();
        blendBlock.mIsTransparent = true;
        blendBlock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
        blendBlock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

        datablock->setBlendblock( blendBlock );

        // Create macro block to disable depth write
        Ogre::HlmsMacroblock macroBlock = Ogre::HlmsMacroblock();
        macroBlock.mDepthWrite = false;
        macroBlock.mDepthCheck = true;
        macroBlock.mCullMode = Ogre::CULL_NONE;

        datablock->setMacroblock( macroBlock );


        Ogre::VaoManager *vaoManager = mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager();
        Ogre::SceneNode *sceneNode;
        Ogre::Item *item;

        mOriginalGrassMesh = new GrassMesh( 12, 18 );
        mGrassMesh = MeshBuilder::buildGrassMesh( vaoManager, mOriginalGrassMesh );

        for( int i = 0; i<50; i++ )
        {
            item = sceneManager->createItem( mGrassMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "grass" );
            item->setCastShadows( false );
            sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->attachObject( item );
            sceneNode->setPosition( Ogre::Math::RangeRandom( -12.0f, 12.0f ), 0.0, Ogre::Math::RangeRandom( -12.0f, 12.0f ) );
        }
    }

    void SinbadDemoGameState::createRain( Ogre::SceneManager *sceneManager )
    {
        Ogre::ParticleSystem *particleSystem = sceneManager->createParticleSystem( "Examples/Rain" );
        //particleSystem->fastForward( 8 );
        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( particleSystem );

        mRainNode = sceneNode;
    }

    void SinbadDemoGameState::createFog( Ogre::SceneManager *sceneManager )
    {
        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        sceneManager->setFog( Ogre::FOG_EXP2,
                              Ogre::ColourValue( 0.5f, 0.5f, 0.5f, 1.0f ), mFogExpDensity, 8.0f, 16.0f );
        // set the customized HlmsListener - for exponent or linear fog
        mHlmsListener = new SinbadHlmsListener();
        hlmsPbs->setListener( mHlmsListener );
    }

    void SinbadDemoGameState::createDistortion( Ogre::SceneManager *sceneManager )
    {
        Ogre::HlmsUnlit *hlmsUnlit = getHlmsUnlit();

        Ogre::HlmsTextureManager *hlmsTextureManager = mGraphicsSystem->getRoot()->getHlmsManager()->getTextureManager();
        Ogre::SceneNode *rootSceneNode = sceneManager->getRootSceneNode();

        // Distortion part

        // We will create varous items that are used to distort the scene in postprocessing.
        // You can use whatever objects you want to (fe. particle effect billboards),
        // but we are going to create just simple spheres.
        // Lets setup a new render queue for distortion pass. Set ID 6 to be our distortion queue
        sceneManager->getRenderQueue()->setRenderQueueMode( 6, Ogre::RenderQueue::FAST );

        for( int i=0; i<8; ++i )
        {
            // Next we need to setup materials and items:
            // - We will be using unlit material with displacement texture.
            // - Distortion objects will be rendered in their own render queue to a separate texture.
            // - See distortion compositor and shaders for more information

            // Create proper blend block for distortion objects
            Ogre::HlmsBlendblock blendBlock = Ogre::HlmsBlendblock();
            blendBlock.mIsTransparent = true;
            blendBlock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
            blendBlock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;

            // Create macro block to disable depth write
            Ogre::HlmsMacroblock macroBlock = Ogre::HlmsMacroblock();
            macroBlock.mDepthWrite = false;
            macroBlock.mDepthCheck = true;

            Ogre::String datablockName = "DistMat" + Ogre::StringConverter::toString( i );
            Ogre::HlmsUnlitDatablock *datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
                        hlmsUnlit->createDatablock( datablockName,
                                                    datablockName,
                                                    macroBlock,
                                                    blendBlock,
                                                    Ogre::HlmsParamVec() ) );

            // Use non-color data as texture type because distortion is stored as x,y vectors to texture.
            Ogre::HlmsTextureManager::TextureLocation texLocation = hlmsTextureManager->
                    createOrRetrieveTexture( "distort_deriv.png",
                                             Ogre::HlmsTextureManager::TEXTURE_TYPE_NON_COLOR_DATA );

            datablock->setTexture( 0, texLocation.xIdx, texLocation.texture );

            // Set material to use vertex colors.
            // Vertex colors are used to control distortion intensity (alpha value)
            datablock->setUseColour( true );
            // Random alpha value for objects. Alpha value is multiplier for distortion strenght
            datablock->setColour( Ogre::ColourValue( 1.0f, 1.0f, 1.0f, Ogre::Math::RangeRandom( 0.35f, 0.5f ) ) );

            Ogre::String meshName = "Sphere1000.mesh";

            Ogre::Item *sphereItem = sceneManager->createItem(
                        meshName,
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        Ogre::SCENE_DYNAMIC );

            sphereItem->setDatablock( datablock );
            sphereItem->setCastShadows( false );

            // Set item to be rendered in distortion queue pass (ID 6)
            sphereItem->setRenderQueueGroup( 6 );

            mDistortionSceneNode[i] = rootSceneNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );

            // Lets stack distortion objects to same position with little variation
            mDistortionSceneNode[i]->setPosition( Ogre::Math::RangeRandom(  1.0f, 5.0f ),
                                                  Ogre::Math::RangeRandom(  0.0f, 5.0f ),
                                                  Ogre::Math::RangeRandom( -3.0f, -8.0f ) );

            float scale = Ogre::Math::RangeRandom( 3.0f, 5.0f );

            mDistortionSceneNode[i]->setScale( scale, scale, scale );
            mDistortionSceneNode[i]->roll( Ogre::Radian( (Ogre::Real)i ) );
            mDistortionSceneNode[i]->attachObject( sphereItem );
        }

        // Receive distortion material and set strenght uniform
        Ogre::MaterialPtr materialDistortion = Ogre::MaterialManager::getSingleton().load(
                    "Distortion/Quad",
                    Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

        Ogre::Pass *passDist = materialDistortion->getTechnique( 0 )->getPass( 0 );
        mDistortionPass = passDist;
        Ogre::GpuProgramParametersSharedPtr psParams = passDist->getFragmentProgramParameters();
        psParams->setNamedConstant( "u_DistortionStrenght", mDistortionStrenght );
    }

    void SinbadDemoGameState::createLights( Ogre::SceneManager *sceneManager )
    {
        Ogre::SceneNode *rootSceneNode = sceneManager->getRootSceneNode();
        Ogre::SceneNode *sceneNode;

        // lights & shadow
        Ogre::Light *light;

        // directional light
        light = sceneManager->createLight();
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDiffuseColour( 1.0f, 1.0f, 1.0f );
        light->setSpecularColour( 1.0f, 1.0f, 1.0f );
        //light->setPowerScale( 1.0f );

        sceneNode = rootSceneNode->createChildSceneNode();
        sceneNode->attachObject( light );
        light->setDirection( Ogre::Vector3( -1.0f, -1.0f, -1.0f ).normalisedCopy() );

        mLightNodes[0] = sceneNode;

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       Ogre::Vector3::UNIT_Y * 0.2f - light->getDirection() );

        // spot light 1
        light = sceneManager->createLight();
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f ); // warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        //light->setPowerScale( Ogre::Math::PI );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        sceneNode = rootSceneNode->createChildSceneNode();
        sceneNode->setPosition( -10.0f, 10.0f, 10.0f );
        sceneNode->attachObject( light );
        light->setDirection( Ogre::Vector3( 1.0f, -1.0f, -1.0f ).normalisedCopy() );

        mLightNodes[1] = sceneNode;

        // spot light 2
        light = sceneManager->createLight();
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f ); // cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        //light->setPowerScale( Ogre::Math::PI );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        sceneNode = rootSceneNode->createChildSceneNode();
        sceneNode->setPosition( 0.0f, 10.0f, 10.0f );
        sceneNode->attachObject( light );
        light->setDirection( Ogre::Vector3( 0.0f, -1.0f, -1.0f ).normalisedCopy() );

        mLightNodes[2] = sceneNode;
    }

    void SinbadDemoGameState::setBlinnPhong()
    {
        Ogre::String datablockNames[NUM_DATABLOCKS] =
        { "demo_floor", "grass",
          "Sinbad/Body", "Sinbad/Gold", "Sinbad/Sheaths", "Sinbad/Clothes", "Sinbad/Teeth",
          "Sinbad/Eyes", "Sinbad/Spikes", "Sinbad/Blade", "Sinbad/Ruby", "Sinbad/Hilt", "Sinbad/Handle" };

        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        // populate our datablock list
        for( int i = 0; i < NUM_DATABLOCKS; i++ )
        {
            Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                        hlmsPbs->getDatablock( datablockNames[i] ) );
            // set datablock shadow bias to remove acne, default value is 0.01f
            datablock->setBrdf( Ogre::PbsBrdf::BlinnPhong );
        }
    }

    void SinbadDemoGameState::setShadowConstantBias()
    {
        Ogre::String datablockNames[NUM_DATABLOCKS] =
        { "demo_floor", "grass",
          "Sinbad/Body", "Sinbad/Gold", "Sinbad/Sheaths", "Sinbad/Clothes", "Sinbad/Teeth",
          "Sinbad/Eyes", "Sinbad/Spikes", "Sinbad/Blade", "Sinbad/Ruby", "Sinbad/Hilt", "Sinbad/Handle" };

        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        // populate our datablock list
        for( int i = 0; i < NUM_DATABLOCKS; i++ )
        {
            Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                        hlmsPbs->getDatablock( datablockNames[i] ) );
            // set datablock shadow bias to remove acne, default value is 0.01f
            datablock->mShadowConstantBias = 0.01f;
        }
    }

    void SinbadDemoGameState::setTransparencyToMaterial( const Ogre::String &datablockName,
                                                         const float transparenyValue )
    {
        Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->getDatablock( datablockName ) );

        datablock->setTransparency( transparenyValue, Ogre::HlmsPbsDatablock::Transparent );
    }

    void SinbadDemoGameState::update( float timeSinceLast )
    {
        mPhyWorld->stepSimulation( timeSinceLast );

        mSinbadCharater->update( timeSinceLast );

        mOriginalGrassMesh->update( mGrassMesh, timeSinceLast );

        // Distortion update
        for( int i = 0; i<8; ++i )
        {
            mDistortionSceneNode[i]->yaw( Ogre::Radian( timeSinceLast * (i + 1.0f) * 0.825f ) );
        }

        // Update distortion uniform
        Ogre::GpuProgramParametersSharedPtr psParams = mDistortionPass->getFragmentProgramParameters();
        psParams->setNamedConstant( "u_DistortionStrenght", mDistortionStrenght );

        BasicGameState::update( timeSinceLast );
    }

    void SinbadDemoGameState::switchLightPreset( int direction )
    {
        struct Preset
        {
            float lightPower[3];  // 3 lights ( 1 directional, 2 spot lights respectively )
        };

        const Preset c_presets[] =
        {
            { 1.0f, Ogre::Math::PI, Ogre::Math::PI  },
            { 1.0f, 0.0f,           0.0f },
            { 1.0f, Ogre::Math::PI, 0.0f },
            { 1.0f, 0.0f,           Ogre::Math::PI  },
        };

        const Ogre::uint32 numPresets = sizeof(c_presets) / sizeof(c_presets[0]);

        if( direction >= 0 )
            mCurrentLightPreset = (mCurrentLightPreset + 1) % numPresets;
        else
            mCurrentLightPreset = (mCurrentLightPreset + numPresets - 1) % numPresets;

        const Preset &preset = c_presets[mCurrentLightPreset];

        for( int i=0; i<3; ++i )
        {
            assert( dynamic_cast<Ogre::Light*>( mLightNodes[i]->getAttachedObject( 0 ) ) );
            Ogre::Light *light = static_cast<Ogre::Light*>( mLightNodes[i]->getAttachedObject( 0 ) );
            light->setPowerScale( preset.lightPower[i] );
        }
    }

    void SinbadDemoGameState::switchCameraMode( int direction )
    {
        if( direction >= 0 )
            mCurrentCameraMode = ( mCurrentCameraMode + 1 ) % NUM_CAM_MODES;
        else
            mCurrentCameraMode = ( mCurrentCameraMode + NUM_CAM_MODES - 1 ) % NUM_CAM_MODES;

        mSinbadCameraController->setCameraMode( mCurrentCameraMode );
    }

    void SinbadDemoGameState::changeFog( int direction )
    {
        if( direction >= 0 )
            mFogExpDensity += 0.005f;
        else
            mFogExpDensity -= 0.005f;

        if( mFogExpDensity > 0.05f ) mFogExpDensity = 0.05f;
        else if( mFogExpDensity < 0.0f ) mFogExpDensity = 0.0f;


        mGraphicsSystem->getSceneManager()->
                setFog( Ogre::FOG_EXP2, Ogre::ColourValue( 0.5f, 0.5f, 0.5f, 1.0f ), mFogExpDensity, 8.0f, 50.0f );
    }

    void SinbadDemoGameState::changeDistortionStrength( int direction )
    {
        if( direction >= 0 )
            mDistortionStrenght += 0.05f;
        else
            mDistortionStrenght -= 0.05f;
    }

    void SinbadDemoGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        BasicGameState::generateDebugText( timeSinceLast, outText );

        if( mDisplayHelpMode == 1 )
        {
            outText += "\n\nHold SHIFT to decrease values";
            outText += "\n[L] to change Light Preset";
            outText += "\n[R] Stop/Start Rain";
            outText += "\n[F] Fog Exponent Density = ";
            outText += Ogre::StringConverter::toString( mFogExpDensity );
            outText += "\n[T] Distortion strenght = ";
            outText += Ogre::StringConverter::toString( mDistortionStrenght );
        }
    }

    void SinbadDemoGameState::keyPressed( const SDL_KeyboardEvent &evt )
    {
        mSinbadCharater->keyPressed( evt );
        BasicGameState::keyPressed( evt );
    }

    void SinbadDemoGameState::keyReleased( const SDL_KeyboardEvent &evt )
    {
        if( (evt.keysym.mod & ~(KMOD_NUM|KMOD_CAPS|KMOD_LSHIFT|KMOD_RSHIFT) ) != 0 )
        {
            BasicGameState::keyReleased( evt );
            return;
        }

        if( evt.keysym.sym == SDLK_ESCAPE )
        {
            mGraphicsSystem->setQuit();
            return;
        }
        
        if( evt.keysym.sym == SDLK_c )
        {
           switchCameraMode( (evt.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) ? -1 : 1 );
           return;
        }

        if( evt.keysym.sym == SDLK_t )
        {
            changeDistortionStrength( (evt.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) ? -1 : 1 );
            return;
        }

        if( evt.keysym.sym == SDLK_r )
        {
            mRainVisible = !mRainVisible;
            mRainNode->setVisible( mRainVisible );
            return;
        }

        if( evt.keysym.sym == SDLK_f )
        {
            changeFog( (evt.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) ? -1 : 1 );
            return;
        }

        if( evt.keysym.sym == SDLK_l )
        {
            switchLightPreset( (evt.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) ? -1 : 1 );
            return;
        }

        else
        {
            mSinbadCharater->keyReleased( evt );
            BasicGameState::keyReleased( evt );
        }
    }

    void SinbadDemoGameState::mouseMoved( const SDL_Event &evt )
    {
        mSinbadCameraController->mouseMoved( evt );
    }

    void SinbadDemoGameState::mousePressed( const SDL_MouseButtonEvent &evt, const Ogre::uint8 id )
    {
        mSinbadCharater->mousePressed( evt, id );
    }
}
