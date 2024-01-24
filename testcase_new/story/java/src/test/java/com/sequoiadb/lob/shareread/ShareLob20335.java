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
 * @Description seqDB-20335 SHARED_READ和WRITE并发读写lob
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20335 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20335";
    private String mainCLName = "mainCL20335";
    private String subCLName = "subCL20335";
    private int lobSize = 1024 * 100;
    private byte[] expData = new byte[ lobSize ];

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clname
                new Object[] { clName, RandomWriteLobUtil.tenKbuff },
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
    public void testLob( String clName, byte[] writeLobBuff ) {
        int writeSize = writeLobBuff.length;
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

        byte[] readLobBuff = new byte[ writeSize ];
        DBLob lob1 = cl1.openLob( id, DBLob.SDB_LOB_SHAREREAD );
        DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_WRITE );
        DBLob lob3 = cl3.openLob( id, DBLob.SDB_LOB_WRITE );

        lob1.lockAndSeek( 1024 * 10, writeSize );
        lob1.read( readLobBuff );
        byte[] expData1 = Arrays.copyOfRange( lobBuff, 1024 * 10,
                1024 * 10 + writeSize );
        RandomWriteLobUtil.assertByteArrayEqual( readLobBuff, expData1,
                "lob data is wrong" );

        lob2.lockAndSeek( writeSize * 2, writeSize );
        lob2.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( lobBuff, writeLobBuff,
                writeSize * 2 );

        try {
            lob3.lockAndSeek( writeSize, writeSize * 2 );
            lob3.write( writeLobBuff );
            Assert.fail( "there should be a lock conflict here." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -320 ) {
                throw e;
            }
        }

        lob1.close();
        lob2.close();
        lob3.close();

        RandomWriteLobUtil.checkShareLobResult( dbcl, id, lobSize, expData );
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
