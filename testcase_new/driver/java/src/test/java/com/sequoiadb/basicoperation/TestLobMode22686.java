package com.sequoiadb.basicoperation;

import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestLobMode22686 extends SdbTestBase {

    private String clName = "cl_22686";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    private byte[] data = new byte[ 1024 ];
    private ObjectId lobId;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
        new Random().nextBytes( data );
        DBLob lob = cl.createLob();
        lob.write( data );
        lobId = lob.getID();
        lob.close();
    }

    @Test
    public void test() {
        DBLob writeLob = cl.openLob( lobId, DBLob.SDB_LOB_WRITE );
        try {
            byte[] data = new byte[ 1024 ];
            writeLob.read( data );
            Assert.fail( "cannot read in write mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        } finally {
            writeLob.close();
        }

        DBLob readLob = cl.openLob( lobId, DBLob.SDB_LOB_READ );
        try {
            byte[] data = new byte[ 1024 ];
            readLob.write( data );
            Assert.fail( "cannot write in read mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        } finally {
            readLob.close();
        }

        DBLob shareLob = cl.openLob( lobId, DBLob.SDB_LOB_SHAREREAD );
        try {
            byte[] data = new byte[ 1024 ];
            shareLob.write( data );
            Assert.fail( "cannot write in shareread mode." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        } finally {
            shareLob.close();
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
