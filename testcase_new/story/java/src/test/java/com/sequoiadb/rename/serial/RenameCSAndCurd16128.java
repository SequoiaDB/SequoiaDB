package com.sequoiadb.rename.serial;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.rename.RenameUtil;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName RenameCSAndCurd16128.java
 * @content the cl in multiple groups,concurrent rename cs and data operations
 *          of cl
 * @testlink seqDB-16128
 * @author wuyan
 * @Date 2018.10.29
 * @version 1.00
 */
public class RenameCSAndCurd16128 extends SdbTestBase {

    private String csName = "renameCS16128";
    private String newCSName = "renameCSNew16128";
    private String clName = "rename16128";
    private Sequoiadb sdb = null;
    private int insertSuccessNums = 0;
    private int updateSuccessNums = 0;
    private String srcGroupName = null;
    private String targetGroupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        RenameUtil.removeCS( sdb, csName );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase16128" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "less than two groups skip testcase16128" );
        }
        RenameUtil.removeCS( sdb, csName );
        RenameUtil.removeCS( sdb, newCSName );

        List< String > rgNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = rgNames.get( 0 );
        targetGroupName = rgNames.get( 1 );
        CollectionSpace cs = RenameUtil.createCS( sdb, csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true, Group:'" + srcGroupName + "'}";

        DBCollection cl = RenameUtil.createCL( cs, clName, clOptions );
        int present = 50;
        cl.split( srcGroupName, targetGroupName, present );
        insertDatas( cl );
    }

    @Test
    public void test() {
        InsertDatasThread insertDatasThread = new InsertDatasThread();
        UpdateDatasThread updateDatasThread = new UpdateDatasThread();
        RenameCSThread renameCSThread = new RenameCSThread();
        insertDatasThread.start();
        updateDatasThread.start();
        renameCSThread.start();

        if ( renameCSThread.isSuccess() ) {
            if ( !insertDatasThread.isSuccess() ) {
                BaseException e = ( BaseException ) ( insertDatasThread
                        .getExceptions().get( 0 ) );
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    Assert.fail( "inseertDatas fail:"
                            + insertDatasThread.getErrorMsg() );
                }
                checkInsertResult( newCSName, clName, insertSuccessNums );
            }
            if ( !updateDatasThread.isSuccess() ) {
                BaseException e = ( BaseException ) ( updateDatasThread
                        .getExceptions().get( 0 ) );
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                    Assert.fail( "updateDatas fail:"
                            + updateDatasThread.getErrorMsg() );
                }
                checkUpdateResult( newCSName, clName, updateSuccessNums );
            }

            int clNum = 1;
            RenameUtil.checkRenameCSResult( sdb, csName, newCSName, clNum );
        } else if ( !renameCSThread.isSuccess() ) {
            Assert.assertTrue( insertDatasThread.isSuccess(),
                    insertDatasThread.getErrorMsg() );
            Assert.assertTrue( updateDatasThread.isSuccess(),
                    updateDatasThread.getErrorMsg() );

            BaseException e = ( BaseException ) ( renameCSThread.getExceptions()
                    .get( 0 ) );
            if ( e.getErrorCode() != -190 && e.getErrorCode() != -334 ) {
                Assert.fail( "renameCS fail:" + renameCSThread.getErrorMsg() );
            }

            long successNums = 5000;
            checkInsertResult( csName, clName, successNums );
            checkUpdateResult( csName, clName, successNums );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RenameUtil.removeCS( sdb, newCSName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class RenameCSThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    public class InsertDatasThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 5000; i < 10000; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "test", "test" + i );
                    String str = "32345.067891234567890123456789" + i;
                    BSONDecimal decimal = new BSONDecimal( str );
                    obj.put( "decimal", decimal );
                    obj.put( "no", i );
                    obj.put( "str", "test_" + String.valueOf( i ) );
                    dbcl.insert( obj );
                    insertSuccessNums++;
                }
            }
        }
    }

    private class UpdateDatasThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 5000; i++ ) {
                    String matcher = "{no:" + i + "}";
                    String modifier = "{$set:{test:'testupdate'}}";
                    dbcl.update( matcher, modifier, null );
                    updateSuccessNums++;
                }
            }
        }
    }

    private void insertDatas( DBCollection dbcl ) {
        ArrayList< BSONObject > insertRecods = new ArrayList< BSONObject >();
        for ( int i = 0; i < 5000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", "test" + i );
            String str = "32345.067891234567890123456789" + i;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimala", decimal );
            obj.put( "no", i );
            obj.put( "flag", i );
            obj.put( "str", "test_" + String.valueOf( i ) );
            insertRecods.add( obj );
        }
        dbcl.insert( insertRecods );
    }

    private void checkInsertResult( String csName, String clName,
            long recordNums ) {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        String matcher = "{no:{$gte:5000}}";
        long count = dbcl.getCount( matcher );
        Assert.assertEquals( count, recordNums, "check data are incorrect!" );
        // if (count != recordNums) {
        // Assert.fail("check data are incorrect! recordNums is " + count + "");
        // }
    }

    private void checkUpdateResult( String csName, String clName,
            long recordNums ) {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        String matcher = "{test:'testupdate'}";
        long count = dbcl.getCount( matcher );
        if ( count != recordNums ) {
            Assert.fail( "check data are incorrect! recordNums is  " + count );
        }
    }
}
