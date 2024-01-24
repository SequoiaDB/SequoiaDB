package com.sequoiadb.session.accessisolation;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: SessionAccessIsolation14108.java test content:seqDB-14108:multiple
 * session connections on different coord,seting same session properties
 * testlink case:seqDB-14108/seqDB-14109
 * 
 * @author wuyan
 * @Date 2018.1.16
 * @version 1.00
 */
public class SessionAccessIsolation14108_14109 extends SdbTestBase {
    private String clName = "session_14108";
    private static DBCollection cl = null;
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String groupName = "group14108";
    private ConcurrentHashMap< Object, String > instanceidTosvcName = new ConcurrentHashMap< Object, String >();
    private Object[] instanceid = { 255, 31, 1 };
    ArrayList< String > coordUrls = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsName = CommLib.getDataGroupNames( sdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        int nodeNums = 3;
        SessionAccessUtils.createRGAndNode( sdb, groupName, instanceid,
                nodeNums, instanceidTosvcName );
        coordUrls = SessionAccessUtils.getCoordUrls( sdb );

        String clOptions = "{ReplSize:0,Compressed:true,Group:'" + groupName
                + "'}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );
        SessionAccessUtils.insertData( cl );
    }

    @Test
    public void testSession14108() {
        int expInstanceid = 1;
        String nodeName = instanceidTosvcName.get( expInstanceid );
        List< AccessSession > accessSessions = new ArrayList<>();
        for ( int i = 0; i < coordUrls.size(); i++ ) {
            accessSessions
                    .add( new AccessSession( nodeName, coordUrls.get( i ) ) );
        }

        for ( AccessSession accessSession : accessSessions ) {
            accessSession.start();
        }

        for ( AccessSession accessSession : accessSessions ) {
            Assert.assertTrue( accessSession.isSuccess(),
                    accessSession.getErrorMsg() );
        }
    }

    @Test
    public void testSession14109() {
        List< AccessDiffSession > accessDiffSessions = new ArrayList<>();
        for ( int i = 0; i < coordUrls.size(); i++ ) {
            String nodeName = instanceidTosvcName.get( instanceid[ i ] );
            accessDiffSessions.add( new AccessDiffSession( instanceid[ i ],
                    nodeName, coordUrls.get( i ) ) );
        }

        for ( AccessDiffSession accessDiffSession : accessDiffSessions ) {
            accessDiffSession.start();
        }

        for ( AccessDiffSession accessDiffSession : accessDiffSessions ) {
            Assert.assertTrue( accessDiffSession.isSuccess(),
                    accessDiffSession.getErrorMsg() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.removeReplicaGroup( groupName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class AccessSession extends SdbThreadBase {
        private String expQueryNodeName;
        private String coordUrl;

        private AccessSession( String expQueryNodeName, String coordUrl ) {
            this.expQueryNodeName = expQueryNodeName;
            this.coordUrl = coordUrl;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                BSONObject session = new BasicBSONObject();
                session.put( "PreferedInstance", 1 );
                db.setSessionAttr( session );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject match = ( BSONObject ) JSON.parse( "{a:{$gt:20}}" );
                DBCursor cursor = dbcl.explain( match, null, null, null, 0, 5,
                        DBQuery.FLG_QUERY_FORCE_HINT, null );
                while ( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    String nodeName = ( String ) obj.get( "NodeName" );
                    Assert.assertEquals( nodeName, expQueryNodeName );
                }
                cursor.close();
            }
        }
    }

    private class AccessDiffSession extends SdbThreadBase {
        private Object instanceid;
        private String expQueryNodeName;
        private String coordUrl;

        private AccessDiffSession( Object instanceid, String expQueryNodeName,
                String coordUrl ) {
            this.instanceid = instanceid;
            this.expQueryNodeName = expQueryNodeName;
            this.coordUrl = coordUrl;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
                BSONObject session = new BasicBSONObject();
                session.put( "PreferedInstance", instanceid );
                db.setSessionAttr( session );
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject match = ( BSONObject ) JSON.parse( "{a:{$gt:20}}" );
                DBCursor cursor = dbcl.explain( match, null, null, null, 0, 5,
                        DBQuery.FLG_QUERY_FORCE_HINT, null );
                while ( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    String nodeName = ( String ) obj.get( "NodeName" );
                    Assert.assertEquals( nodeName, expQueryNodeName );
                }
                cursor.close();
            }
        }
    }

}
