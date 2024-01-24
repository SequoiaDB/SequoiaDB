package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23927:并发复制相同索引到相同子表
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23927 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection maincl;
    private String mainclName = "maincl_23927";
    private String subclName1 = "subcl_23927a";
    private String subclName2 = "subcl_23927b";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private String indexName = "testindex23927";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        maincl = createMainCLAndIndex( cs, mainclName, indexName );
        createAndAttachCL( cs, maincl, subclName1, subclName2 );
        int recsNum = 40000;
        insertRecords = IndexUtils.insertData( maincl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String indexName = "testindex23927";
        int threadNum = 10;
        for ( int i = 0; i < threadNum; i++ ) {
            es.addWorker( new CopyIndexThread( subclName1, indexName ) );
        }

        es.run();

        IndexUtils.checkRecords( maincl, insertRecords, "", "" );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                subclName1, indexName );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                indexName, false );
        checkExplain( maincl, subclName1, subclName2 );

        List< String > indexNames = new ArrayList<>();
        indexNames.add( indexName );
        List< String > subclNames = new ArrayList<>();
        subclNames.add( SdbTestBase.csName + "." + subclName1 );
        checkCopyTask( sdb, SdbTestBase.csName, mainclName, indexNames,
                subclNames );
        runSuccess = true;
    }

    private class CopyIndexThread {
        private String subclName;
        private String indexName;

        private CopyIndexThread( String subclName, String indexName ) {
            this.subclName = subclName;
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                cl.copyIndex( SdbTestBase.csName + "." + subclName, indexName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_CREATING
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( mainclName );
            }
        } finally {
            sdb.close();
        }
    }

    private DBCollection createMainCLAndIndex( CollectionSpace cs,
            String mainclName, String indexName ) {
        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );
        mainCL.createIndex( indexName, "{testb:1}", false, false );
        return mainCL;
    }

    private void createAndAttachCL( CollectionSpace cs, DBCollection mainCL,
            String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
        cs.createCollection( subclName2 );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
    }

    private void checkExplain( DBCollection dbcl, String subclName1,
            String subclName2 ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "testb", 2 );

        DBCursor explainCursor = dbcl.explain( matcher, null, null, null, 0, -1,
                0, null );
        while ( explainCursor.hasNext() ) {
            BSONObject record = explainCursor.getNext();
            BasicBSONList info = ( BasicBSONList ) record
                    .get( "SubCollections" );
            BSONObject subInfo = ( BSONObject ) info.get( 0 );
            String scanType = ( String ) subInfo.get( "ScanType" );
            String name = ( String ) subInfo.get( "Name" );
            String actIndexName = ( String ) subInfo.get( "IndexName" );
            if ( name.equals( SdbTestBase.csName + "." + subclName1 ) ) {
                Assert.assertEquals( scanType, "ixscan" );
                Assert.assertEquals( actIndexName, indexName );
            } else {
                Assert.assertEquals( name,
                        SdbTestBase.csName + "." + subclName2 );
                Assert.assertEquals( scanType, "tbscan" );
                Assert.assertEquals( actIndexName, "" );
            }
        }
        explainCursor.close();
    }

    // 只有一个copyIndex任务执行成功,可能出现串行重复copyindex产生空任务，校验忽略空任务
    private void checkCopyTask( Sequoiadb db, String csName, String mainclName,
            List< String > indexNames, List< String > subclNames ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + mainclName );
        matcher.put( "TaskTypeDesc", "Copy index" );
        matcher.put( "SucceededSubTasks", 1 );
        DBCursor cursor = db.listTasks( matcher, null, null, null );
        BSONObject taskInfo = new BasicBSONObject();
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        // 校验索引名
        List< String > actIndexNames = ( List< String > ) taskInfo
                .get( "IndexNames" );
        Collections.sort( actIndexNames );
        Collections.sort( indexNames );
        Assert.assertEquals( actIndexNames, indexNames,
                "actTaskInfo= " + taskInfo );

        // 校验结果状态码和结果码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        int expCode = 0;
        Assert.assertEquals( actResultCode, expCode,
                "actTaskInfo= " + taskInfo );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo= " + taskInfo );

        // 校验copy子表信息
        List< String > actSubCLNames = ( List< String > ) taskInfo
                .get( "CopyTo" );
        Collections.sort( actSubCLNames );
        Collections.sort( subclNames );
        Assert.assertEquals( actSubCLNames, subclNames );

        Assert.assertEquals( taskNum, 1,
                "copy index task num should be 1!" + taskInfo );
    }
}