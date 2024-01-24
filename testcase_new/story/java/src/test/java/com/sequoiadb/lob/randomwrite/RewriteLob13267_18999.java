package com.sequoiadb.lob.randomwrite;

import java.util.Arrays;

import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description: ConcurrentRewriteLob13267.java test content:lock the data
 *               segment to concurrent write lob, and the locking range of
 *               intersection testlink case:seqDB-13267 seqDB-18999
 *               主子表并发加锁写lob，其中锁定数据范围有交集
 * 
 * @author wuyan
 * @Date 2017.11.8
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13267_18999 extends SdbTestBase {
    private String clName = "writelob13267";
    private String mainCLName = "mainCL_18999";
    private String subCLName = "subCL_18999";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is subCLName
                // testcase:13267
                new Object[] { clName },
                // testcase:18999
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true}";
        RandomWriteLobUtil.createCL( cs, clName, clOptions );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        // put lob
        int writeSize = 1024 * 1024 * 2;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );

        int offset1 = 262145;
        int rewriteLobSize1 = 2621440;
        int offset2 = 262144;
        int rewriteLobSize2 = 1024000;
        byte[] rewriteBuff1 = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize1 );
        byte[] rewriteBuff2 = RandomWriteLobUtil
                .getRandomBytes( rewriteLobSize2 );
        RewriteLobTask rewriteLob1 = new RewriteLobTask( clName, oid, offset1,
                rewriteBuff1 );
        RewriteLobTask rewriteLob2 = new RewriteLobTask( clName, oid, offset2,
                rewriteBuff2 );
        rewriteLob1.start();
        rewriteLob2.start();

        if ( !rewriteLob1.isSuccess() ) {
            Assert.assertTrue( rewriteLob2.isSuccess(),
                    rewriteLob2.getErrorMsg() );
            BaseException e = ( BaseException ) ( rewriteLob1.getExceptions()
                    .get( 0 ) );
            Assert.assertEquals( -320, e.getErrorCode(),
                    "---the RewriteLob1 must fail" );
            readLobAndcheckWriteResult( cl, oid, testLobBuff, rewriteBuff2,
                    offset2 );
        } else {
            if ( !rewriteLob2.isSuccess() ) {
                Assert.assertTrue( !rewriteLob2.isSuccess(),
                        rewriteLob2.getErrorMsg() );
                BaseException e = ( BaseException ) ( rewriteLob2
                        .getExceptions().get( 0 ) );
                Assert.assertEquals( -320, e.getErrorCode(),
                        "---the RewriteLob2 must fail" );
                readLobAndcheckWriteResult( cl, oid, testLobBuff, rewriteBuff1,
                        offset1 );
            } else {
                // there may be delays that lead to the success of two tasks
                // success
                Assert.assertTrue( rewriteLob2.isSuccess(),
                        rewriteLob1.getErrorMsg() );
                int rewriteSize = rewriteLobSize1 + ( offset1 - offset2 );
                byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, oid,
                        rewriteSize, offset2 );
                byte[] expBuff = Arrays.copyOfRange( testLobBuff, offset2,
                        offset2 + rewriteSize );
                RandomWriteLobUtil.assertByteArrayEqual( actBuff, expBuff );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RewriteLobTask extends SdbThreadBase {
        private String clNamet;
        private ObjectId oid;
        private int offset;
        private byte[] rewriteLobBuff;

        public RewriteLobTask( String clName, ObjectId oid, int offset,
                byte[] rewriteLobBuff ) {
            this.clNamet = clName;
            this.oid = oid;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @Override
        public void exec() throws Exception {
            DBLob lob = null;
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( this.clNamet );
                lob = cl.openLob( this.oid, DBLob.SDB_LOB_WRITE );
                lob.lockAndSeek( offset, rewriteLobBuff.length );
                lob.write( rewriteLobBuff );
                lob.close();
                testLobBuff = RandomWriteLobUtil.appendBuff( testLobBuff,
                        rewriteLobBuff, offset );
            }
        }
    }

    private void readLobAndcheckWriteResult( DBCollection cl, ObjectId oid,
            byte[] lobBuff, byte[] rewriteBuff, int offset ) {
        // check the rewrite lob
        byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, oid,
                rewriteBuff.length, offset );
        RandomWriteLobUtil.assertByteArrayEqual( actBuff, rewriteBuff );

        // check the all write lob
        try ( DBLob lob = cl.openLob( oid )) {
            byte[] actAllLob = new byte[ ( int ) lob.getSize() ];
            lob.read( actAllLob );
            if ( !Arrays.equals( actAllLob, testLobBuff ) ) {
                RandomWriteLobUtil.writeLobAndExpectData2File( lob,
                        testLobBuff );
                Assert.fail( "check actlob and expbuff different" );
            }
        }
    }
}
