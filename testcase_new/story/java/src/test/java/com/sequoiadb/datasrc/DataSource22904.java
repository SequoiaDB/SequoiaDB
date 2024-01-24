package com.sequoiadb.datasrc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @Description seqDB-22904:使用数据源的子表并发执行数据操作
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 * 
 */

public class DataSource22904 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource22904";
    private String srcCSName = "cssrc_22904";
    private String csName = "cs_22904";
    private String mainclName = "maincl_22904";
    private String clName = "cl_22904";
    private String subclName = "subcl_22904";

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
        createAndAttachCL( cs, mainclName, subclName, clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int threads = 4;
        int perNums = 5000;
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
                .getCollection( mainclName );
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

    private void createAndAttachCL( CollectionSpace cs, String mainclName,
            String subclName1, String subclName2 ) {
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + clName );
        cs.createCollection( subclName1, options );
        cs.createCollection( subclName2 );

        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:10000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:10000},UpBound:{no:20000}}" ) );
    }
}
