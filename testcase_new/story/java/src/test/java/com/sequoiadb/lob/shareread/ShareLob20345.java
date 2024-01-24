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
 * @Description seqDB-20345 SHARED_READ|WRITE先读后写同一个lob
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20345 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl20345";
    private String mainCLName = "mainCL20345";
    private String subCLName = "subCL20345";
    private int lobSize = 1024 * 100;
    private byte[] expData = new byte[ lobSize ];

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
    public void testLob( String clName, byte[] writeLobBuff ) {
        int writeSize = writeLobBuff.length;
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }

        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        byte[] lobBuff = RandomWriteLobUtil.lobBuff;
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );

        byte[] readLob1 = new byte[ writeSize ];
        DBLob lob = cl.openLob( id,
                DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );

        lob.lockAndSeek( writeSize, writeSize );
        lob.read( readLob1 );
        byte[] expData1 = Arrays.copyOfRange( lobBuff, writeSize,
                writeSize * 2 );
        RandomWriteLobUtil.assertByteArrayEqual( readLob1, expData1,
                "lob data is wrong" );

        // SHARED_READ|WRITE 写区域相离
        lob.lockAndSeek( writeSize * 3, writeSize );
        lob.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( lobBuff, writeLobBuff,
                writeSize * 3 );

        // SHARED_READ|WRITE 写区域相交
        lob.lockAndSeek( writeSize + 1024, writeSize );
        lob.write( writeLobBuff );
        expData = RandomWriteLobUtil.appendBuff( expData, writeLobBuff,
                writeSize + 1024 );

        lob.close();

        RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, expData );
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
}
