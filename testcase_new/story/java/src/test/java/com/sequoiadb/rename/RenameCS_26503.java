package com.sequoiadb.rename;

import java.util.ArrayList;

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
 * @Description seqDB-26503:主子表在不同CS，插入数据/rename子表所在的CS
 * @Author liuli
 * @Date 2022.05.12
 * @UpdateAuthor liuli
 * @UpdateDate 2022.05.12
 * @version 1.10
 */
public class RenameCS_26503 extends SdbTestBase {

    private String csName = "cs_26503";
    private String subCSName = "subcs_26503";
    private String mainCLName = "maincl_26503";
    private String subCLName1 = "subcl_26503_1";
    private String subCLName2 = "subcl_26503_2";
    private String newCSName = "cs_26503_new";
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
        if ( sdb.isCollectionSpaceExist( subCSName ) ) {
            sdb.dropCollectionSpace( subCSName );
        }
        if ( sdb.isCollectionSpaceExist( newCSName ) ) {
            sdb.dropCollectionSpace( newCSName );
        }

        CollectionSpace maincs = sdb.createCollectionSpace( csName );

        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        DBCollection maincl = maincs.createCollection( mainCLName, optionsM );

        maincs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );

        CollectionSpace subcs = sdb.createCollectionSpace( subCSName );
        subcs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( subCSName + "." + subCLName2, option2 );
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
        Assert.assertFalse( sdb.isCollectionSpaceExist( subCSName ) );

        if ( insert.getRetCode() == 0 ) {
            DBCollection maincl = sdb.getCollectionSpace( csName )
                    .getCollection( mainCLName );
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
                Assert.fail( "insert.getRetCode() : " + insert.getRetCode() );
            }
            DBCollection subcl = sdb.getCollectionSpace( newCSName )
                    .getCollection( subCLName2 );
            RenameUtil.checkRecords( subcl, insertRecord, "{a:1}" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isCollectionSpaceExist( subCSName ) ) {
                sdb.dropCollectionSpace( subCSName );
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
        @ExecuteOrder(step = 1)
        private void renameCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.renameCollectionSpace( subCSName, newCSName );
            }
        }
    }

    private class Insert extends ResultStore {
        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                int insertNums = 20000;
                insertRecord = insertDatas( maincl, insertNums );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
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
