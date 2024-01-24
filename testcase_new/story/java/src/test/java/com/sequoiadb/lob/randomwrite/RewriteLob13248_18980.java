package com.sequoiadb.lob.randomwrite;

import org.bson.types.ObjectId;
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
 * @Description seqDB-13248:lock the data segment to write lob,and locking data
 *              segment range discontinuity seqDB-18980 主子表多次锁定相同数据段范围写入lob
 * @author wuyan
 * @Date 2017.11.7
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13248_18980 extends SdbTestBase {
    private String clName = "writelob13248";
    private String mainCLName = "maincl18980";
    private String subCLName = "subcl18980";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13248
                new Object[] { clName },
                // testcase:18980
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash'}";
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
        int writeSize = 1024 * 1024 * 1;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );

        int offset = 1024;
        int reWriteLobSize = 1024 * 256;
        byte[] lastWriteBuff = rewriteLob( cl, oid, offset, reWriteLobSize );

        RandomWriteLobUtil.checkRewriteLobResult( cl, oid, offset,
                lastWriteBuff, testLobBuff );
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

    private byte[] rewriteLob( DBCollection cl, ObjectId oid, int offset,
            int writeLobSize ) {
        int lockCount = 10;
        byte[] rewriteBuff = new byte[ writeLobSize ];
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            for ( int i = 0; i < lockCount; i++ ) {
                rewriteBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
                lob.lockAndSeek( offset, writeLobSize );
                lob.write( rewriteBuff );
            }
        }
        // get the last written lobBuff
        return rewriteBuff;
    }
}
