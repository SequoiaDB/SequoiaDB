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
 * @Description seqDB-13246:lock the all lob to write,set different offset
 *              positions to covering rewrite lob seqDB-18978 主子表锁定整个lob写入
 * @author wuyan
 * @Date 2019.07.16
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13246_18978 extends SdbTestBase {
    @DataProvider(name = "pagesizeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter is writeLobSize, offset, rewriteLobSize
                // start from the start position
                new Object[] { clName, 1024 * 1024, 0, 1024 * 1024 * 2 },
                // start from the middle position
                new Object[] { clName, 1024 * 1024, 1024 * 512, 1024 * 1024 },
                // start from the end postition
                new Object[] { clName, 1024 * 512, 1024 * 512, 1024 * 4 },

                // testcase:18978
                new Object[] { mainCLName, 1024 * 1024, 0, 1024 * 1024 * 2 },
                // start from the middle position
                new Object[] { mainCLName, 1024 * 1024, 1024 * 512,
                        1024 * 1024 },
                // start from the end postition
                new Object[] { mainCLName, 1024 * 512, 1024 * 512,
                        1024 * 4 }, };
    }

    private String clName = "writelob13246";
    private String mainCLName = "maincl18978";
    private String subCLName = "subcl18978";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0}";
        RandomWriteLobUtil.createCL( cs, clName, clOptions );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "pagesizeProvider")
    public void testLob( String clName, int writeLobSize, int offset,
            int rewriteLobSize ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
                throw new SkipException( "is standalone skip testcase!" );
            }
            DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl,
                    lobBuff );

            byte[] rewriteBuff = RandomWriteLobUtil
                    .getRandomBytes( rewriteLobSize );
            lockAndRewriteLob( dbcl, oid, offset, rewriteBuff );
            RandomWriteLobUtil.checkRewriteLobResult( dbcl, oid, offset,
                    rewriteBuff, lobBuff );
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
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void lockAndRewriteLob( DBCollection cl, ObjectId oid, int offset,
            byte[] rewriteBuff ) {
        long lockLength = -1;
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.lockAndSeek( offset, lockLength );
            lob.write( rewriteBuff );
        }
    }

}
