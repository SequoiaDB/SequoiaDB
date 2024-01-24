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
 * @Description seqDB-13254: read empty lob piece. seqDB-18986 主子表读取lob中空切片数据
 * @author wuyan
 * @Date 2017.11.7
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @version 1.00
 */
public class RewriteLob13254_18986 extends SdbTestBase {
    private String clName = "writelob13254";
    private final String mainCLName = "maincl18986";
    private final String subCLName = "subcl18986";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;
    private byte[] testWriteBuff = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                // testcase:13254
                new Object[] { clName },
                // testcase:18986
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
        int writeSize = 1024 * 255;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, testLobBuff );
        rewriteLob( cl, oid );
        checkWriteLobResult( cl, oid );

        readLobFromNullPieces( cl, oid );
        readContainsNullPieces( cl, oid );
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

    private void checkWriteLobResult( DBCollection cl, ObjectId oid ) {
        try ( DBLob lob = cl.openLob( oid )) {
            byte[] actAllLob = new byte[ ( int ) lob.getSize() ];
            lob.read( actAllLob );
            if ( !Arrays.equals( actAllLob, testWriteBuff ) ) {
                RandomWriteLobUtil.writeLobAndExpectData2File( lob,
                        testWriteBuff );
                Assert.fail( "check actlob and expbuff different" );
            }
        }
    }

    private void rewriteLob( DBCollection cl, ObjectId oid ) {
        // contain 1 and 2 pieces, lobPageSize is 1024 *256, the first piece(0)
        // size is 255k
        int offset1 = 1024 * 255;
        int lobSize = 1024 * 512;
        // contian 5 and 6 pieces
        int offset2 = 1024 * 255 + 1024 * 256 * 4;
        byte[] rewriteBuff = RandomWriteLobUtil.getRandomBytes( lobSize );

        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE ) ;) {
            lob.lockAndSeek( offset1, lobSize );
            lob.write( rewriteBuff );
            lob.lockAndSeek( offset2, lobSize );
            lob.write( rewriteBuff );
            byte[] testBuff = RandomWriteLobUtil.appendBuff( testLobBuff,
                    rewriteBuff, offset1 );
            testWriteBuff = RandomWriteLobUtil.appendBuff( testBuff,
                    rewriteBuff, offset2 );
        }
    }

    private void readLobFromNullPieces( DBCollection cl, ObjectId oid ) {
        byte[] rbuff = null;
        try ( DBLob rLob = cl.openLob( oid ) ;) {
            // the 3th pieces is empty pieces
            int offset = 1024 * ( 255 + 256 * 2 );
            rbuff = new byte[ 1024 * 256 ];
            rLob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            rLob.read( rbuff );
            byte[] testnullBuff = new byte[ 1024 * 256 ];
            RandomWriteLobUtil.assertByteArrayEqual( rbuff, testnullBuff );
        }
    }

    private void readContainsNullPieces( DBCollection cl, ObjectId oid ) {
        byte[] rbuff = null;
        try ( DBLob rLob = cl.openLob( oid ) ;) {
            // read the 4 and 5 pieces, and the 4 pieces is null,the read length
            // is 256k+256k
            int offset = 1024 * ( 255 + 256 * 3 );
            int length = 1024 * 256 * 2;
            rbuff = new byte[ length ];
            rLob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            rLob.read( rbuff );
            byte[] containsNullBuff = Arrays.copyOfRange( testWriteBuff, offset,
                    offset + length );
            RandomWriteLobUtil.assertByteArrayEqual( rbuff, containsNullBuff );
        }
    }

}
