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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-20347 READ和SHARED_READ并发
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20347 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20347";
    private String mainCLName = "mainCL20347";
    private String subCLName = "subCL20347";
    private int lobSize = 1024 * 100;
    private byte[] lobBuff;

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
        lobBuff = RandomWriteLobUtil.lobBuff;
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName, byte[] writeLobBuff ) {
        int writeSize = writeLobBuff.length;
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }

        DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        ObjectId id = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );

        db1 = CommLib.getRandomSequoiadb();
        db2 = CommLib.getRandomSequoiadb();
        DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );
        DBCollection cl2 = db2.getCollectionSpace( SdbTestBase.csName )
                .getCollection( clName );

        DBLob lob1 = cl1.openLob( id, DBLob.SDB_LOB_READ );
        DBLob lob2 = cl2.openLob( id, DBLob.SDB_LOB_SHAREREAD );

        ReadThread read = new ReadThread( lob1, writeSize / 2, writeSize );
        ReadThread shareRead = new ReadThread( lob2, writeSize, writeSize );
        read.start();
        shareRead.start();

        Assert.assertTrue( read.isSuccess(), read.getErrorMsg() );
        Assert.assertTrue( shareRead.isSuccess(), read.getErrorMsg() );

        lob1.close();
        lob2.close();

        RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, lobBuff );
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
        }
    }

    private class ReadThread extends SdbThreadBase {

        private DBLob lob;
        private int begin;
        private int length;

        public ReadThread( DBLob lob, int begin, int length ) {
            this.lob = lob;
            this.begin = begin;
            this.length = length;
        }

        @Override
        public void exec() throws BaseException {
            byte[] readLob = new byte[ length ];
            lob.lockAndSeek( this.begin, this.length );
            lob.read( readLob );
            byte[] expData = Arrays.copyOfRange( lobBuff, this.begin,
                    this.begin + this.length );
            RandomWriteLobUtil.assertByteArrayEqual( readLob, expData,
                    "lob data is wrong" );
        }
    }

}
