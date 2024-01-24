package com.sequoiadb.sdb.serial;

import java.util.ArrayList;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-20015:forceSession接口测试
 * @Author huangxiaoni
 * @Date 2019.10.10
 */

public class ForceSession20015 extends SdbTestBase {
    private Sequoiadb sdb;
    private ArrayList< String > groupNames;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone." );
        }
        groupNames = CommLib.getDataGroupNames( sdb );
    }

    @Test
    public void test_forceSession() throws InterruptedException {
        Sequoiadb nodeDB1 = null;
        Sequoiadb nodeDB2 = null;
        DBCursor cursor;
        try {
            nodeDB1 = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster()
                    .connect();
            nodeDB2 = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster()
                    .connect();

            // get the nodeDB1 sessionID
            cursor = nodeDB1.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null,
                    null, null );
            int sessionId = ( ( BasicBSONObject ) cursor.getNext() )
                    .getInt( "SessionID" );
            cursor.close();

            // nodeDB2 forceSession the nodeDB1 session
            nodeDB2.forceSession( sessionId );

            // check results
            try {
                nodeDB1.isCollectionSpaceExist( csName );
                Assert.fail( "expect fail but actual success." );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -15 ) {
                    throw e;
                }
            }

            cursor = nodeDB2.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT,
                    new BasicBSONObject( "SessionID", sessionId ), null, null );
            boolean hasNext = cursor.hasNext();
            cursor.close();
            Assert.assertFalse( hasNext );
        } finally {
            if ( nodeDB1 != null )
                nodeDB1.disconnect();
            if ( nodeDB2 != null )
                nodeDB2.disconnect();
        }
    }

    @Test
    public void test_forceSessionByOption_match() {
        Sequoiadb nodeDB = null;
        DBCursor cursor;
        try {
            Node master = sdb.getReplicaGroup( groupNames.get( 0 ) )
                    .getMaster();
            nodeDB = master.connect();
            int nodeId = master.getNodeId();

            // get the nodeDB sessionID
            cursor = nodeDB.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null,
                    null, null );
            int sessionId = ( ( BasicBSONObject ) cursor.getCurrent() )
                    .getInt( "SessionID" );
            cursor.close();

            // sdb forceSession the nodeDB session, option match
            try {
                sdb.forceSession( sessionId,
                        new BasicBSONObject( "NodeID", nodeId ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -264 ) { // nomal throw
                    throw e;
                }
            }

            // check results
            try {
                nodeDB.isCollectionSpaceExist( csName );
                Assert.fail( "expect fail but actual success." );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -15 ) {
                    throw e;
                }
            }

            cursor = sdb.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT,
                    new BasicBSONObject( "SessionID", sessionId ), null, null );
            boolean hasNext = cursor.hasNext();
            cursor.close();
            Assert.assertFalse( hasNext );
        } finally {
            nodeDB.disconnect();
        }
    }

    @Test
    public void test_forceSessionByOption_noMatch() {
        Sequoiadb nodeDB1 = null;
        Sequoiadb nodeDB2 = null;
        DBCursor cursor;
        try {
            nodeDB1 = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster()
                    .connect();
            Node node2 = sdb.getReplicaGroup( groupNames.get( 0 ) ).getMaster();
            int nodeId2 = node2.getNodeId();
            nodeDB2 = node2.connect();

            // get the nodeDB sessionID
            cursor = nodeDB1.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null,
                    null, null );
            int sessionId = ( ( BasicBSONObject ) cursor.getCurrent() )
                    .getInt( "SessionID" );
            cursor.close();

            // nodeID is unique in the node, if the specified node(node2) has
            // the same nodeID, then skip
            cursor = nodeDB2.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null,
                    null, null );
            if ( cursor.hasNext() ) {
                cursor.close();
                return;
            }

            // sdb forceSession the nodeDB session, option no match
            try {
                sdb.forceSession( sessionId,
                        new BasicBSONObject( "NodeID", nodeId2 ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -264 ) {
                    throw e;
                }
            }

            // check results
            nodeDB1.isCollectionSpaceExist( csName );
            cursor = nodeDB1.getList( Sequoiadb.SDB_LIST_SESSIONS_CURRENT, null,
                    null, null );
            boolean hasNext = cursor.hasNext();
            cursor.close();
            Assert.assertTrue( hasNext );
        } finally {
            nodeDB1.disconnect();
            nodeDB2.disconnect();
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.disconnect();
    }
}
