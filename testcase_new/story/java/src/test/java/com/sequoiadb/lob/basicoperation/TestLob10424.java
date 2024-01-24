package com.sequoiadb.lob.basicoperation;

import java.util.Random;

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
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description:seqDB-10424:并发删除相同lob *
 * @Author linsuqiang
 * @Date 2016-12-12
 * @Version 1.00
 */
public class TestLob10424 extends SdbTestBase {
    private String clName = "cl_10424";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private ObjectId delOid = null; // which lob will be delete
    private int delLobSize;

    public class Md5Data {
        public ObjectId oid = null;
        public String md5 = null;

        public Md5Data() {
        }

        public Md5Data( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = ( BSONObject ) JSON.parse(
                "{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096}" );
        cl = cs.createCollection( clName, options );
        Md5Data md5Data = buildAndPutLob( cl );
        delOid = md5Data.oid;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() {
        RemoveLobsThread removeLobsThread = new RemoveLobsThread();
        removeLobsThread.start( 10 );
        Assert.assertTrue( removeLobsThread.isSuccess(),
                removeLobsThread.getErrorMsg() );
        checkRemain();
    }

    private class RemoveLobsThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                // do remove lob
                cl.removeLob( delOid );
            } catch ( BaseException e ) {
                // -4, File Not Exist
                // -269, LOB is not usable
                // -317, LOB is using
                // both above are acceptable
                if ( e.getErrorCode() != -4 && e.getErrorCode() != -317
                        && e.getErrorCode() != -269 ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    public Md5Data buildAndPutLob( DBCollection cl ) {
        // build a lob and get it's md5, then insert
        Random random = new Random();
        int lobsize = random.nextInt( 1048576 );
        // delLobSize is a global parameter. Change it here for convenience
        delLobSize = lobsize;
        String lobSb = LobOprUtils.getRandomString( lobsize );
        Md5Data prevMd5 = new Md5Data();

        try ( DBLob lob = cl.createLob()) {
            lob.write( lobSb.getBytes() );
            prevMd5.oid = lob.getID();
            prevMd5.md5 = LobOprUtils.getMd5( lobSb );
        }
        return prevMd5;
    }

    public void checkRemain() {
        // check whether lob is removed
        try {
            cl.openLob( delOid );
            Assert.fail( "lob has not been removed! loboid=" + delOid );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -4, e.getMessage() );
        }
        // check whether lob remains
        // insert a new lob with delOid
        String lobSb = LobOprUtils.getRandomString( delLobSize );
        DBLob wLob = cl.createLob( delOid );
        wLob.write( lobSb.getBytes() );
        String prevMd5 = LobOprUtils.getMd5( lobSb );
        wLob.close();
        // read the new lob
        DBLob rLob = cl.openLob( delOid );
        byte[] buff = new byte[ ( int ) rLob.getSize() ];
        rLob.read( buff );
        String afterMd5 = LobOprUtils.getMd5( buff );
        rLob.close();
        // check the correctness of the new lob
        if ( !prevMd5.equals( afterMd5 ) ) {
            Assert.fail( "lob remains! the lob oid =" + delOid );
        }
    }
}