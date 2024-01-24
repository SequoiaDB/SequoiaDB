package com.sequoiadb.lob.randomwrite;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-18967 主子表未加锁写lob时分片边界值验证
 * @author luweikang
 * @Date 2019.09.12
 * @version 1.00
 */
public class RewriteLob18967 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider")
    public Object[][] generatePageSize() {
        int lobPageSizes[] = { 4096, 8192, 16384, 32768, 65536, 131072, 262144,
                524288 };
        int len = lobPageSizes.length;
        int num = lobPageSizes[ random.nextInt( len ) ];
        int lobMetaSize = 1024;
        return new Object[][] {
                // the paramter is lobPageSize, write offset,write lob length
                // sharding after just over 1 pages,rewrite lobsize is 256k
                new Object[] { num, 0, num - lobMetaSize },
                // sharding after less than 1byte, lobsize is num-1
                new Object[] { num, num - lobMetaSize, num - 1 },
                // sharding after more than 1kb
                new Object[] { num, num - lobMetaSize, num + 1 }, };
    }

    private String csName = "lobcs18967";
    private String mainCLName = "mainCL18967";
    private String subCLName = "subCL18967";
    private static Sequoiadb sdb = null;
    Random random = new Random();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testLob( int lobPageSize, int offset, int length ) {
        DBCollection cl = createCsAndCl( sdb, lobPageSize );
        int writeSize = 1024 * 1024;
        byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );

        byte[] rewritelobBuff = RandomWriteLobUtil.getRandomBytes( length );
        rewriteLob( cl, oid, offset, rewritelobBuff );
        RandomWriteLobUtil.checkRewriteLobResult( cl, oid, offset,
                rewritelobBuff, lobBuff );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void rewriteLob( DBCollection cl, ObjectId oid, int offset,
            byte[] rewriteLobBuff ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.write( rewriteLobBuff );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, "rewrite lob fail" + e.getMessage() );
        }
    }

    private DBCollection createCsAndCl( Sequoiadb sdb, int lobPagesize ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "LobPageSize", lobPagesize );
        sdb.createCollectionSpace( csName, options );
        DBCollection cl = LobSubUtils.createMainCLAndAttachCL( sdb, csName,
                mainCLName, subCLName );
        return cl;
    }

}
