package com.sequoiadb.rename;

import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16094.java cs数据在多个组上，并发修改cs名和读写删查LOB
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16094 extends SdbTestBase {

    private String clName = "rename_CL_16094";
    private String newCLName = "rename_CL_16094_new";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private List< ObjectId > lobIdList = null;
    private int fileSize = 1024 * 1024;
    private String MD5 = null;
    private int lobNum = 10;
    private byte[] data = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( csName );
        BSONObject options = new BasicBSONObject();
        cl = cs.createCollection( clName, options );

        data = new byte[ fileSize ];
        Random random = new Random();
        random.nextBytes( data );
        MD5 = RenameUtil.getMd5( data );
        lobIdList = RenameUtil.putLob( cl, data, lobNum );

    }

    @Test
    public void test() {
        RenameCLThread renameThread = new RenameCLThread();
        PutLobThread putThread = new PutLobThread();
        ReadLobThread readThread = new ReadLobThread();
        DeleteLobThread deleteThread = new DeleteLobThread();
        ListLobThread listThread = new ListLobThread();

        renameThread.start();
        putThread.start();
        readThread.start();
        deleteThread.start();
        listThread.start();

        Assert.assertTrue( renameThread.isSuccess(),
                renameThread.getErrorMsg() );

        if ( !putThread.isSuccess() ) {
            Integer[] errnosA = { -23 };
            BaseException errorA = ( BaseException ) putThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( putThread.getErrorMsg() );
            }
        }

        if ( !readThread.isSuccess() ) {
            Integer[] errnosB = { -23, -4, -317 };
            BaseException errorB = ( BaseException ) readThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( readThread.getErrorMsg() );
            }
        }

        if ( !deleteThread.isSuccess() ) {
            Integer[] errnosC = { -23, -317 };
            BaseException errorC = ( BaseException ) deleteThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosC ).contains( errorC.getErrorCode() ) ) {
                Assert.fail( deleteThread.getErrorMsg() );
            }
        }

        if ( !listThread.isSuccess() ) {
            Integer[] errnosD = { -23 };
            BaseException errorD = ( BaseException ) listThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosD ).contains( errorD.getErrorCode() ) ) {
                Assert.fail( listThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            checkLob( db, csName, newCLName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, csName, newCLName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( 200 );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class PutLobThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                RenameUtil.putLob( sdbcl, data, lobNum / 2 );
            }
        }
    }

    private class ReadLobThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                sdbcl.listLobs();
                byte[] data = new byte[ fileSize ];
                for ( int i = lobNum / 2; i < lobNum; i++ ) {
                    DBLob lob = sdbcl.openLob( lobIdList.get( 0 ) );
                    lob.read( data );
                    lob.close();
                }
            }
        }
    }

    private class DeleteLobThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < lobNum / 2; i++ ) {
                    sdbcl.removeLob( lobIdList.get( i ) );
                }
            }
        }
    }

    private class ListLobThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    Thread.sleep( 500 );
                    sdbcl.listLobs();
                }
            }
        }
    }

    private void checkLob( Sequoiadb db, String csName, String clNmae ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clNmae );
        DBCursor cur = cl.listLobs();
        try {
            while ( cur.hasNext() ) {
                try {
                    BSONObject idObj = cur.getNext();
                    ObjectId id = ( ObjectId ) idObj.get( "Oid" );
                    DBLob lob = cl.openLob( id );
                    byte[] data = new byte[ fileSize ];
                    lob.read( data );
                    String actMD5 = RenameUtil.getMd5( data );
                    if ( !actMD5.equals( MD5 ) ) {
                        Assert.fail( "file md5 error, exp: " + MD5 + ", act: "
                                + actMD5 );
                    }
                } catch ( BaseException e ) {
                    Assert.assertEquals( e.getErrorCode(), -269,
                            e.getMessage() );
                }
            }
        } finally {
            if ( cur != null ) {
                cur.close();
            }
        }
    }

}
