package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.testcommon.CommLib;
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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23967:取消创建索引任务
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23967 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_23967";
    private int recsNum = 50000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone." );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        String indexName = "testindex23967";
        CreateIndex createIndex = new CreateIndex( indexName );
        CancelIndexTask cancelIndex = new CancelIndexTask( indexName );
        es.addWorker( createIndex );
        es.addWorker( cancelIndex );
        es.run();

        // check results
        if ( createIndex.getRetCode() != 0 ) {
            Assert.assertEquals( cancelIndex.getRetCode(), 0 );
            Assert.assertEquals( createIndex.getRetCode(),
                    SDBError.SDB_TASK_HAS_CANCELED.getErrorCode() );
            int resultCode = -243;
            checkIndexTask( sdb, "Create index", SdbTestBase.csName, clName,
                    indexName, resultCode );
            IndexUtils.checkIndexConsistent( sdb, csName, clName, indexName,
                    false );
        } else {
            // 如果任务已执行完成，则取消失败，校验执行完成结果
            Assert.assertEquals( cancelIndex.getRetCode(),
                    SDBError.SDB_TASK_ALREADY_FINISHED.getErrorCode() );
            Assert.assertEquals( createIndex.getRetCode(), 0 );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    clName, indexName );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                    indexName, true );
        }
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex extends ResultStore {
        private String indexName;

        private CreateIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{testno:1}", false, false );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class CancelIndexTask extends ResultStore {
        private String indexName;

        private CancelIndexTask( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待2S内时间再取消,覆蓋任务ready和running两种状态
                int waitTime = new Random().nextInt( 2000 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                long taskId = getIndexTaskId( indexName );
                db.cancelTask( taskId, true );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private long getIndexTaskId( String indexName ) {
        int times = 0;
        int sleepTime = 10;
        int maxWaitTimes = 20000;
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", SdbTestBase.csName + '.' + clName );
        matcher.put( "TaskTypeDesc", "Create index" );
        matcher.put( "IndexName", indexName );
        long taskId = 0;
        do {
            DBCursor cursor = sdb.listTasks( matcher, null, null, null );
            BSONObject taskInfo = null;
            while ( cursor.hasNext() ) {
                taskInfo = cursor.getNext();
                taskId = ( long ) taskInfo.get( "TaskID" );
            }
            cursor.close();
            try {
                Thread.sleep( sleepTime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            times++;
            if ( times * sleepTime > maxWaitTimes ) {
                throw new Error( "waiting task time out! waitTimes="
                        + times * sleepTime + "\ntask=" + taskInfo.toString() );
            }
        } while ( taskId == 0 );
        return taskId;
    }

    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        matcher.put( "ResultCode", resultCode );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();
        Assert.assertEquals( taskNum, 1,
                "index task num should be 1!" + taskInfo );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status,
                "check status error! --taskinfo=" + taskInfo );

        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode,
                "check resultcode error! actResult=" + actResultCode
                        + "\n--taskinfo=" + taskInfo );
    }
}