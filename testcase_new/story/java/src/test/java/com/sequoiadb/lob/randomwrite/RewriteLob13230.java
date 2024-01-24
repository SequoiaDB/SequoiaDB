package com.sequoiadb.lob.randomwrite;

import java.util.Random;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description: RemoveAndReadSameLob13230.java test content:concurrent reading
 *               when deleting the same lob testlink case:seqDB-13230
 * 
 * @author wuyan
 * @Date 2017.11.2
 * @version 1.00
 */
public class RewriteLob13230 extends SdbTestBase {
    private String clName = "writelob13230";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;
    private static byte[] testLobBuff = null;
    private Random random = new Random();
    private ObjectId oid = new ObjectId( "30bb5667c5d061d6f579d0bb" );

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true}";
        cl = RandomWriteLobUtil.createCL( cs, clName, clOptions );

        int writeSize = random.nextInt( 1024 * 1024 * 2 );
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        RandomWriteLobUtil.createAndWriteLob( cl, oid, testLobBuff );
    }

    @Test
    private void testLob() {
        RemoveLobTask removeLob = new RemoveLobTask();
        ReadLobTask readLob = new ReadLobTask();
        removeLob.start();
        readLob.start();
        removeLob.join();
        readLob.join();

        Assert.assertTrue( removeLob.isSuccess(), removeLob.getErrorMsg() );
        Assert.assertTrue( readLob.isSuccess(), readLob.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

    private class ReadLobTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            DBLob rLob = null;
            try ( Sequoiadb sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl1 = sdb1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                rLob = cl1.openLob( oid );

                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
            } catch ( BaseException e ) {
                if ( -4 != e.getErrorCode() && -268 != e.getErrorCode()
                        && -317 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class RemoveLobTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl2 = sdb2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl2.removeLob( oid );
                // check the remove result
                DBCursor listCursor1 = cl2.listLobs();
                Assert.assertEquals( listCursor1.hasNext(), false,
                        "list lob not null" );
                listCursor1.close();
            } catch ( BaseException e ) {
                if ( -268 != e.getErrorCode() && -317 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
