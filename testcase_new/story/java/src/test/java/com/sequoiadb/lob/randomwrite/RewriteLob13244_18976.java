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
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-13244:multiple execution of locked data segments to write
 *              lob operations, and locking data segment range discontinuity
 *              seqDB-18976 主子表同一会话多次锁定/写入lob，其中锁定范围有交集
 * @author wuyan
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Date 2017.11.7
 * @version 1.00
 */
public class RewriteLob13244_18976 extends SdbTestBase {
    private String clName = "writelob13244";
    private String mainCLName = "maincl18976";
    private String subCLName = "subcl18976";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13244
                new Object[] { clName },
                // testcase:18976
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024}";
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
        int writeSize = 1024 * 2;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );
        rewriteLob( cl, oid );
        checkAllLobResult( cl, oid );
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

    private void checkAllLobResult( DBCollection cl, ObjectId oid ) {
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

    private void rewriteLob( DBCollection cl, ObjectId oid ) {
        int offset1 = 1024;
        int lobSize1 = 1024 * 254;
        int offset2 = 1024 * 5;
        int lobSize2 = 1024 * 1024;
        byte[] rewriteBuff1 = RandomWriteLobUtil.getRandomBytes( lobSize1 );
        byte[] rewriteBuff2 = RandomWriteLobUtil.getRandomBytes( lobSize2 );

        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset1, lobSize1 );
            lob.write( rewriteBuff1 );

            lob.lockAndSeek( offset2, lobSize2 );
            lob.write( rewriteBuff2 );
        }

        // write to compare buff
        testLobBuff = RandomWriteLobUtil.appendBuff( testLobBuff, rewriteBuff1,
                offset1 );
        testLobBuff = RandomWriteLobUtil.appendBuff( testLobBuff, rewriteBuff2,
                offset2 );

        // the intersection part is covered and written
        byte[] coverWriteLobBuff = RandomWriteLobUtil.seekAndReadLob( cl, oid,
                rewriteBuff2.length, offset2 );
        RandomWriteLobUtil.assertByteArrayEqual( coverWriteLobBuff,
                rewriteBuff2 );
    }
}
