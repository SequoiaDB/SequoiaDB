package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Random;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-26504:主子表在相同CS，插入数据/renameCS
 * @Author liuli
 * @Date 2022.05.12
 * @UpdateAuthor liuli
 * @UpdateDate 2022.05.12
 * @version 1.10
 */
public class RenameCS_26504 extends SdbTestBase {

    private String csName = "cs_26504";
    private String mainCLName = "maincl_26504";
    private String subCLName1 = "subcl_26504_1";
    private String subCLName2 = "subcl_26504_2";
    private String newCSName = "cs_26504_new";
    private Sequoiadb sdb = null;
    private ArrayList< BSONObject > insertRecord = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "can not support split" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        DBCollection maincl = dbcs.createCollection( mainCLName, optionsM );

        dbcs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );

        dbcs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        RenameCS renameCS = new RenameCS();
        Insert insert = new Insert();
        es.addWorker( renameCS );
        es.addWorker( insert );
        es.run();

        Assert.assertTrue( sdb.isCollectionSpaceExist( newCSName ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );

        DBCollection maincl = sdb.getCollectionSpace( newCSName )
                .getCollection( mainCLName );
        if ( insert.getRetCode() == 0 ) {
            RenameUtil.checkRecords( maincl, insertRecord, "{a:1}" );
        } else {
            if ( insert.getRetCode() != SDBError.SDB_DMS_NOTEXIST.getErrorCode()
                    && insert.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode()
                    && insert.getRetCode() != SDBError.SDB_LOCK_FAILED
                            .getErrorCode()
                    && insert
                            .getRetCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                    .getErrorCode() ) {
                Assert.fail( "not expected error, insert.getRetCode() : "
                        + insert.getRetCode() );
            }
            // 插入报错后数据可能的1w或者0，且无法确定具体数据
            if ( maincl.getCount() != 0 && maincl.getCount() != 10000 ) {
                Assert.fail( "maincl.getCount() is " + maincl.getCount()
                        + " ,expect is 1w or 0" );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isCollectionSpaceExist( newCSName ) ) {
                sdb.dropCollectionSpace( newCSName );
            }
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCS extends ResultStore {
        @ExecuteOrder(step = 2)
        private void renameCS() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待100ms后再renameCS，防止先renameCS再插入数据
                Random random = new Random();
                Thread.sleep( random.nextInt( 100 ) );
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    private class Insert extends ResultStore {
        Sequoiadb db = null;
        DBCollection maincl = null;

        @ExecuteOrder(step = 1)
        private void getMainCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            maincl = db.getCollectionSpace( csName )
                    .getCollection( mainCLName );
        }

        @ExecuteOrder(step = 2)
        private void insert() {
            try {
                int insertNums = 20000;
                insertRecord = insertDatas( maincl, insertNums );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNums; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", "test" + i );
            obj.put( "no", i );
            obj.put( "order", i );
            obj.put( "a", i );
            obj.put( "ftest", i + 0.2345 );
            insertRecord.add( obj );
        }
        dbcl.bulkInsert( insertRecord );
        return insertRecord;
    }
}
