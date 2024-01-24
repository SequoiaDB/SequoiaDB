package com.sequoiadb.lob.basicoperation;

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
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-7843:concurrent reading when deleting the same lob
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestReadAndRemoveSameLob7843 extends SdbTestBase {
    private String clName = "cl_lob7843";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private static ObjectId oid = new ObjectId( "30bb5667c5d061d6f579d0bb" );
    private static byte[] testLobBuff = null;
    private Random random = new Random();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',ReplSize:0}";
        cl = LobOprUtils.createCL( cs, clName, clOptions );

        int writeSize = random.nextInt( 1024 * 1024 * 2 );
        testLobBuff = LobOprUtils.getRandomBytes( writeSize );
        createAndWriteLob( cl, oid, testLobBuff );
    }

    @Test
    public void testSplitAndWrite() {
        RemoveLobTask removeLob = new RemoveLobTask();
        ReadLobTask readLob = new ReadLobTask();
        removeLob.start();
        readLob.start();
        Assert.assertTrue( removeLob.isSuccess(), removeLob.getErrorMsg() );
        Assert.assertTrue( readLob.isSuccess(), readLob.getErrorMsg() );
    }

    private class RemoveLobTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb2 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = sdb2.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl2.removeLob( oid );
                // if remove success,check the remove result
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

    private class ReadLobTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            DBLob rLob = null;
            try ( Sequoiadb sdb1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl1 = sdb1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                rLob = cl1.openLob( oid );
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                rLob.close();
                // if read success,check read result
                String curMd5 = LobOprUtils.getMd5( rbuff );
                String prevMd5 = LobOprUtils.getMd5( testLobBuff );
                Assert.assertEquals( curMd5, prevMd5,
                        "the lobs md5 different" );
            } catch ( BaseException e ) {
                if ( -4 != e.getErrorCode() && -268 != e.getErrorCode()
                        && -317 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void createAndWriteLob( DBCollection dbcl, ObjectId id,
            byte[] data ) {
        DBLob lob = dbcl.createLob( id );
        lob.write( data );
        lob.close();
    }
}
