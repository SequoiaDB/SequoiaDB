package com.sequoiadb.lob.shareread;

import java.util.Arrays;

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
 * @Description seqDB-20336 SHARED_READ和READ并发读写lob
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20336 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private Sequoiadb db3 = null;
    private Sequoiadb db4 = null;
    private Sequoiadb db5 = null;
    private Sequoiadb db6 = null;
    private Sequoiadb db7 = null;
    private Sequoiadb db8 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20336";
    private String mainCLName = "mainCL20336";
    private String subCLName = "subCL20336";
    private int lobSize = 1024 * 100;

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
        db4 = CommLib.getRandomSequoiadb();
        db5 = CommLib.getRandomSequoiadb();
        db6 = CommLib.getRandomSequoiadb();
        db7 = CommLib.getRandomSequoiadb();
        db8 = CommLib.getRandomSequoiadb();

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
        DBCollection cl4 = db4.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl5 = db5.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl6 = db6.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl7 = db7.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl8 = db8.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );

        byte[] readLob1 = new byte[ writeSize ];
        byte[] readLob2 = new byte[ writeSize ];
        byte[] readLob3 = new byte[ writeSize ];
        byte[] readLob4 = new byte[ writeSize ];
        byte[] readLob5 = new byte[ writeSize / 2 ];
        byte[] readLob6 = new byte[ writeSize ];
        byte[] readLob7 = new byte[ writeSize ];
        byte[] readLob8 = new byte[ writeSize * 2 ];
        DBLob lob1 = cl1.openLob( id, DBLob.SDB_LOB_SHAREREAD );
        DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob3 = cl3.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob4 = cl4.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob5 = cl5.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob6 = cl6.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob7 = cl7.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob8 = cl8.openLob( id, DBLob.SDB_LOB_READ );

        lob1.lockAndSeek( writeSize * 4, writeSize );
        lob1.read( readLob1 );
        byte[] expData1 = Arrays.copyOfRange( lobBuff, writeSize * 4,
                writeSize * 5 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob1, expData1,
                "lob data is wrong" );

        // READ和SHARED_READ读区域相离
        lob2.lockAndSeek( 0, writeSize );
        lob2.read( readLob2 );
        byte[] expData2 = Arrays.copyOfRange( lobBuff, 0, writeSize );
        RandomWriteLobUtil.assertByteArrayEqual( readLob2, expData2,
                "lob data is wrong" );

        // SHARED_READ读区域在READ读区域前并相邻
        lob3.lockAndSeek( writeSize * 3, writeSize );
        lob3.read( readLob3 );
        byte[] expData3 = Arrays.copyOfRange( lobBuff, writeSize * 3,
                writeSize * 4 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob3, expData3,
                "lob data is wrong" );

        // SHARED_READ读区域在READ读区域前并相交
        lob4.lockAndSeek( writeSize * 4 - 1024, writeSize );
        lob4.read( readLob4 );
        byte[] expData4 = Arrays.copyOfRange( lobBuff, writeSize * 4 - 1024,
                writeSize * 5 - 1024 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob4, expData4,
                "lob data is wrong" );

        // SHARED_READ读区域在READ读区域内
        lob5.lockAndSeek( writeSize * 4 + 1024, writeSize / 2 );
        lob5.read( readLob5 );
        byte[] expData5 = Arrays.copyOfRange( lobBuff, writeSize * 4 + 1024,
                writeSize * 4 + 1024 + writeSize / 2 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob5, expData5,
                "lob data is wrong" );

        // SHARED_READ读区域在READ读区域后并相邻
        lob6.lockAndSeek( writeSize * 5, writeSize );
        lob6.read( readLob6 );
        byte[] expData6 = Arrays.copyOfRange( lobBuff, writeSize * 5,
                writeSize * 6 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob6, expData6,
                "lob data is wrong" );

        // SHARED_READ读区域在READ读区域后并相交
        lob7.lockAndSeek( writeSize * 4 - 1024, writeSize );
        lob7.read( readLob7 );
        byte[] expData7 = Arrays.copyOfRange( lobBuff, writeSize * 4 - 1024,
                writeSize * 5 - 1024 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob7, expData7,
                "lob data is wrong" );

        // SHARED_READ读区域包含READ读区域
        lob8.lockAndSeek( writeSize * 3, writeSize * 2 );
        lob8.read( readLob8 );
        byte[] expData8 = Arrays.copyOfRange( lobBuff, writeSize * 3,
                writeSize * 5 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob8, expData8,
                "lob data is wrong" );

        lob1.close();
        lob2.close();
        lob3.close();
        lob4.close();
        lob5.close();
        lob6.close();
        lob7.close();
        lob8.close();

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
            if ( db4 != null ) {
                db4.close();
            }
            if ( db5 != null ) {
                db5.close();
            }
            if ( db6 != null ) {
                db6.close();
            }
            if ( db7 != null ) {
                db7.close();
            }
            if ( db8 != null ) {
                db8.close();
            }
        }
    }
}
