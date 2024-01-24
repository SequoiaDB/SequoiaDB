package com.sequoiadb.lob.basicoperation;

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
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-22108 : 并发读取创建一个已存在的lob
 * @author luweikang
 * @Date 2020.4.25
 */

public class TestLobCreate22108 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl22108";
    private DBCollection cl = null;
    private int lobSize = 1024 * 1024;
    private byte[] lobBuff;
    private ObjectId lobid = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBLob lob = cl.createLob();
        lob.write( lobBuff );
        lobid = lob.getID();
        lob.close();
    }

    @Test
    public void testLob() {
        ReadThread read = new ReadThread();
        CreateThread create = new CreateThread();

        read.start();
        create.start( 20 );

        Assert.assertTrue( read.isSuccess(), read.getErrorMsg() );
        Assert.assertTrue( create.isSuccess(), create.getErrorMsg() );

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

    private class ReadThread extends SdbThreadBase {

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 20; i++ ) {
                    byte[] readLob = new byte[ lobSize ];
                    DBLob lob = cl.openLob( lobid, DBLob.SDB_LOB_READ );
                    lob.read( readLob );
                    RandomWriteLobUtil.assertByteArrayEqual( readLob, lobBuff,
                            "lob data is wrong" );
                    lob.close();
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -317 ) {
                    throw e;
                }
            }
        }
    }

    private class CreateThread extends SdbThreadBase {

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    try {
                        DBLob lob = cl.createLob( lobid );
                        lob.close();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -5 ) {
                            throw e;
                        }
                    }
                }
            }
        }
    }

}
