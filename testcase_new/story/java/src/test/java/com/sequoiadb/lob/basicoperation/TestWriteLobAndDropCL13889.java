package com.sequoiadb.lob.basicoperation;

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
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-13889: when drop cl of write lob
 * @author wuyan
 * @Date 2016.9.12
 * @update [2017.12.21]
 * @version 1.00
 */
public class TestWriteLobAndDropCL13889 extends SdbTestBase {
    private String clName = "cl_lob13889";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private Random random = new Random();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
    }

    @Test
    public void testWriteLobAndDropCL() {
        PutLobsTask putLobsTask = new PutLobsTask();
        putLobsTask.start( 20 );
        dropCL();

        Assert.assertTrue( putLobsTask.isSuccess(), putLobsTask.getErrorMsg() );
        Assert.assertEquals( cs.isCollectionExist( clName ), false,
                "the cl must be drop!" );
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

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int writeLobSize = random.nextInt( 1024 * 1024 );
                byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
                ObjectId oid = LobOprUtils.createAndWriteLob( dbcl, wlobBuff );

                // read lob and check the lob data
                try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    LobOprUtils.assertByteArrayEqual( rbuff, wlobBuff,
                            "lob:" + oid.toString() + " data is wrong!" );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -317 && e.getErrorCode() != -23
                        && e.getErrorCode() != -4 ) {
                    throw e;
                }
            }
        }
    }

    private void dropCL() {
        // random time to delete cl in writing lob
        long sleeptime = random.nextInt( 2000 );
        try {
            Thread.sleep( sleeptime );
        } catch ( InterruptedException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        cs.dropCollection( clName );
    }

}
