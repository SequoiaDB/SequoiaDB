package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
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
 * @Description RenameCS_16284.java cs数据在多个组上，并发修改cs名和读写删查LOB
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCS_16284 extends SdbTestBase {

    private String csName = "renameCS_16284";
    private String newCSName = "renameCS_16284_new";
    private String clName = "renameCS_CL_16284";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private List< ObjectId > lobIdList = null;
    private int fileSize = 1024 * 1024;
    private String MD5 = null;
    private int lobNum = 10;
    private byte[] data = null;
    private int putLobNum = 0;
    private int deleteLobNum = 0;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );

        data = new byte[ fileSize ];
        Random random = new Random();
        random.nextBytes( data );
        MD5 = RenameUtil.getMd5( data );
        lobIdList = RenameUtil.putLob( cl, data, lobNum );
    }

    @Test
    public void test() {
        RenameCSThread renameThread = new RenameCSThread();
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
            Integer[] errnosA = { -34, -23 };
            BaseException errorA = ( BaseException ) putThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( putThread.getErrorMsg() );
            }
        }

        if ( !readThread.isSuccess() ) {
            Integer[] errnosB = { -23, -34, -4, -317, -269 };
            BaseException errorB = ( BaseException ) readThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( readThread.getErrorMsg() );
            }
        }

        if ( !deleteThread.isSuccess() ) {
            Integer[] errnosC = { -23, -34, -317 };
            BaseException errorC = ( BaseException ) deleteThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosC ).contains( errorC.getErrorCode() ) ) {
                Assert.fail( deleteThread.getErrorMsg() );
            }
        }

        if ( !listThread.isSuccess() ) {
            Integer[] errnosD = { -23, -34 };
            BaseException errorD = ( BaseException ) listThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosD ).contains( errorD.getErrorCode() ) ) {
                Assert.fail( listThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCSResult( db, csName, newCSName, 1 );
            checkLob( db, newCSName, clName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCS( sdb, newCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCSThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Thread.sleep( 100 );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
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
                putLob( sdbcl, data, lobNum / 2 );
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
                    DBLob lob = sdbcl.openLob( lobIdList.get( i ) );
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
                Thread.sleep( 50 );
                for ( int i = 0; i < lobNum / 2; i++ ) {
                    sdbcl.removeLob( lobIdList.get( i ) );
                    deleteLobNum++;
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
                .getCollection( clName );
        DBCursor cur = cl.listLobs();
        int actLobNum = 0;
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
                Assert.assertEquals( e.getErrorCode(), -269, e.getMessage() );
            }
            actLobNum++;
        }
        Assert.assertEquals( actLobNum, lobNum + putLobNum - deleteLobNum,
                "check lob num" );
        cur.close();
    }

    private List< ObjectId > putLob( DBCollection cl, byte[] data,
            int lobNum ) {

        List< ObjectId > idList = new ArrayList< ObjectId >();
        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = null;
            try {
                lob = cl.createLob();
                lob.write( data );
                idList.add( lob.getID() );
            } finally {
                if ( lob != null ) {
                    lob.close();
                }
            }
            putLobNum++;
        }
        return idList;
    }

}
