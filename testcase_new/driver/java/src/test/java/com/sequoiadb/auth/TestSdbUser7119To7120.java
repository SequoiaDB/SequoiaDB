package com.sequoiadb.auth;

import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:Testlink seqDB-7119 seqDB-7120
 * @author chensiqin
 * @Date 2016-09-19
 * @version 1.00
 */

public class TestSdbUser7119To7120 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7119";
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        try {
            this.coordAddr = SdbTestBase.coordUrl;
            this.commCSName = SdbTestBase.csName;
            this.sdb = new Sequoiadb( this.coordAddr, "", "" );
            if ( !Util.isCluster( this.sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            this.cs = this.sdb.getCollectionSpace( this.commCSName );
            createCL();

        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestSdbUser7119 setUp error, error " +
                            "description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName );
    }

    @Test
    public void test() {
        testSdbUser7119();
        testSdbUser7120();
    }

    public void testSdbUser7119() {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < 257; i++) {
            stringBuilder.append("a");
        }
        String str = "test";
        String outRange = stringBuilder.toString();
        // username out of range
        try {
            sdb.createUser( outRange, str );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }
        // password out of range
        try {
            sdb.createUser( str, outRange );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() ) {
                throw e;
            }
        }

        String username = "用户七一一九";
        String password = "密码七一一九";
        Sequoiadb sdb1 = null;
        try {
            BSONObject bson = new BasicBSONObject();
            bson.put( "name", "xiaoming" );
            bson.put( "age", 6 );
            this.cl.insert( bson );

            this.sdb.createUser( username, password );
            sdb1 = new Sequoiadb( coordAddr, username, password );
            sdb1.getCollectionSpace( commCSName ).getCollection( clName )
                    .delete( "{age:{$et:6}}" );
            // user admin to insertdata
            BSONObject bsonObject = new BasicBSONObject();
            bsonObject.put( "school", "university" );
            bsonObject.put( "score", 99.999 );
            sdb1.getCollectionSpace( commCSName ).getCollection( clName )
                    .insert( bsonObject );
            DBCursor cursor = sdb1.getCollectionSpace( commCSName )
                    .getCollection( clName ).query();
            BSONObject actual = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                actual = cursor.getNext();
            }
            cursor.close();
            Assert.assertEquals( actual, bsonObject );
            // test node.connect disconnect
            Node node = null;
            try {
                node = this.sdb.getReplicaGroup( "SYSCatalogGroup" )
                        .getMaster();
                node.connect( username, password );
                node.disconnect();
            } catch ( BaseException e ) {
                Assert.fail( "connect or disconnect node failed, errMsg:"
                        + e.getMessage() );
            }
        } finally {
            if ( sdb1 != null ) {
                this.sdb.removeUser( username, password );
                sdb1.disconnect();
            }
        }
    }

    public void testSdbUser7120() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        String username = "admin7120";
        String password = "admin7120";
        Sequoiadb sdb1 = null;
        try {
            for ( int i = 0; i < 3; i++ ) {
                this.sdb.createUser( username, password );
                sdb1 = new Sequoiadb( coordAddr, username, password );
                Assert.assertFalse( sdb1.isCollectionSpaceExist( username ) );
            }
            Assert.fail(
                    "Sequoiadb driver TestSdbUser7120 repeat create the same " +
                            "user!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -295 );
        } finally {
            if ( sdb1 != null ) {
                this.sdb.removeUser( username, password );
                sdb1.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
        } finally {
            if ( this.sdb != null ) {
                this.sdb.disconnect();
            }
        }
    }
}
