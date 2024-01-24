package com.sequoiadb.lob.shareread;

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
 * @Description seqDB-20349 WRITE和SHARED_READ并发
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20349 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private Sequoiadb db1 = null;
    private Sequoiadb db2 = null;
    private CollectionSpace cs = null;
    private String clName = "cl20349";
    private String mainCLName = "mainCL20349";
    private String subCLName = "subCL20349";
    private int lobSize = 1024 * 100;
    private byte[] lobBuff;
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

        ReadThread read = new ReadThread( cl1, id, DBLob.SDB_LOB_SHAREREAD,
                writeSize / 2, writeSize );
        WriteThread write = new WriteThread( cl2, id, DBLob.SDB_LOB_WRITE,
                writeSize, writeSize, writeLobBuff );
        read.start();
        write.start();

        Assert.assertTrue( read.isSuccess(), read.getErrorMsg() );
        if ( write.isSuccess() ) {
            RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, expData );
        } else {
            BaseException error = ( BaseException ) write.getExceptions()
                    .get( 0 );
            if ( error.getErrorCode() != -320 ) {
                throw error;
            }
            RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, lobBuff );
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
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
        }
    }

    private class ReadThread extends SdbThreadBase {

        private DBCollection cl;
        private ObjectId id;
        private int mode;
        private int begin;
        private int length;

        public ReadThread( DBCollection cl, ObjectId id, int mode, int begin,
                int length ) {
            this.cl = cl;
            this.id = id;
            this.mode = mode;
            this.begin = begin;
            this.length = length;
        }

        @Override
        public void exec() throws BaseException {
            byte[] readLob = new byte[ length ];
            DBLob lob = cl.openLob( this.id, this.mode );
            try {
                lob.lockAndSeek( this.begin, this.length );
                lob.read( readLob );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -320 ) {
                    throw e;
                }
            } finally {
                lob.close();
            }
        }
    }

    private class WriteThread extends SdbThreadBase {

        private DBCollection cl;
        private ObjectId id;
        private int mode;
        private int begin;
        private int length;
        private byte[] writeLobBuff;

        public WriteThread( DBCollection cl, ObjectId id, int mode, int begin,
                int length, byte[] writeLobBuff ) {
            this.cl = cl;
            this.id = id;
            this.mode = mode;
            this.begin = begin;
            this.length = length;
            this.writeLobBuff = writeLobBuff;
        }

        @Override
        public void exec() throws BaseException {
            DBLob lob = cl.openLob( this.id, this.mode );
            try {
                lob.lockAndSeek( this.begin, this.length );
                lob.write( writeLobBuff );
            } finally {
                lob.close();
            }
            expData = RandomWriteLobUtil.appendBuff( lobBuff, writeLobBuff,
                    this.begin );
        }
    }

}
