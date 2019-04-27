
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
    currentNinjaIndex( 0 ),
    currentBarrelIndex( 0 ),
    mFogExpDensity( 0.0f ),
    mDistortionStrenght( 1.0f ),
    mFakeTimeOfDay( 0.0f ),
    mCurrentIntroMsgIndex( 0 ),
    mCurrentCameraMode( CAM_TPS_MODE )
  {
  }

  void SinbadDemoGameState::createScene01( void )
  {
    // load all meshes
    loadMeshes();

    // load shader cache
    loadShadersFromCache( "SinbadDemo" );

    SmaaUtils::initialize( mGraphicsSystem->getRoot()->getRenderSystem(),
                           SmaaUtils::SMAA_PRESET_HIGH,
                           SmaaUtils::EdgeDetectionLuma );

    enableDebugScript( false );

    Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();

    hlmsPbs->setShadowSettings( Ogre::HlmsPbs::PCF_2x2 );
    //setBlinnPhong( hlmsPbs );
    //setShadowConstantBias( hlmsPbs, 0.1f );

    // set some material to transparent
    setTransparencyToMaterial( hlmsPbs, "Sinbad/Ruby", 0.95f );
    setTransparencyToMaterial( hlmsPbs, "Ogre/Eyes", 0.8f );

    // scene manager
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    // ground
    createGround( sceneManager );
    createWalls( sceneManager );
    createNinjas( sceneManager );
    createBarrels( sceneManager );

    // physics: should be setup before creating physics objects
    setupBulletPhysics();
    phyCreateGround();
    phyCreateWalls();
    phyCreateNinjas();
    phyCreateBarrels();

    phyShowOneBarrel();
    phyShowOneNinja();

    // grass
    createGrass( sceneManager );

    // create particle
    createRain( sceneManager );
    createSnow( sceneManager );
    createGreenyNimbus( sceneManager );
    createAureola( sceneManager );
    createSmokeHead( sceneManager );

    // create & set fog data
    createFog( sceneManager );

    // create the distortion sphere
    createDistortion( sceneManager );

    // create all the 5 lights
    createLights( sceneManager );
    // light preset 1 - ( on, off, off, off, off )
    switchLightPreset();

    // camera
    Ogre::Camera *cam = mGraphicsSystem->getCamera();
    cam->setPosition( Ogre::Vector3( 0.0f, 1.8f, 2.5f ) );
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
    // Free rigid bodies
    for( int i = 0; i<currentNinjaIndex; i++ )
    {
      mPhyWorld->removeRigidBody( mNinjaBody[i] );
    }

    for( int i = 0; i<NUM_NINJAS; i++ )
    {
      delete mNinjaBody[i]->getMotionState();
      delete mNinjaBody[i];
      delete mNinjaShape[i];
    }


    for( int i = 0; i<currentBarrelIndex; i++ )
    {
      mPhyWorld->removeRigidBody( mBarrelBody[i] );
    }

    for( int i = 0; i<NUM_BARRELS; i++ )
    {
      delete mBarrelBody[i]->getMotionState();
      delete mBarrelBody[i];
      delete mBarrelShape[i];
    }


    mPhyWorld->removeRigidBody( mGroundBody );
    delete mGroundBody->getMotionState();
    delete mGroundBody;
    //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
    delete reinterpret_cast<btTriangleMeshShape*>(mGroundShape)->getMeshInterface();
    delete mGroundShape;

    for(int i = 0; i<NUM_WALLS; i++ )
    {
      mPhyWorld->removeRigidBody( mWallBody[i] );
      delete mWallBody[i]->getMotionState();
      delete mWallBody[i];
      //Here's a quirk of the cleanup of a "triangle" based collision shape: You need to delete the mesh interface
      delete reinterpret_cast<btTriangleMeshShape*>(mWallShape[i])->getMeshInterface();
      delete mWallShape[i];
    }

    delete mPhyWorld;

    delete mSolver;
    delete mDispatcher;
    delete mCollisionConfig;
    delete mBroadphase;
  }


  void SinbadDemoGameState::loadMeshes()
  {
    // DemoFloor
    Ogre::v1::MeshManager::getSingleton().createPlane(
                "DemoFloor",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ),
                30.0f, 30.0f, 1, 1, true, 1, 6.0f, 6.0f, Ogre::Vector3::UNIT_Z,
                Ogre::v1::HardwareBuffer::HBU_STATIC,
                Ogre::v1::HardwareBuffer::HBU_STATIC );

    MeshUtils::importV1Mesh( "DemoFloor", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

    // DemoWall
    Ogre::v1::MeshManager::getSingleton().createPlane(
                "DemoWall",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::Plane( Ogre::Vector3::UNIT_Z, 1.0f ),
                30.0f, 2.0f, 1, 1, true, 1, 15.0f, 1.0f, Ogre::Vector3::UNIT_Y,
                Ogre::v1::HardwareBuffer::HBU_STATIC,
                Ogre::v1::HardwareBuffer::HBU_STATIC );

    MeshUtils::importV1Mesh( "DemoWall", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

    // Ninja player
    MeshUtils::importV1Mesh( "Player.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    Ogre::MeshManager::getSingleton().load( "Player.mesh",
                                            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

    // mesh - RustyBarrel
    MeshUtils::importV1Mesh( "Barrel.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    Ogre::MeshManager::getSingleton().load( "Barrel.mesh",
                                            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );


    MeshUtils::importV1Mesh( "ogrehead.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    Ogre::MeshManager::getSingleton().load( "ogrehead.mesh",
                                            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

    // grass
    Ogre::VaoManager *vaoManager = mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager();
    mOriginalGrassMesh = new GrassMesh( 12, 18 );
    mGrassMesh = MeshBuilder::buildGrassMesh( vaoManager, mOriginalGrassMesh );
  }


  void SinbadDemoGameState::createGround( Ogre::SceneManager *sceneManager )
  {
    Ogre::HlmsPbs *hlmsPbs = getHlmsPbs();
    Ogre::HlmsPbsDatablock *datablock;

    // demo_floor
    datablock = static_cast<Ogre::HlmsPbsDatablock*>( hlmsPbs->getDatablock( "demo_floor" ) );

    for( size_t i=Ogre::PBSM_DIFFUSE; i<=Ogre::PBSM_ROUGHNESS; ++i )
    {
      Ogre::HlmsSamplerblock samplerblock;
      samplerblock.mU = Ogre::TAM_WRAP;
      samplerblock.mV = Ogre::TAM_WRAP;
      datablock->setSamplerblock( static_cast<Ogre::PbsTextureTypes>(i), samplerblock );
    }

    Ogre::Item *item;
    item = sceneManager->createItem(
                "DemoFloor",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::SCENE_STATIC );
    item->setDatablock( "demo_floor" );
    //item->setCastShadows( false );

    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
    sceneNode->setPosition( Ogre::Vector3( 0.0f, -1.0f, 0.0f ) );
    sceneNode->attachObject( item );

    mGroundItem = item;
    mGroundNode = sceneNode;
  }

  void SinbadDemoGameState::phyCreateGround()
  {
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

  void SinbadDemoGameState::createWall( Ogre::SceneManager *sceneManager, int index, Ogre::Vector3 pos, Ogre::Real yRot )
  {
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
    sceneNode->yaw( Ogre::Degree( yRot ) );
    sceneNode->setPosition( pos );
    sceneNode->attachObject( item );

    mWallItem[index] = item;
    mWallNode[index] = sceneNode;
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

    // wall index:
    // 0 - south, 1 - north, 2 - west, 3 - east
    // south wall
    createWall( sceneManager, SOUTH, Ogre::Vector3( 0.0f, 1.0f, 16.0f ), 180.0f );
    // north wall
    createWall( sceneManager, NORTH, Ogre::Vector3( 0.0f, 1.0f, -16.0f ), 0.0f );
    // west wall
    createWall( sceneManager, WEST, Ogre::Vector3( 16.0f, 1.0f, 0.0f ), -90.0f );
    // east wall
    createWall( sceneManager, EAST, Ogre::Vector3( -16.0f, 1.0f, 0.0f ), 90.0f );
  }

  void SinbadDemoGameState::phyCreateWall( int index )
  {
    BtOgre::StaticMeshToShapeConverter converter( mWallItem[index] );
    mWallShape[index] = converter.createTrimesh();
    auto wallState = new BtOgre::RigidBodyState( mWallNode[index] );
    mWallBody[index] = new btRigidBody( 0, wallState, mWallShape[index], btVector3( 0.0f, 0.0f, 0.0f ) );
    mPhyWorld->addRigidBody( mWallBody[index] );
  }

  void SinbadDemoGameState::phyCreateWalls()
  {
    // wall index:
    // 0 - south, 1 - north, 2 - west, 3 - east
    phyCreateWall( SOUTH );
    phyCreateWall( NORTH );
    phyCreateWall( WEST );
    phyCreateWall( EAST );
  }

  void SinbadDemoGameState::createNinja( Ogre::SceneManager *sceneManager, int index )
  {
    // sceneNode & item
    Ogre::SceneNode *sceneNode;
    Ogre::Item *item;

    item = sceneManager->createItem(
                "Player.mesh",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::SCENE_DYNAMIC );
    sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode(
                Ogre::SCENE_DYNAMIC,
                Ogre::Vector3( Ogre::Math::RangeRandom( -1.0f, 1.0f ), 10.0f, Ogre::Math::RangeRandom( -1.0f, -3.0f ) ),
                Ogre::Quaternion( 0.8f, 0.3f, 0.2f, 0.6f ) );

    sceneNode->setScale( 0.6f, 0.6f, 0.6f );
    sceneNode->attachObject( item );
    sceneNode->setVisible( false );

    mNinjaItem[index] = item;
    mNinjaNode[index] = sceneNode;
  }

  void SinbadDemoGameState::createNinjas( Ogre::SceneManager *sceneManager )
  {
    for( int i=0; i<NUM_NINJAS; i++ )
    {
      createNinja( sceneManager, i );
    }
  }

  void SinbadDemoGameState::phyCreateNinja( int index )
  {
    // physics
    btScalar mass = 5.0f;
    btVector3 inertia;

    // create shape.
    BtOgre::StaticMeshToShapeConverter converter( mNinjaItem[index] );
    mNinjaShape[index] = converter.createSphere();

    // calculate inertia.
    mNinjaShape[index]->calculateLocalInertia( mass, inertia );

    // create BtOgre MotionState (connects Ogre and Bullet).
    auto ninjaState = new BtOgre::RigidBodyState( mNinjaNode[index] );

    // create the Body.
    mNinjaBody[index] = new btRigidBody( mass, ninjaState, mNinjaShape[index], inertia );
  }

  void SinbadDemoGameState::phyCreateNinjas()
  {
    for( int i=0; i<NUM_NINJAS; i++ )
    {
      phyCreateNinja( i );
    }
  }

  void SinbadDemoGameState::phyShowOneNinja()
  {
    if( currentNinjaIndex < NUM_NINJAS )
    {
      mNinjaNode[currentNinjaIndex]->setVisible( true );

      mPhyWorld->addRigidBody( mNinjaBody[currentNinjaIndex] );
      currentNinjaIndex++;
    }
  }

  void SinbadDemoGameState::createBarrel(Ogre::SceneManager *sceneManager , int index )
  {
    // sceneNode & item
    Ogre::SceneNode *sceneNode;
    Ogre::Item *item;

    item = sceneManager->createItem(
                "Barrel.mesh",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::SCENE_DYNAMIC );
    sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode(
                Ogre::SCENE_DYNAMIC,
                Ogre::Vector3( Ogre::Math::RangeRandom( -1.0f, 1.0f ), 8.0f, Ogre::Math::RangeRandom( -1.0f, -3.0f ) ),
                Ogre::Quaternion( 0.6f, 0.3f, 0.2f, 0.8f ) );

    sceneNode->setScale( 0.18f, 0.18f, 0.18f );
    sceneNode->attachObject( item );
    sceneNode->setVisible( false );

    mBarrelItem[index] = item;
    mBarrelNode[index] = sceneNode;
  }

  void SinbadDemoGameState::createBarrels( Ogre::SceneManager *sceneManager )
  {
    for( int i=0; i<NUM_BARRELS; i++ )
    {
      createBarrel( sceneManager, i );
    }
  }

  void SinbadDemoGameState::phyCreateBarrel( int index )
  {
    // physics
    btScalar mass = 8.0f;
    btVector3 inertia;

    // create shape
    BtOgre::StaticMeshToShapeConverter converter( mBarrelItem[index] );
    mBarrelShape[index] = converter.createCylinder();

    // calculate inertia.
    mBarrelShape[index]->calculateLocalInertia( mass, inertia );

    // create BtOgre MotionState (connects Ogre and Bullet).
    auto barrelState = new BtOgre::RigidBodyState( mBarrelNode[index] );

    // create the Body.
    mBarrelBody[index] = new btRigidBody( mass, barrelState, mBarrelShape[index], inertia );
  }

  void SinbadDemoGameState::phyCreateBarrels()
  {
    for( int i=0; i<NUM_BARRELS; i++ )
    {
      phyCreateBarrel( i );
    }
  }

  void SinbadDemoGameState::phyShowOneBarrel()
  {
    if( currentBarrelIndex < NUM_BARRELS )
    {
      mBarrelNode[currentBarrelIndex]->setVisible( true );

      mPhyWorld->addRigidBody( mBarrelBody[currentBarrelIndex] );
      currentBarrelIndex++;
    }
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

    Ogre::SceneNode *sceneNode;
    Ogre::Item *item;
    for( int i = 0; i<38; i++ )
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
    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                                         Ogre::Vector3( -8.0f, 3.0f, -8.0f ) );
    sceneNode->attachObject( particleSystem );

    mRainNode = sceneNode;
  }

  void SinbadDemoGameState::createSnow( Ogre::SceneManager *sceneManager )
  {
    Ogre::ParticleSystem *particleSystem = sceneManager->createParticleSystem( "Examples/Snow" );
    //particleSystem->fastForward( 8 );
    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                                         Ogre::Vector3( -8.0f, 2.0f, 8.0f ) );
    sceneNode->attachObject( particleSystem );

    mSnowNode = sceneNode;
  }

  void SinbadDemoGameState::createGreenyNimbus( Ogre::SceneManager *sceneManager )
  {
    Ogre::ParticleSystem *particleSystem = sceneManager->createParticleSystem( "Examples/GreenyNimbus" );
    //particleSystem->fastForward( 8 );
    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                                         Ogre::Vector3( 8.0f, 1.0f, 8.0f ) );
    sceneNode->attachObject( particleSystem );

    mGreenyNimbusNode = sceneNode;
  }

  void SinbadDemoGameState::createAureola( Ogre::SceneManager *sceneManager )
  {
    Ogre::ParticleSystem *particleSystem = sceneManager->createParticleSystem( "Examples/Aureola" );
    //particleSystem->fastForward( 8 );
    Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                                         Ogre::Vector3( 8.0f, 0.0f, -8.0f ) );
    sceneNode->attachObject( particleSystem );

    mAureolaNode = sceneNode;
  }

  void SinbadDemoGameState::createSmokeHead( Ogre::SceneManager *sceneManager )
  {
    Ogre::Item *item = sceneManager->createItem(
                "ogrehead.mesh",
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::SCENE_DYNAMIC );

    mHeadPivotNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    Ogre::SceneNode *sceneNode = mHeadPivotNode->createChildSceneNode( Ogre::SCENE_DYNAMIC,
                                                                       Ogre::Vector3( -8.0f, 1.0f, 0.0f ) );
    sceneNode->yaw( Ogre::Degree( 180.0f ) );
    sceneNode->scale( 0.01f, 0.01f, 0.01f );
    sceneNode->attachObject( item );

    Ogre::ParticleSystem *particleSystem = sceneManager->createParticleSystem( "Examples/Smoke" );
    sceneNode->attachObject( particleSystem );
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

    for( int i=0; i<NUM_DISTORTIONSPHERES; ++i )
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

      Ogre::String datablockName = "DistortionMaterial" + Ogre::StringConverter::toString( i );
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

      Ogre::Item *sphereItem = sceneManager->createItem(
                  "Sphere1000.mesh",
                  Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                  Ogre::SCENE_DYNAMIC );

      sphereItem->setDatablock( datablock );
      sphereItem->setCastShadows( false );

      // Set item to be rendered in distortion queue pass (ID 6)
      sphereItem->setRenderQueueGroup( 6 );

      mDistortionSceneNode[i] = rootSceneNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );

      // Lets stack distortion objects to same position with little variation
      mDistortionSceneNode[i]->setPosition( Ogre::Math::RangeRandom(  5.0f, 9.0f ),
                                            Ogre::Math::RangeRandom(  0.0f, 3.0f ),
                                            Ogre::Math::RangeRandom( -5.0f, -9.0f ) );

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

    // directional light 0
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
    light->setDiffuseColour( 0.8f, 0.3f, 0.3f ); // red
    light->setSpecularColour( 0.8f, 0.3f, 0.3f );
    //light->setPowerScale( Ogre::Math::PI );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
    sceneNode = rootSceneNode->createChildSceneNode();
    sceneNode->setPosition( 0.0f, 6.8f, 0.0f );
    sceneNode->attachObject( light );
    light->setDirection( Ogre::Vector3( 1.0f, -1.0f, -1.0f ).normalisedCopy() );

    mLightNodes[1] = sceneNode;

    // spot light 2
    light = sceneManager->createLight();
    light->setType( Ogre::Light::LT_SPOTLIGHT );
    light->setDiffuseColour( 0.3f, 0.8f, 0.3f ); // green
    light->setSpecularColour( 0.3f, 0.8f, 0.3f );
    //light->setPowerScale( Ogre::Math::PI );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
    sceneNode = rootSceneNode->createChildSceneNode();
    sceneNode->setPosition( 0.0f, 6.8f, 0.0f );
    sceneNode->attachObject( light );
    light->setDirection( Ogre::Vector3( 1.0f, -1.0f, 1.0f ).normalisedCopy() );

    mLightNodes[2] = sceneNode;

    // spot light 3
    light = sceneManager->createLight();
    light->setType( Ogre::Light::LT_SPOTLIGHT );
    light->setDiffuseColour( 0.3f, 0.3f, 0.8f ); // blue
    light->setSpecularColour( 0.3f, 0.3f, 0.8f );
    //light->setPowerScale( Ogre::Math::PI );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
    sceneNode = rootSceneNode->createChildSceneNode();
    sceneNode->setPosition( 0.0f, 6.8f, 0.0f );
    sceneNode->attachObject( light );
    light->setDirection( Ogre::Vector3( -1.0f, -1.0f, 1.0f ).normalisedCopy() );

    mLightNodes[3] = sceneNode;

    // spot light 4
    light = sceneManager->createLight();
    light->setType( Ogre::Light::LT_SPOTLIGHT );
    light->setDiffuseColour( 0.8f, 0.8f, 0.8f ); // white
    light->setSpecularColour( 0.8f, 0.8f, 0.8f );
    //light->setPowerScale( Ogre::Math::PI );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
    sceneNode = rootSceneNode->createChildSceneNode();
    sceneNode->setPosition( 0.0f, 6.8f, 0.0f );
    sceneNode->attachObject( light );
    light->setDirection( Ogre::Vector3( -1.0f, -1.0f, -1.0f ).normalisedCopy() );

    mLightNodes[4] = sceneNode;
  }

  void SinbadDemoGameState::setBlinnPhong( Ogre::HlmsPbs *hlmsPbs )
  {
    Ogre::String datablockNames[NUM_PBSDATABLOCKS] =
    { "demo_floor", "demo_wall", "RustyBarrel", "Player/TEXFACE/Player.png", "grass",
      "Ogre/Earring", "Ogre/Skin", "Ogre/Tusks", "Ogre/Eyes",
      "Sinbad/Body", "Sinbad/Gold", "Sinbad/Sheaths", "Sinbad/Clothes", "Sinbad/Teeth",
      "Sinbad/Eyes", "Sinbad/Spikes", "Sinbad/Blade", "Sinbad/Ruby", "Sinbad/Hilt", "Sinbad/Handle" };

    // populate our datablock list
    for( int i = 0; i < NUM_PBSDATABLOCKS; i++ )
    {
      Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                  hlmsPbs->getDatablock( datablockNames[i] ) );
      // set datablock shadow bias to remove acne, default value is 0.01f
      datablock->setBrdf( Ogre::PbsBrdf::BlinnPhong );
    }
  }

  void SinbadDemoGameState::setShadowConstantBias( Ogre::HlmsPbs *hlmsPbs, float bias )
  {
    Ogre::String datablockNames[NUM_PBSDATABLOCKS] =
    { "demo_floor", "demo_wall", "RustyBarrel", "Player/TEXFACE/Player.png", "grass",
      "Ogre/Earring", "Ogre/Skin", "Ogre/Tusks", "Ogre/Eyes",
      "Sinbad/Body", "Sinbad/Gold", "Sinbad/Sheaths", "Sinbad/Clothes", "Sinbad/Teeth",
      "Sinbad/Eyes", "Sinbad/Spikes", "Sinbad/Blade", "Sinbad/Ruby", "Sinbad/Hilt", "Sinbad/Handle" };

    // populate our datablock list
    for( int i = 0; i < NUM_PBSDATABLOCKS; i++ )
    {
      Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                  hlmsPbs->getDatablock( datablockNames[i] ) );
      // set datablock shadow bias to remove acne, default value is 0.01f
      datablock->mShadowConstantBias = bias;
    }
  }

  void SinbadDemoGameState::setTransparencyToMaterial( Ogre::HlmsPbs *hlmsPbs,
                                                       const Ogre::String &datablockName,
                                                       const float transparenyValue )
  {
    Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                hlmsPbs->getDatablock( datablockName ) );

    datablock->setTransparency( transparenyValue, Ogre::HlmsPbsDatablock::Transparent );
  }

  void SinbadDemoGameState::update( float timeSinceLast )
  {
    assert( dynamic_cast<Ogre::Light*>( mLightNodes[0]->getAttachedObject( 0 ) ) );
    Ogre::Light *light = static_cast<Ogre::Light*>( mLightNodes[0]->getAttachedObject( 0 ) );


    if( mFakeTimeOfDay >= 360.0f ) mFakeTimeOfDay = 0.0f;
    mFakeTimeOfDay += 0.02f;
    light->setDirection( ( Ogre::Quaternion( Ogre::Math::Cos( Ogre::Degree(mFakeTimeOfDay) ), 0.0f, Ogre::Math::Sin( Ogre::Degree(mFakeTimeOfDay) ), 0.0f ) *
                           Ogre::Vector3( -1.0f, -1.0f, -1.0f ) ).normalisedCopy() );


    mOriginalGrassMesh->update( mGrassMesh, timeSinceLast );

    mPhyWorld->stepSimulation( timeSinceLast );

    mSinbadCharater->update( timeSinceLast );

    mHeadPivotNode->yaw( Ogre::Radian( -timeSinceLast*0.28f ) );

    // Distortion update
    for( int i = 0; i<NUM_DISTORTIONSPHERES; ++i )
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
      float lightPower[5];  // 5 lights ( 1 directional, 4 spot lights respectively )
    };

    const Preset c_presets[] =
    {
      { 1.0f,                Ogre::Math::PI, Ogre::Math::PI, Ogre::Math::PI, Ogre::Math::PI  },
      { Ogre::Math::PI,      0.0f,           0.0f,           0.0f,           0.0f            },
      { 0.1f,                Ogre::Math::PI, 0.0f,           0.0f,           0.0f            },
      { 0.1f,                0.0f,           Ogre::Math::PI, 0.0f,           0.0f            },
      { 0.1f,                0.0f,           0.0f,           Ogre::Math::PI, 0.0f            },
      { 0.1f,                0.0f,           0.0f,           0.0f,           Ogre::Math::PI  },
      { 0.1f,                Ogre::Math::PI, Ogre::Math::PI, Ogre::Math::PI, Ogre::Math::PI  }
    };

    const Ogre::uint32 numPresets = sizeof(c_presets) / sizeof(c_presets[0]);

    if( direction >= 0 )
      mCurrentLightPreset = (mCurrentLightPreset + 1) % numPresets;
    else
      mCurrentLightPreset = (mCurrentLightPreset + numPresets - 1) % numPresets;

    const Preset &preset = c_presets[mCurrentLightPreset];

    for( int i=0; i<5; ++i )
    {
      assert( dynamic_cast<Ogre::Light*>( mLightNodes[i]->getAttachedObject( 0 ) ) );
      Ogre::Light *light = static_cast<Ogre::Light*>( mLightNodes[i]->getAttachedObject( 0 ) );
      light->setPowerScale( preset.lightPower[i] );
    }

    // change sky texture
    Ogre::MaterialPtr materialSky = Ogre::MaterialManager::getSingleton().load(
                "Demo/Sky",
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

    Ogre::Pass *passSky = materialSky->getTechnique( 0 )->getPass( 0 );
    Ogre::TextureUnitState *texState = passSky->getTextureUnitState( 0 );


    if( mCurrentLightPreset == 1 )
    {
      mRainNode->setVisible( false );
      mSnowNode->setVisible( false );
      mGreenyNimbusNode->setVisible( false );
      mAureolaNode->setVisible( false );
    }
    else if( mCurrentLightPreset == 2 )
    {
      mAureolaNode->setVisible( true );
    }
    else if( mCurrentLightPreset == 3 )
    {
      mGreenyNimbusNode->setVisible( true );
    }
    else if( mCurrentLightPreset == 4 )
    {
      mSnowNode->setVisible( true );
    }
    else if( mCurrentLightPreset == 5 )
    {
      mRainNode->setVisible( true );
    }

    if( mCurrentLightPreset == 0 || mCurrentLightPreset == 1 )
    {
      texState->setCubicTextureName( "cloudy_noon.jpg", true );
    }
    else
    {
      texState->setCubicTextureName( "early_morning.jpg", true );
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
      switch( mCurrentIntroMsgIndex )
      {
      case 0:
        outText += "\n\n很多人都知道Ogre3D这个3D渲染引擎"
                   "\n\"火炬之光\"这个游戏就是用它渲染场景的";

        outText += "\n\nA lot of people know about Ogre3D"
                   "\nIt is the core part of the rendering"
                   "\nengine of the game TorchLight";
        break;

      case 1:
        outText += "\n\n但那都是这个引擎1.0版本做的事了"
                   "\n现在这个引擎已经有了构架重新设计的2.1版本";

        outText += "\n\nBut that was back in the old days of Ogre3D v1.x"
                   "\nNow it has evolved into Ogre3D v2.1 with a breaking"
                   "\nnew architecture";
        break;

      case 2:
        outText += "\n\n新的版本采用了基于物理基础的渲染(PBS)"
                   "\n使画面看起来比以往更为逼真";

        outText += "\n\nOgre3D v2.1 employees Physically Base Shading (PBS)"
                   "\nwhich makes the rendered scene more realistic"
                   "\nthen ever before";
        break;

      case 3:
        outText += "\n\n已经有著名的AAA级游戏引擎转向了基于物理基础"
                   "\n的渲染, 比如寒霜引擎等. 这足以说明PBS的重要性";

        outText += "\n\nSome famous AAA game engine has already moved their"
                   "\ncodebase to the PBS rendering, ex. Frostbite Engine."
                   "\nThis indicates how important the PBS rending is.";
        break;

      case 4:
        outText += "\n\n除了PBS渲染系统, Ogre3D 2.1版本最具特色的"
                   "\n是它的渲染合成系统, 既可以通过脚本完成, 也可以"
                   "\n通过C++程序完成, 非常灵活好用.";

        outText += "\n\nIn addition to PBS rendering system, Ogre3D v2.1 introduced"
                   "\nthe most flexible compositing system which can achieve scene"
                   "\nrendering and post process effect either with scripts or with"
                   "\nthe direct implementation of C++.";
        break;

      case 5:
        outText += "\n\n今天我用我写个一个演示程序来说明Ogre3D 2.1版本"
                   "\n有多么优秀. 演示是在AMD最低端的APU E1-2500上运行的,"
                   "\n但2.1版本依然可以让FPS值维持在40~50 :)";

        outText += "\n\nToday, I am going to show you how excellent the Ogre3D v2.1 is"
                   "\nwith a Demo I wrote. Even the Demo is running on the low(lowest)"
                   "\nend AMD E1-2500 APU, Ogre3D v2.1 can still keep the FPS at 40~50 :)";
        break;

      default:
        return;
      }
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

    if( evt.keysym.sym == SDLK_b )
    {
      phyShowOneBarrel();
      return;
    }

    if( evt.keysym.sym == SDLK_n )
    {
      phyShowOneNinja();
      return;
    }

    if( evt.keysym.sym == SDLK_i )
    {
      mCurrentIntroMsgIndex++;
      if( mCurrentIntroMsgIndex == 10 )
      {
        mCurrentIntroMsgIndex = 0;
      }
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
