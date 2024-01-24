package com.sequoiadb.session.accessisolation;

import java.util.concurrent.ConcurrentHashMap;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * FileName: SessionAccessIsolation14107.java test content:multiple session
 * connections on the same coord,seting session properties are S testlink
 * case:seqDB-14107
 * 
 * @author wuyan
 * @Date 2018.1.16
 * @version 1.00
 */
public class SessionAccessIsolation14107 extends SdbTestBase {
    private String clName = "session_14107";
    private static DBCollection cl = null;
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String groupName = "group14107";
    private ConcurrentHashMap< Object, String > instanceidTosvcName = new ConcurrentHashMap< Object, String >();
    private Object[] instanceid = { 155, 31, 23, 244 };

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        int nodeNums = 4;
        SessionAccessUtils.createRGAndNode( sdb, groupName, instanceid,
                nodeNums, instanceidTosvcName );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ReplSize:0,Compressed:true,Group:'" + groupName
                + "'}";
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );
        SessionAccessUtils.insertData( cl );
    }

    @Test
    public void testSession() {
        int instanceid = 244;
        String nodeName = instanceidTosvcName.get( instanceid );

        AccessSession accessSession = new AccessSession( nodeName );

        accessSession.start( 20 );

        Assert.assertTrue( accessSession.isSuccess(),
                accessSession.getErrorMsg() );
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

        private AccessSession( String expQueryNodeName ) {
            this.expQueryNodeName = expQueryNodeName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject session = new BasicBSONObject();
                BSONObject arr = new BasicBSONList();
                arr.put( "0", 244 );
                arr.put( "1", "S" );
                session.put( "PreferedInstance", arr );
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
