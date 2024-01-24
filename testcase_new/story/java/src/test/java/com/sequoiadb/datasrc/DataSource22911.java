package com.sequoiadb.datasrc;

import java.util.ArrayList;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22911:并发使用数据源和删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22911 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource22911";
    private String csName = "cs_22911";
    private String srcCSName = "cssrc_22911";
    private String srcCLName = "clsrc_22911";
    private String clName = "cl_22911";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, srcCLName );
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + srcCLName );
        cs.createCollection( clName, options );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DeleteDataSrc deleteDataSrc = new DeleteDataSrc();
        UseDataSrc useDataSrc = new UseDataSrc();
        es.addWorker( deleteDataSrc );
        es.addWorker( useDataSrc );
        es.run();

        Assert.assertEquals( useDataSrc.getRetCode(), 0 );
        Assert.assertEquals( deleteDataSrc.getRetCode(),
                SDBError.SDB_CAT_DATASOURCE_INUSE.getErrorCode() );

        Assert.assertTrue( sdb.isDataSourceExist( dataSrcName ) );

    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb.dropCollectionSpace( srcCSName );
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class DeleteDataSrc extends ResultStore {
        @ExecuteOrder(step = 2)
        private void delete() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropDataSource( dataSrcName );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class UseDataSrc extends ResultStore {
        private Sequoiadb db = null;
        private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        private DBCollection dbcl = null;

        @ExecuteOrder(step = 1)
        private void prepare() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( csName ).getCollection( clName );
            for ( int i = 0; i < 2000; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", "test" + i );
                obj.put( "no", i );
                obj.put( "num", i );
                insertRecords.add( obj );
            }
        }

        @ExecuteOrder(step = 2)
        private void insertData() {
            try {
                dbcl.insert( insertRecords );
                DataSrcUtils.checkRecords( dbcl, insertRecords, "" );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}
