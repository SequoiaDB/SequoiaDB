package com.sequoiadb.rename;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description RenameCS_16094.java 并发增删改数据和修改cl名
 * @author luweikang
 * @date 2018年10月17日
 */
public class RenameCL_16093 extends SdbTestBase {

    private String clName = "rename_CL_16093";
    private String newCLName = "rename_CL_16093_new";
    private int recordNum = 2000;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( csName );
        BSONObject options = new BasicBSONObject();
        cl = cs.createCollection( clName, options );
        RenameUtil.insertData( cl, recordNum );
    }

    @Test
    public void test() {
        RenameCLThread renameThread = new RenameCLThread();
        InsertThread putThread = new InsertThread();
        DeleteThread deleteThread = new DeleteThread();
        UpdateThread updateThread = new UpdateThread();

        renameThread.start();
        putThread.start();
        deleteThread.start();
        updateThread.start();

        Assert.assertTrue( renameThread.isSuccess(),
                renameThread.getErrorMsg() );

        if ( !putThread.isSuccess() ) {
            Integer[] errnosA = { -23, -190 };// node open transaction on,will
                                              // have error -190
            BaseException errorA = ( BaseException ) putThread.getExceptions()
                    .get( 0 );
            if ( !Arrays.asList( errnosA ).contains( errorA.getErrorCode() ) ) {
                Assert.fail( putThread.getErrorMsg() );
            }
        }

        if ( !updateThread.isSuccess() ) {
            Integer[] errnosB = { -23, -190 };
            BaseException errorB = ( BaseException ) updateThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosB ).contains( errorB.getErrorCode() ) ) {
                Assert.fail( updateThread.getErrorMsg() );
            }
        }

        if ( !deleteThread.isSuccess() ) {
            Integer[] errnosC = { -23, -190 };
            BaseException errorC = ( BaseException ) deleteThread
                    .getExceptions().get( 0 );
            if ( !Arrays.asList( errnosC ).contains( errorC.getErrorCode() ) ) {
                Assert.fail( deleteThread.getErrorMsg() );
            }
        }

        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            RenameUtil.checkRenameCLResult( db, csName, clName, newCLName );
            cl = db.getCollectionSpace( csName ).getCollection( newCLName );
            checkRecord( cl );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CommLib.clearCL( sdb, csName, newCLName );
        } catch ( Exception e ) {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class RenameCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    private class InsertThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 1000; i++ ) {
                    BSONObject record = new BasicBSONObject();
                    record.put( "a", 1000 + i );
                    record.put( "no", "No." + 1000 + i );
                    record.put( "phone", 13700000000L + 1000 + i );
                    record.put( "text",
                            "Test ReName, This is the test statement used to populate the data" );
                    sdbcl.insert( record );
                }
            }
        }
    }

    private class DeleteThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                sdbcl.delete( new BasicBSONObject( "a",
                        new BasicBSONObject( "$lt", "1000" ) ) );
            }
        }
    }

    private class UpdateThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection sdbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                sdbcl.update(
                        new BasicBSONObject( "a",
                                new BasicBSONObject( "$lt", "2000" ) ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "no", "No.10086" ) ),
                        null );
            }
        }
    }

    private void checkRecord( DBCollection dbcl ) {
        DBCursor cur = dbcl.query(
                "{a :{$isnull: 0}, no: {$isnull: 0}, phone: {$isnull: 0}, text: {$isnull: 0}}",
                null, null, null );
        while ( cur.hasNext() ) {
            BSONObject obj = cur.getNext();
            obj.toString();
        }
        cur.close();
    }

}
