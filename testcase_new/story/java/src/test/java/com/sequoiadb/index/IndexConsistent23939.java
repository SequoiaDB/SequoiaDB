package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import com.sequoiadb.base.DBCursor;
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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23939:并发创建索引和删除切分表
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23939 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23939";
    private String srcGroupName;
    private String destGroupName;
    private int recsNum = 50000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "---Skip testCase.Current environment less than tow groups! " );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupNames.get( 0 );
        destGroupName = groupNames.get( 1 );
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        cl = cs.createCollection( clName, options );
        cl.split( srcGroupName, destGroupName, 50 );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23939";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName ) );
        es.addWorker( new DropCL() );

        es.run();

        Assert.assertFalse( cs.isCollectionExist( clName ) );
        IndexUtils.checkNoTask( sdb, SdbTestBase.csName, clName,
                "Create index" );
        checkNoSnapshotTask( sdb, "Create index", SdbTestBase.csName, clName );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex {
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
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorType() &&
                     e.getErrorType() != SDBError.SDB_TASK_HAS_CANCELED
                        .getErrorType() ) {
                    throw e;
                }
            }
        }
    }

    private class DropCL {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待2S内时间再删除cl
                int waitTime = new Random().nextInt( 2000 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
            }
        }
    }

    private void checkNoSnapshotTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );

        int times = 0;
        int sleepTime = 100;
        int maxWaitTimes = 20000;
        while ( true ) {
            DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                    null, null );
            BSONObject taskInfo = null;
            List< BSONObject > taskInfos = new ArrayList<>();
            int taskNum = 0;
            while ( cursor.hasNext() ) {
                taskInfo = cursor.getNext();
                taskInfos.add( taskInfo );
                taskNum++;
            }
            cursor.close();

            if ( taskNum == 0 ) {
                break;
            } else if ( times * sleepTime > maxWaitTimes ) {
                throw new Error( "waiting task time out! waitTimes="
                        + times * sleepTime + "\ntask=" + taskInfos );
            }
            try {
                Thread.sleep( sleepTime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            times++;
        }
    }
}
