package com.sequoiadb.monitor;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:seqDB-14423:重置快照接口测试
 * @Author linsuqiang
 * @Date 2016-12-12
 * @Version 1.00
 */

public class ResetSnapshot14423 extends SdbTestBase {
    private String clName = "cl_14423";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( CommLib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
            BSONObject opt = new BasicBSONObject( "Group", groupName );
            DBCollection cl = cs.createCollection( clName, opt );
            List< BSONObject > docs = new ArrayList< BSONObject >();
            for ( int i = 0; i < 100; i++ ) {
                docs.add( new BasicBSONObject( "a", i ) );
            }
            cl.insert( docs );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
    }

    @Test
    void test() {
        try {
            Sequoiadb dataDB = sdb.getReplicaGroup( groupName ).getMaster()
                    .connect();
            createStatisInfo( dataDB );
            BSONObject opt = ( BSONObject ) JSON.parse( "{Type: 'sessions'}" );
            sdb.resetSnapshot( opt );
            Assert.assertFalse( isDataBaseSnapClean( dataDB ),
                    "snapshot-database shouldn't be reset" );
            Assert.assertTrue( isSessionSnapClean( dataDB ),
                    "snapshot-session has not been reset" );
            dataDB.close();

            opt = ( BSONObject ) null;
            sdb.resetSnapshot( null );
            // as long as no exception

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    private void createStatisInfo( Sequoiadb dataDB ) {
        DBCollection cl = dataDB.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCursor cur = cl.query();
        while ( cur.hasNext() ) {
            cur.getNext();
        }
        cur.close();
    }

    private boolean isDataBaseSnapClean( Sequoiadb dataDB ) {
        DBCursor cur = dataDB.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE, "", "",
                "" );
        BSONObject rec = cur.getNext();
        long totalRead = ( long ) rec.get( "TotalRead" );
        cur.close();
        return ( totalRead == 0 );
    }

    private boolean isSessionSnapClean( Sequoiadb dataDB ) {
        DBCursor cur = dataDB.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS,
                "{Type: 'Agent'}", "", "" );
        BSONObject rec = cur.getNext();
        long totalRead = ( long ) rec.get( "TotalRead" );
        cur.close();
        return ( totalRead == 0 );
    }

}