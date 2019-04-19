
#include "BtOgreExtras.h"


namespace BtOgre
{
    //---------------------------------------------------------------------------------------------
    // static Convert functions

    btQuaternion Convert::toBullet( const Ogre::Quaternion& q )
    {
        return { q.x, q.y, q.z, q.w };
    }

    btVector3 Convert::toBullet( const Ogre::Vector3& v )
    {
        return { v.x, v.y, v.z };
    }

    Ogre::Quaternion Convert::toOgre( const btQuaternion& q )
    {
        return { q.w(), q.x(), q.y(), q.z() };
    }

    Ogre::Vector3 Convert::toOgre( const btVector3& v )
    {
        return { v.x(), v.y(), v.z() };
    }


    //---------------------------------------------------------------------------------------------
    // LineDrawer

    LineDrawer::LineDrawer( Ogre::SceneNode* node, Ogre::String datablockId, Ogre::SceneManager* smgr ) :
        mAttachNode( node ),
        mDatablockToUse( datablockId ),
        mManualObject( nullptr ),
        mSceneManager( smgr ),
        mIndex( 0 )
    {
    }

    LineDrawer::~LineDrawer()
    {
        clear();
        if( mManualObject ) mSceneManager->destroyManualObject( mManualObject );
    }

    void LineDrawer::clear()
    {
        if( mManualObject ) mManualObject->clear();
        mLines.clear();
    }

    void LineDrawer::addLine( const Ogre::Vector3& start, const Ogre::Vector3& end, const Ogre::ColourValue& value )
    {
        mLines.push_back( { start, end, value } );
    }

    void LineDrawer::checkForMaterial() const
    {
        const auto hlmsUnlit = static_cast<Ogre::HlmsUnlit*>( Ogre::Root::getSingleton().getHlmsManager()->getHlms( Ogre::HLMS_UNLIT ) );
        const auto datablock = hlmsUnlit->getDatablock( mDatablockToUse );

        if( datablock ) return;
        DebugDrawer::logToOgre( "BtOgre's datablock not found, creating..." );
        auto createdDatablock = hlmsUnlit->createDatablock( mDatablockToUse, mDatablockToUse, {}, {}, {}, true, Ogre::BLANKSTRING, DebugDrawer::BtOgre21ResourceGroup );

        if( !createdDatablock ) throw std::runtime_error( std::string( "BtOgre Line Drawer failed to create HLMS Unlit datablock" ) + mDatablockToUse );
    }

    void LineDrawer::update()
    {
        if( !mManualObject )
        {
            DebugDrawer::logToOgre( "Create manual object" );

            mManualObject = mSceneManager->createManualObject( Ogre::SCENE_STATIC );
            DebugDrawer::logToOgre( "Set no shadows" );
            mManualObject->setCastShadows( false );
            DebugDrawer::logToOgre( "Attach object to node" );
            mAttachNode->attachObject( mManualObject );
            DebugDrawer::logToOgre( "done creating object" );
        }

        checkForMaterial();
        mManualObject->begin( mDatablockToUse, Ogre::OT_LINE_LIST );

        mIndex = 0;
        for( const auto& l : mLines )
        {
            mManualObject->position( l.start );
            mManualObject->colour( l.vertexColor );
            mManualObject->index( mIndex++ );

            mManualObject->position( l.end );
            mManualObject->colour( l.vertexColor );
            mManualObject->index( mIndex++ );
        }

        mManualObject->end();
    }


    //---------------------------------------------------------------------------------------------
    // DebugDrawer

    void DebugDrawer::logToOgre( const std::string& message )
    {
        Ogre::LogManager::getSingleton().logMessage( "BtOgre21Log : " + message );
    }

    DebugDrawer::DebugDrawer( Ogre::SceneNode* node, btDynamicsWorld* world, Ogre::String smgrName ) :
        mNode( node->createChildSceneNode( Ogre::SCENE_STATIC ) ),
        mWorld( world ),
        mDebugOn( true ),
        mUnlitDatablockId( mUnlitDatablockName ),
        mUnlitDiffuseMultiplier( 1 ),
        mStepped( false ),
        mSceneManagerName( smgrName ),
        mSceneManager( Ogre::Root::getSingleton().getSceneManager( smgrName ) ),
        mDrawer( mNode, mUnlitDatablockName, mSceneManager )
    {
        init();
    }

    DebugDrawer::DebugDrawer( Ogre::SceneNode* node, btDynamicsWorld* world, Ogre::SceneManager* smgr ) :
        mNode( node->createChildSceneNode( Ogre::SCENE_STATIC ) ),
        mWorld( world ),
        mDebugOn( true ),
        mUnlitDatablockId( mUnlitDatablockName ),
        mUnlitDiffuseMultiplier( 1 ),
        mStepped( false ),
        mSceneManagerName( "nonamegiven" ),
        mSceneManager( smgr ),
        mDrawer( mNode, mUnlitDatablockName, smgr )
    {
        init();
    }

    void DebugDrawer::init()
    {
        if( !Ogre::ResourceGroupManager::getSingleton().resourceGroupExists( BtOgre21ResourceGroup ) )
            Ogre::ResourceGroupManager::getSingleton().createResourceGroup( BtOgre21ResourceGroup );
    }

    void DebugDrawer::setUnlitDiffuseMultiplier( float value )
    {
        if( value >= 1 ) mUnlitDiffuseMultiplier = value;
    }

    void DebugDrawer::drawLine( const btVector3& from, const btVector3& to, const btVector3& color )
    {
        if( mStepped )
        {
            mDrawer.clear();
            mStepped = false;
        }

        const auto ogreFrom = Convert::toOgre( from );
        const auto ogreTo = Convert::toOgre( to );

        Ogre::ColourValue ogreColor{ color.x(), color.y(), color.z(), 1.0f };
        ogreColor *= mUnlitDiffuseMultiplier;

        mDrawer.addLine( ogreFrom, ogreTo, ogreColor );
    }

    void DebugDrawer::draw3dText( const btVector3& location, const char* textString )
    {
    }

    void DebugDrawer::drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color )
    {
        drawLine( pointOnB, pointOnB + normalOnB * distance * 20, color );
    }

    void DebugDrawer::reportErrorWarning( const char* warningString )
    {
        logToOgre( warningString );
    }

    void DebugDrawer::setDebugMode( int isOn )
    {
        mDebugOn = isOn;

        if( !mDebugOn ) mDrawer.clear();
    }

    int DebugDrawer::getDebugMode() const
    {
        return mDebugOn;
    }

    void DebugDrawer::step()
    {
        if( mDebugOn )
        {
            mWorld->debugDrawWorld();
            mDrawer.update();
        }
        else
        {
            mDrawer.clear();
        }
        mStepped = true;
    }
}
