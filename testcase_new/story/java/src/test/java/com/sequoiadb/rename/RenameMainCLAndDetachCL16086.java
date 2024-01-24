package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.util.JSON;
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
 * @FileName RenameMainCLAndDetachCL16086.java
 * @content concurrent rename mainCL and attach subcl
 * @testlink seqDB-16086
 * @author wuyan
 * @Date 2018.10.31
 * @version 1.00
 */
public class RenameMainCLAndDetachCL16086 extends SdbTestBase {

    private String mainCLName = "renameCL16086";
    private String newMainCLName = "renameCLNew16086";
    private String subCLName = "renameSubCL16086";
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "standAlone skip testcase16086" );
        }

        DBCollection dbMaincl = createCLAndAttachCL( mainCLName, subCLName );
        insertDatas( dbMaincl );
    }

    @Test
    public void test() {
        DetachCLThread detachCLThread = new DetachCLThread();
        RenameCLThread renameCLThread = new RenameCLThread();
        detachCLThread.start();
        renameCLThread.start();

        if ( renameCLThread.isSuccess() ) {
            Assert.assertTrue( !detachCLThread.isSuccess(),
                    detachCLThread.getErrorMsg() );
            BaseException e = ( BaseException ) ( detachCLThread.getExceptions()
                    .get( 0 ) );
            if ( e.getErrorCode() != -147 && e.getErrorCode() != -23
                    && e.getErrorCode() != -190 ) {
                Assert.fail( "detach fail:" + detachCLThread.getErrorMsg()
                        + "  e:" + e.getErrorCode() );
            }

            RenameUtil.checkRenameCLResult( sdb, SdbTestBase.csName, mainCLName,
                    newMainCLName );
            checkDetachFailResult( subCLName );
        } else {
            Assert.assertTrue( detachCLThread.isSuccess(),
                    detachCLThread.getErrorMsg() );

            BaseException e = ( BaseException ) ( renameCLThread.getExceptions()
                    .get( 0 ) );
            if ( e.getErrorCode() != -33 && e.getErrorCode() != -334 ) {
                Assert.fail( "renameCS fail:" + renameCLThread.getErrorMsg()
                        + "  e:" + e.getErrorCode() );
            }
            checkRenameFailResult( mainCLName, newMainCLName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( newMainCLName ) ) {
                cs.dropCollection( newMainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
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
                // random wait time to different execution order
                Random random = new Random();
                int result = random.nextInt( 5000 );
                try {
                    Thread.sleep( result );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = sdb
                        .getCollectionSpace( SdbTestBase.csName );
                cs.renameCollection( mainCLName, newMainCLName );
            }
        }
    }

    public class DetachCLThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainCLName );
                dbcl.detachCollection( SdbTestBase.csName + subCLName );
            }
        }
    }

    private DBCollection createCLAndAttachCL( String mainCLName,
            String subCLName ) {
        // create maincl and subcl
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String mainCLOptions = "{ShardingKey:{no:1},ShardingType:'range',IsMainCL:true,"
                + "ReplSize:0,Compressed:true}}";
        DBCollection maincl = RenameUtil.createCL( cs, mainCLName,
                mainCLOptions );
        String clOptions = "{ShardingKey:{a:1},ReplSize:0,Compressed:true}}";
        RenameUtil.createCL( cs, subCLName, clOptions );

        // attach cl
        BSONObject options = new BasicBSONObject();
        options.put( "LowBound", JSON.parse( "{\"no\":0}" ) );
        options.put( "UpBound", JSON.parse( "{\"no\":10000}" ) );
        maincl.attachCollection( SdbTestBase.csName + "." + subCLName,
                options );

        return maincl;
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

    private void checkDetachFailResult( String subCLName ) {
        try ( DBCursor cursor = sdb.getSnapshot( 8,
                "{'Name':'" + SdbTestBase.csName + "." + subCLName + "'}", "",
                "" )) {
            // detach cl fail, than the subcl is exist on the maincl
            while ( cursor.hasNext() ) {
                BSONObject objInfo = cursor.getNext();
                if ( objInfo.get( "MainCLName" ) == null ) {
                    Assert.fail( "subcl is not exist on the maincl!" );
                } else {
                    String getMainclName = ( String ) objInfo
                            .get( "MainCLName" );
                    String expMainclName = SdbTestBase.csName + "."
                            + newMainCLName;
                    if ( !getMainclName.equals( expMainclName ) ) {
                        Assert.fail(
                                "the mainclName is error:" + getMainclName );
                    }
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
