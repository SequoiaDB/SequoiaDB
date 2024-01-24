package com.sequoiadb.lob.basicoperation;

import java.util.Random;
import java.util.concurrent.LinkedBlockingDeque;

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
 * @Description seqDB-7844: when write lob of reading
 * @author wuyan
 * @Date 2016.9.12
 * @update 2017.12.19
 * @version 1.00
 */
public class TestPutAndReadLobs7844 extends SdbTestBase {
    private String clName = "cl_lob7844";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();

    private class LobInfo {
        public ObjectId oid;
        public String md5;
    }

    private LinkedBlockingDeque< LobInfo > lobInfoQue = new LinkedBlockingDeque< LobInfo >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash'}";
        DBCollection cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );

        int lobtimes = 30;
        writeLobAndGetMd5( cl, lobtimes );
    }

    @Test
    public void testPutAndReadLob() {
        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start( 30 );

        ReadLobsTask readLobsTask = new ReadLobsTask();
        readLobsTask.start( 60 );

        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );
        Assert.assertTrue( readLobsTask.isSuccess(),
                readLobsTask.getErrorMsg() );
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                int lobtimes = 1;
                writeLobAndGetMd5( dbcl, lobtimes );
            }
        }
    }

    private class ReadLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                LobInfo lobinfotmp = lobInfoQue.take();
                ObjectId oid = lobinfotmp.oid;
                try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = LobOprUtils.getMd5( rbuff );
                    String prevMd5 = lobinfotmp.md5;
                    Assert.assertEquals( curMd5, prevMd5 );
                }
            }
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = LobOprUtils.getMd5( wlobBuff );
            LobInfo lobInfoTmp = new LobInfo();
            lobInfoTmp.oid = oid;
            lobInfoTmp.md5 = prevMd5;
            lobInfoQue.offer( lobInfoTmp );
        }
    }

}
