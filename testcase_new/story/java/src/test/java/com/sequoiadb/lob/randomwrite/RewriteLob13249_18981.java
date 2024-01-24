package com.sequoiadb.lob.randomwrite;

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

/**
 * @Description seqDB-13249:write empty pieces over limits, record empty pieces
 *              metadata information no more than 320 bytes , No more than 40
 *              empty pieces * seqDB-18981 主子表写空切片超出限制
 * @author wuyan
 * @Date 2017.11.7
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13249_18981 extends SdbTestBase {
    private String clName = "writelob13249";
    private String mainCLName = "maincl18981";
    private String subCLName = "subcl18981";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13249
                new Object[] { clName },
                // testcase:18981
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
        int writeSize = 261120;
        byte[] testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );
        rewriteLob( cl, oid );
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
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void rewriteLob( DBCollection cl, ObjectId oid ) {
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE ) ;) {
            int rewriteLobSize = 262144;
            int maxEmptyPieces = 40;
            for ( int i = 0; i < maxEmptyPieces; i++ ) {
                int offset = ( i * 2 + 2 ) * 524288;
                byte[] rewriteBuff = RandomWriteLobUtil
                        .getRandomBytes( rewriteLobSize );
                lob.lockAndSeek( offset, rewriteLobSize );
                lob.write( rewriteBuff );
            }
            Assert.fail(
                    "the number of empty pieces exceeds the limit to be reported wrong" );
        } catch ( BaseException e ) {
            if ( -319 != e.getErrorCode() ) {
                Assert.fail( "empty pieces num limit check faild!e="
                        + e.getErrorCode() + e.getMessage() );
            }
        }
    }
}
