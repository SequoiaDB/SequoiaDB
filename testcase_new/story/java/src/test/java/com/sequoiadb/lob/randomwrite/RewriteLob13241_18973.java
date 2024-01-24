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
 * @Description seqDB-13241:lock the data segment to write lob,and locking data
 *              segment range discontinuity
 * @author wuyan
 * @Date 2017.11.7
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13241_18973 extends SdbTestBase {
    private String clName = "writelob13241";
    private String mainCLName = "maincl18973";
    private String subCLName = "subcl18973";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13241
                new Object[] { clName },
                // testcase:18973
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
        int writeSize = 1024 * 1024 * 35;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );

        int[] offset = { 1024, 524288, 26214400, 46214400 };
        int[] writeLobSize = { 260096, 524289, 262144, 2621440 };
        long actLobSize = rewriteLob( cl, oid, offset, writeLobSize );
        checkLobSize( actLobSize );
        checkLobDataResult( cl, oid, offset, writeLobSize );
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

    private void checkLobDataResult( DBCollection cl, ObjectId oid,
            int[] offset, int[] writeLength ) {
        // check the offset write lob
        int lockedSegments = offset.length;
        try ( DBLob lob = cl.openLob( oid )) {
            for ( int i = 0; i < lockedSegments; i++ ) {
                byte[] actLobBuff = new byte[ writeLength[ i ] ];
                lob.seek( offset[ i ], DBLob.SDB_LOB_SEEK_SET );
                lob.read( actLobBuff );
                byte[] expBuff = Arrays.copyOfRange( testLobBuff, offset[ i ],
                        offset[ i ] + writeLength[ i ] );
                Assert.assertEquals( actLobBuff, expBuff,
                        "the lob offset=" + offset[ i ] + " different" );
            }
        }

    }

    private void checkLobSize( long actLobSize ) {
        // check the all write lob
        long expLobSize = 48835840;
        Assert.assertEquals( actLobSize, expLobSize,
                "the lobsize is different!" );
    }

    private long rewriteLob( DBCollection cl, ObjectId oid, int offset[],
            int writeLobSize[] ) {
        long actWriteLobSize = 0;
        int lockedSegments = offset.length;
        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            for ( int i = 0; i < lockedSegments; i++ ) {
                byte[] rewriteBuff = RandomWriteLobUtil
                        .getRandomBytes( writeLobSize[ i ] );
                lob.lockAndSeek( offset[ i ], writeLobSize[ i ] );
                lob.write( rewriteBuff );
                testLobBuff = RandomWriteLobUtil.appendBuff( testLobBuff,
                        rewriteBuff, offset[ i ] );
            }

            actWriteLobSize = lob.getSize();
        }
        return actWriteLobSize;
    }
}
