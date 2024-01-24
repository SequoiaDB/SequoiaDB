package com.sequoiadb.datasrc;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-24223:多个coord并发使用数据源执行数据操作
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 * 
 */
@Test
public class DataSource24223 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource24223";
    private String srcCSName = "cssrc_24223new";
    private String csName = "cs_24223";
    private String clName = "cl_24223";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
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
        int perNums = 10000;
        int beginNo = 0;
        List< String > nodes = getCoordNodes( "SYSCoord" );
        for ( int i = 0; i < nodes.size(); i++ ) {
            es.addWorker( new DataCRUD( beginNo, beginNo + perNums,
                    nodes.get( i ) ) );

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
        private Sequoiadb db = null;
        private DBCollection dbcl = null;
        private String url;
        private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        private String queryMatcher;

        private DataCRUD( int beginNo, int endNo, String url ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
            this.url = url;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            db = new Sequoiadb( url, "", "" );
            dbcl = db.getCollectionSpace( csName ).getCollection( clName );
            int recordNum = endNo - beginNo;
            System.out.println( "begin insert , coordUrl:" + url );
            insertRecords = DataSrcUtils.insertData( dbcl, recordNum, beginNo );
            System.out.println( "end insert , coordUrl:" + url );
            queryMatcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endNo + "}}]}";
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " begin insert check results , coordUrl:" + url );
            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " end insert check results , coordUrl:" + url );
        }

        @ExecuteOrder(step = 2)
        private void update() {
            int updateNum = 3000;
            int endCond = beginNo + updateNum;
            String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endCond + "}}]}";
            String modifier = "{$set:{testa:'updatetest" + beginNo + "'}}";
            System.out.println( "begin update , coordUrl:" + url );
            dbcl.update( matcher, modifier, "" );
            System.out.println( "end update , coordUrl:" + url );

            for ( int i = 0; i < updateNum; i++ ) {
                BSONObject obj = insertRecords.get( i );
                obj.put( "testa", "updatetest" + beginNo );
            }

            System.out.println( new Date() + " " + this.getClass().getName()
                    + " begin update check results , coordUrl:" + url );
            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " end update check results , coordUrl:" + url );
        }

        @ExecuteOrder(step = 3)
        private void remove() {
            int removeNum = 4000;
            int endCond = beginNo + removeNum;
            String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                    + endCond + "}}]}";
            System.out.println( "begin remove , coordUrl:" + url );
            dbcl.delete( matcher );
            System.out.println( "end remove , coordUrl:" + url );
            List< BSONObject > sublist = insertRecords.subList( 0, removeNum );
            insertRecords.removeAll( sublist );
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " begin remove check results , coordUrl:" + url );
            DataSrcUtils.checkRecords( dbcl, insertRecords, queryMatcher );
            System.out.println( new Date() + " " + this.getClass().getName()
                    + " end remove check results , coordUrl:" + url );
            db.close();
        }
    }

    private List< String > getCoordNodes( String groupName ) {
        List< String > nodes = new ArrayList< String >();
        ReplicaGroup rGroup = sdb.getReplicaGroup( groupName );
        BSONObject groupInfo = rGroup.getDetail();
        String hostName = null;
        String port = null;

        BasicBSONList nodesinfo = ( BasicBSONList ) groupInfo.get( "Group" );
        for ( int i = 0; i < nodesinfo.size(); ++i ) {

            BasicBSONObject nodeinfo = ( BasicBSONObject ) nodesinfo.get( i );
            hostName = nodeinfo.getString( "HostName" );
            port = ( ( BasicBSONObject ) ( ( BasicBSONList ) nodeinfo
                    .get( "Service" ) ).get( 0 ) ).getString( "Name" );
            nodes.add( hostName + ":" + port );
        }
        return nodes;
    }
}
