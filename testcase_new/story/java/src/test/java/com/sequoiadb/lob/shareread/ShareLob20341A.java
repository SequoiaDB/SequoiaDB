package com.sequoiadb.lob.shareread;

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

/**
 * @Description seqDB-20341 SHARED_READ|WRITE和SHARED_READ并发读写lob
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20341A extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20341A";
    private String mainCLName = "mainCL20341A";
    private String subCLName = "subCL20341A";
    private int lobSize = 1024 * 100;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                new Object[] { clName, RandomWriteLobUtil.twentyKbuff },
                new Object[] { mainCLName, RandomWriteLobUtil.threeKbuff } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cs.createCollection( clName );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, SdbTestBase.csName,
                    mainCLName, subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName, byte[] readLobBuff ) {
        int readSize = readLobBuff.length;
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }

        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        db3 = CommLib.getRandomSequoiadb();

        DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.lobBuff;
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( dbcl, lobBuff );

        DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl3 = db3.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );

        byte[] readLob1 = new byte[ readSize ];
        byte[] readLob2 = new byte[ readSize ];
        byte[] readLob3 = new byte[ readSize ];
        DBLob lob1 = cl1.openLob( id,
                DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
        DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_SHAREREAD );
        DBLob lob3 = cl3.openLob( id, DBLob.SDB_LOB_SHAREREAD );

        lob1.lockAndSeek( readSize, readSize );
        lob1.read( readLob1 );
        byte[] expData1 = Arrays.copyOfRange( lobBuff, readSize, readSize * 2 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob1, expData1,
                "lob data is wrong" );

        // SHARED_READ|WRITE读区域和SHARED_READ读区域相离
        lob2.lockAndSeek( readSize * 3, readSize );
        lob2.read( readLob2 );
        byte[] expData2 = Arrays.copyOfRange( lobBuff, readSize * 3,
                readSize * 4 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob2, expData2,
                "lob data is wrong" );

        // SHARED_READ|WRITE读区域和SHARED_READ读区域相交
        try {
            lob3.lockAndSeek( readSize + 1024, readSize );
            lob3.read( readLob3 );
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -320 ) {
                throw e;
            }
        }

        lob1.close();
        lob2.close();
        lob3.close();

        RandomWriteLobUtil.checkShareLobResult( dbcl, id, lobSize, lobBuff );
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
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
            if ( db3 != null ) {
                db3.close();
            }
        }
    }
}
