package com.sequoiadb.lob.basicoperation;

import java.util.Arrays;

import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-60012:开启lob数据页crc校验后,lockAndSeek中途覆盖写(RMW)
 *              产生的"非全写页"不应误报crc错误,且数据正确。
 *              该场景需低层lob写接口,JS高层API无法构造,故用java用例覆盖。
 * @author Claude
 * @Date 2026.07.02
 * @version 1.00
 */
public class TestLobDataChecksumRMW60012 extends SdbTestBase {
    private String clName = "cl_lobChecksumRMW60012";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private static final int PAGE_SIZE = 262144; // 默认lob页大小256K

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = LobOprUtils.createCL( cs, clName );
        // 开启写生成 + 读校验
        sdb.updateConfig(
                new BasicBSONObject( "lobdatachecksum", "write|read" ) );
    }

    @Test
    public void test() {
        // 1) 写入多个整页数据(满写页,带crc)
        int lobSize = PAGE_SIZE * 4;
        byte[] lobBuff = LobOprUtils.getRandomBytes( lobSize );
        ObjectId oid = LobOprUtils.createAndWriteLob( cl, lobBuff );

        // 2) lockAndSeek 到页中间的非对齐偏移,覆盖写一小段(触发RMW)
        int offset = PAGE_SIZE * 2 + 1000;
        int rmwLen = 5000;
        byte[] rmwBuff = LobOprUtils.getRandomBytes( rmwLen );
        try ( DBLob wLob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            wLob.lockAndSeek( offset, rmwLen );
            wLob.write( rmwBuff );
        }

        // 3) 构造期望数据:原数据在[offset,offset+rmwLen)处被覆盖
        byte[] expBuff = Arrays.copyOf( lobBuff, lobSize );
        System.arraycopy( rmwBuff, 0, expBuff, offset, rmwLen );

        // 4) 开启读校验完整读回:RMW页为非全写页(NoCRC)不应报错,
        //    其余满写页crc仍应校验通过,且整体数据正确
        byte[] rBuff = new byte[ lobSize ];
        try ( DBLob rLob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
            int readLen = 0;
            while ( readLen < lobSize ) {
                int n = rLob.read( rBuff, readLen, lobSize - readLen );
                if ( n <= 0 ) {
                    break;
                }
                readLen += n;
            }
            Assert.assertEquals( readLen, lobSize, "read length mismatch" );
        }
        LobOprUtils.assertByteArrayEqual( rBuff, expBuff,
                "lob data after RMW is wrong" );
    }

    @AfterClass
    public void tearDown() {
        try {
            // 恢复默认配置
            sdb.updateConfig(
                    new BasicBSONObject( "lobdatachecksum", "write" ) );
            if ( cs != null && cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
