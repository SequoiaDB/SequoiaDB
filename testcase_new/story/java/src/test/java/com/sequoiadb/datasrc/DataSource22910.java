package com.sequoiadb.datasrc;

import java.util.ArrayList;
import java.util.List;

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
 * @Description seqDB-22910:并发创建使用数据源的集合和删除数据源
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22910 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource22910";
    private String csName = "cs_22910";
    private String srcCSName = "cssrc_22910";
    private String clName = "clsrc_22910";

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
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        DeleteDataSrc deleteDataSrc = new DeleteDataSrc();
        CreateCLUseDataSrc createCL = new CreateCLUseDataSrc();
        es.addWorker( deleteDataSrc );
        es.addWorker( createCL );
        es.run();

        if ( createCL.getRetCode() != 0 ) {
            Assert.assertEquals( deleteDataSrc.getRetCode(), 0 );
            // 创建cl失败,删除数据源成功
            Assert.assertEquals( createCL.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_NOTEXIST.getErrorCode() );
            Assert.assertFalse( sdb.isDataSourceExist( dataSrcName ) );
        } else {
            // 创建cl成功，删除数据源失败
            Assert.assertEquals( deleteDataSrc.getRetCode(),
                    SDBError.SDB_CAT_DATASOURCE_INUSE.getErrorCode() );
            Assert.assertTrue( sdb.isDataSourceExist( dataSrcName ) );
            insertAndCheckResult( csName, clName );
        }

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

    private class CreateCLUseDataSrc extends ResultStore {
        private Sequoiadb db = null;
        private BasicBSONObject options = new BasicBSONObject();
        private CollectionSpace cs = null;

        @ExecuteOrder(step = 1)
        private void prepare() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            options.put( "DataSource", dataSrcName );
            options.put( "Mapping", srcCSName + "." + clName );
            cs = db.createCollectionSpace( csName );
        }

        @ExecuteOrder(step = 2)
        private void createCL() {
            try {
                cs.createCollection( clName, options );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void insertAndCheckResult( String csName, String clName ) {
        List< BSONObject > insertRecords = new ArrayList< BSONObject >();
        BSONObject obj = new BasicBSONObject();
        obj.put( "a", "test数据操作" );
        insertRecords.add( obj );
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.insert( insertRecords );
        DataSrcUtils.checkRecords( dbcl, insertRecords, "" );
    }
}
