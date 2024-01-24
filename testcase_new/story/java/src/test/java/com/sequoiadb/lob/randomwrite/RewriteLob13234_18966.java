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
 * @Description seqDB-13234:分区表未seek写lob；seqDB-18966 :主子表未seek写lob
 * @author wuyan
 * @Date 2017.11.2
 * @UpdateDate 2019.08.28
 * @version 1.00
 */
public class RewriteLob13234_18966 extends SdbTestBase {
    @DataProvider(name = "lobsizeProvider", parallel = true)
    public Object[][] generateLobSize() {
        return new Object[][] {
                // the parameter is csName, clName, writeLobSize, rewriteLobSize
                // testcase:13234
                // test a: the writeLobSize > rewriteLobSize
                new Object[] { SdbTestBase.csName, clName, 1024 * 1024 * 2,
                        1024 * 512 },
                // test b: the writeLobSize = rewriteLobSize
                new Object[] { SdbTestBase.csName, clName, 1024 * 1024,
                        1024 * 1024 },
                // test c : the writeLobSize < rewriteLobSize
                new Object[] { SdbTestBase.csName, clName, 1024 * 256,
                        1024 * 512 },
                // testcase:18966
                // test a: the writeLobSize > rewriteLobSize
                new Object[] { SdbTestBase.csName, mainCLName, 1024 * 1024 * 2,
                        1024 * 512 },
                // test b: the writeLobSize = rewriteLobSize
                new Object[] { SdbTestBase.csName, mainCLName, 1024 * 1024,
                        1024 * 1024 },
                // test c : the writeLobSize < rewriteLobSize
                new Object[] { SdbTestBase.csName, mainCLName, 1024 * 256,
                        1024 * 512 } };
    }

    private String clName = "writelob13234";
    private String mainCLName = "lobMainCL_18966";
    private String subCLName = "lobSubCL_18966";
    private Sequoiadb sdb = null;
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

    @Test(dataProvider = "lobsizeProvider")
    public void testLob( String csName, String clName, int writeLobSize,
            int rewriteLobSize ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
                throw new SkipException( "is standalone skip testcase!" );
            }
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            byte[] lobBuff = RandomWriteLobUtil.getRandomBytes( writeLobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( dbcl,
                    lobBuff );

            byte[] rewriteBuff = RandomWriteLobUtil
                    .getRandomBytes( rewriteLobSize );
            writeLob( dbcl, oid, rewriteBuff );
            RandomWriteLobUtil.checkRewriteLobResult( dbcl, oid, 0, rewriteBuff,
                    lobBuff );
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
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    private void writeLob( DBCollection cl, ObjectId oid, byte[] rewriteBuff ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            lob.write( rewriteBuff );
        }
    }
}
