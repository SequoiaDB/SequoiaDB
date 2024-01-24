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
 * @Description: LobTest13266.java test content:concurrent locking to the same
 *               range of data segments to write lob testlink case:seqDB-13266
 *               seqDB-18998 :: 版本: 1 :: 主子表并发加锁相同范围数据段写lob
 * @author laojingtang
 * @Date 2017.12.1
 * @version 1.00
 * @update wuyan on 2019.5.20.
 */
public class RewriteLob13266_18998 extends SdbTestBase {
    private String clName = "writelob13266";
    private String mainCLName = "mainCL_18998";
    private String subCLName = "subCL_18998";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;
    private int writeFailNum = 0;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, mainCLName, subCLName
                // testcase:133266
                new Object[] { clName },
                // testcase:18998
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
        int writeSize = 1024 * 200;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );

        int offset = 10;
        int rewriteLobSize = 1024 * 100;
        byte[] writeBuff1 = RandomWriteLobUtil.getRandomBytes( rewriteLobSize );
        byte[] writeBuff2 = RandomWriteLobUtil.getRandomBytes( rewriteLobSize );

        LockAndRewriteLobTask lockAndRewriteLob1 = new LockAndRewriteLobTask(
                clName, oid, offset, writeBuff1 );
        LockAndRewriteLobTask lockAndRewriteLob2 = new LockAndRewriteLobTask(
                clName, oid, offset, writeBuff2 );

        lockAndRewriteLob1.start( 5 );
        lockAndRewriteLob2.start( 5 );

        Assert.assertTrue( lockAndRewriteLob1.isSuccess(),
                lockAndRewriteLob1.getErrorMsg() );
        Assert.assertTrue( lockAndRewriteLob2.isSuccess(),
                lockAndRewriteLob2.getErrorMsg() );

        // the concurrent operation has at least one locking fail,so the
        // writeFailNum > 0
        Assert.assertNotEquals( writeFailNum, 0,
                "at least one locking fail!" + writeFailNum );
        checkRewriteLobBuff( cl, oid, rewriteLobSize, offset, writeBuff1,
                writeBuff2 );
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
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class LockAndRewriteLobTask extends SdbThreadBase {

        private String clNamet;
        private ObjectId oid;
        private int offset;
        private byte[] rewriteLobBuff;

        public LockAndRewriteLobTask( String clName, ObjectId oid, int offset,
                byte[] rewriteLobBuff ) {
            this.clNamet = clName;
            this.oid = oid;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( this.clNamet );
                try ( DBLob lob = cl.openLob( this.oid, DBLob.SDB_LOB_WRITE )) {
                    lob.lockAndSeek( offset, rewriteLobBuff.length );
                    lob.write( rewriteLobBuff );
                }
            } catch ( BaseException e ) {
                writeFailNum++;
                if ( -320 != e.getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void checkRewriteLobBuff( DBCollection dbcl, ObjectId oid,
            int rewriteLobSize, int offset, byte[] rewriteBuff1,
            byte[] rewriteBuff2 ) {
        // check the rewrite lob
        byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( dbcl, oid,
                rewriteLobSize, offset );
        // It is expected that the buffer may be rewriteBuff1 1 or rewriteBuff12
        byte[] expRewriteBuff = rewriteBuff1;
        if ( !Arrays.equals( actBuff, expRewriteBuff ) ) {
            expRewriteBuff = rewriteBuff2;
        }
        RandomWriteLobUtil.assertByteArrayEqual( actBuff, expRewriteBuff );

        // check the all write lob
        byte[] expBuff = RandomWriteLobUtil.appendBuff( testLobBuff,
                expRewriteBuff, offset );
        try ( DBLob lob = dbcl.openLob( oid )) {
            byte[] actAllLob = new byte[ ( int ) lob.getSize() ];
            lob.read( actAllLob );
            if ( !Arrays.equals( actAllLob, expBuff ) ) {
                RandomWriteLobUtil.writeLobAndExpectData2File( lob, expBuff );
                Assert.fail( "check actlob and expbuff different: oid="
                        + oid.toString() );
            }
        }
    }

}
