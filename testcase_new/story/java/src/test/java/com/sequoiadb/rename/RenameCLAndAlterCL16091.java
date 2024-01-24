package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Random;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName RenameCLAndAlterCL16091.java
 * @content concurrent rename CL and alter cl(for example: alter compressType)
 * @testlink seqDB-16091
 * @author wuyan
 * @Date 2018.10.31
 * @version 1.00
 */
public class RenameCLAndAlterCL16091 extends SdbTestBase {

    private String clName = "renameCL16091";
    private String newCLName = "renameCLNew16091";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase16091" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
                + "ReplSize:0,Compressed:true,CompressionType:'snappy'}";

        DBCollection cl = RenameUtil.createCL( cs, clName, clOptions );
        insertDatas( cl );
    }

    @Test
    public void test() {
        AlterCLThread alterCLThread = new AlterCLThread();
        RenameCLThread renameCLThread = new RenameCLThread();
        alterCLThread.start();
        renameCLThread.start();

        if ( renameCLThread.isSuccess() ) {
            Assert.assertTrue( renameCLThread.isSuccess(),
                    renameCLThread.getErrorMsg() );
            if ( alterCLThread.isSuccess() ) {
                Assert.assertTrue( alterCLThread.isSuccess(),
                        alterCLThread.getErrorMsg() );
                RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, clName,
                        newCLName );
                checkAlterCLResult( newCLName, true );
            } else {
                Assert.assertTrue( !alterCLThread.isSuccess(),
                        alterCLThread.getErrorMsg() );
                BaseException e = ( BaseException ) ( alterCLThread
                        .getExceptions().get( 0 ) );
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -23
                        && e.getErrorCode() != -190 ) {
                    Assert.fail( "detach fail:" + alterCLThread.getErrorMsg()
                            + "  e:" + e.getErrorCode() );
                }

                RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, clName,
                        newCLName );
                checkAlterCLResult( newCLName, false );
            }
        } else {
            Assert.assertTrue( alterCLThread.isSuccess(),
                    alterCLThread.getErrorMsg() );
            BaseException e = ( BaseException ) ( renameCLThread.getExceptions()
                    .get( 0 ) );
            if ( e.getErrorCode() != -33 && e.getErrorCode() != -334 ) {
                Assert.fail( "renameCS fail:" + renameCLThread.getErrorMsg()
                        + "  e:" + e.getErrorCode() );
            }

            checkAlterCLResult( clName, true );
            checkRenameFailResult( clName, newCLName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( newCLName ) ) {
                cs.dropCollection( newCLName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            if ( sdb != null )
                sdb.close();
        }
    }

    public class RenameCLThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // random wait time to different exection order
                Random random = new Random();
                int result = random.nextInt( 60 );
                try {
                    Thread.sleep( result );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( clName, newCLName );
            }
        }
    }

    public class AlterCLThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject options = new BasicBSONObject();
                options.put( "CompressionType", "lzw" );
                dbcl.setAttributes( options );
            }
        }
    }

    private void insertDatas( DBCollection dbcl ) {
        ArrayList< BSONObject > insertRecods = new ArrayList< BSONObject >();
        for ( int i = 0; i < 10000; i++ ) {
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

    private void checkAlterCLResult( String clName, Boolean alterSuccess ) {
        try ( DBCursor cursor = sdb.getSnapshot( 8,
                "{'Name':'" + SdbTestBase.csName + "." + clName + "'}", "",
                "" )) {
            while ( cursor.hasNext() ) {
                BSONObject objInfo = cursor.getNext();
                String compressionType = ( String ) objInfo
                        .get( "CompressionTypeDesc" );
                if ( alterSuccess ) {
                    Assert.assertEquals( compressionType, "lzw" );
                } else {
                    // does not rollback after alter compressionType fails.
                    String[] expResult = { "lzw", "snappy" };
                    Assert.assertTrue(
                            Arrays.asList( expResult )
                                    .contains( compressionType ),
                            "the type:" + compressionType );
                }
            }
        }
    }

    private void checkRenameFailResult( String oldCLName, String newCLName ) {
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( newCLName ) ) {
            Assert.fail( " the new cl should not exist!" );
        }
        if ( !cs.isCollectionExist( oldCLName ) ) {
            Assert.fail( " the old cl should be exist!" );
        }
    }

}
