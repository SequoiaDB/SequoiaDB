package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13227 : 写lob同时读lob
 * @author laojingtang
 * @UpdateAuthor wangkexin
 * @Date 2017.11.2
 * @UpdateDate 2018.08.10
 * @version 1.10
 */

public class RewriteLob13227 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection dbcl = null;
    private CollectionSpace cs = null;
    private String clName = "cl_" + this.getClass().getSimpleName();

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( coordUrl, "", "" );
        cs = db.getCollectionSpace( SdbTestBase.csName );
        dbcl = cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"_id\":1},ShardingType:\"hash\"}" ) );
    }

    @Test
    public void testLob() {
        ObjectId oid = ObjectId.get();
        int lobSize = 1024 * 1024 * 10;
        byte[] expectBytes = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBLob lob = null;
        lob = this.dbcl.createLob( oid );
        lob.write( expectBytes );
        readLobInWritingLob( oid, lobSize );
        lob.close();

        // check write lob
        byte[] actualBytes = RandomWriteLobUtil.readLob( dbcl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actualBytes, expectBytes );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void readLobInWritingLob( ObjectId oid, int lobSize ) {
        try {
            byte[] actualBytes = new byte[ lobSize ];
            DBLob lob = this.dbcl.openLob( oid );
            lob.read( actualBytes );
            lob.close();
            Assert.fail( "read lob must be fail in writing lob!" );
        } catch ( BaseException e ) {
            // -317：SDB_LOB_IS_IN_USE
            if ( e.getErrorCode() != -317 ) {
                Assert.fail( "read lob fail! e=" + e.getErrorCode()
                        + e.getMessage() );
            }
        }
    }
}
