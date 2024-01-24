package com.sequoiadb.datasrc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-22903:并发使用数据源执行数据操作
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 * 
 */
@Test
public class DataSource22903 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource22903";
    private String srcCSName = "cssrc_22903";
    private String csName = "cs_22903";
    private String clName = "cl_22903";

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
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + clName );
        cs.createCollection( clName, options );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( 300000 );
        int threads = 3;
        int perNums = 10000;
        int beginNo = 0;
        for ( int i = 0; i < threads; i++ ) {
            es.addWorker( new DataCRUD( beginNo, beginNo + perNums ) );
            beginNo = beginNo + perNums;
        }
        es.run();
    }

    @AfterClass
    public void tearDown() {
        try {
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
            srcdb.dropCollectionSpace( srcCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class DataCRUD {
        private int beginNo;
        private int endNo;
        private Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        private DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        private String queryMatcher;

        private DataCRUD( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            int recordNum = endNo - beginNo;
            insertRecords = DataSrcUtils.insertData( dbcl, recordNum, beginNo );
            queryMatcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endNo + "}}]}";
            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
        }

        @ExecuteOrder(step = 2)
        private void update() {
            int updateNum = 3000;
            int endCond = beginNo + updateNum;
            String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endCond + "}}]}";
            String modifier = "{$set:{testa:'updatetest" + beginNo + "'}}";
            dbcl.update( matcher, modifier, "" );

            for ( int i = 0; i < updateNum; i++ ) {
                BSONObject obj = insertRecords.get( i );
                obj.put( "testa", "updatetest" + beginNo );
            }

            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
        }

        @ExecuteOrder(step = 3)
        private void remove() {
            int removeNum = 4000;
            int endCond = beginNo + removeNum;
            String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endCond + "}}]}";
            dbcl.delete( matcher );
            List< BSONObject > sublist = insertRecords.subList( 0, removeNum );
            insertRecords.removeAll( sublist );
            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
        }
    }
}
