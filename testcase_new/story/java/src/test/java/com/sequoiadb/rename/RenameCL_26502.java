package com.sequoiadb.rename;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

import java.util.ArrayList;

/**
 * @Description seqDB-26502:主子表在相同CS，插入数据/rename子表
 * @Author liuli
 * @Date 2022.05.12
 * @UpdateAuthor liuli
 * @UpdateDate 2022.05.12
 * @version 1.10
 */
public class RenameCL_26502 extends SdbTestBase {

    private String csName = "cs_26502";
    private String mainCLName = "maincl_26502";
    private String subCLName1 = "subcl_26502_1";
    private String subCLName2 = "subcl_26502_2";
    private String newCLName = "cl_26502_new";
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
        es.addWorker( new RenameCL() );
        es.addWorker( new Insert() );
        es.run();

        CollectionSpace dbcs = sdb.getCollectionSpace( csName );
        Assert.assertTrue( dbcs.isCollectionExist( newCLName ) );
        Assert.assertFalse( dbcs.isCollectionExist( subCLName1 ) );

        DBCollection maincl = dbcs.getCollection( mainCLName );
        RenameUtil.checkRecords( maincl, insertRecord, "{a:1}" );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class RenameCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void renameCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                cs.renameCollection( subCLName1, newCLName );
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
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode() ) {
                    throw e;
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
