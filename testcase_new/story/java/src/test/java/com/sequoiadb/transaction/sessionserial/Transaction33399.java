package com.sequoiadb.transaction.sessionserial;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.transaction.TransUtils;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @version 1.10
 * @Description seqDB-33399:事务中查询和切主并发，提交事务
 * @Author liuli
 * @Date 2023.09.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.16
 */
@Test(groups = "ru")
public class Transaction33399 extends SdbTestBase {

    private Sequoiadb sdb;
    private Sequoiadb db1;
    private CollectionSpace dbcs;
    private List< String > groupNames = new ArrayList<>();
    private String tclName = "testcl_33399";
    private String clName = "cl_33399";
    private int startId = 0;
    private int stopId = 1000;
    private int insertValue = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }

        groupNames = CommLib.getDataGroupNames( sdb );

        dbcs = sdb.getCollectionSpace( SdbTestBase.csName );

        // 创建集合
        BasicBSONObject options = new BasicBSONObject();
        options.put( "Group", groupNames.get( 0 ) );
        DBCollection dbcl = dbcs.createCollection( clName, null );

        TransUtils.insertDatas( dbcl, startId, stopId, insertValue );
    }

    @Test
    public void test() throws Exception {
        // 事务并发线程不关闭连接，直接使用用例连接的Sdb
        ThreadExecutor es = new ThreadExecutor();
        Query query = new Query();
        Reelect reelect = new Reelect();
        es.addWorker( query );
        es.addWorker( reelect );
        es.run();

        // 创建集合，指定ReplSize为0
        BasicBSONObject options = new BasicBSONObject();
        options.put( "Group", groupNames.get( 0 ) );
        options.put( "ReplSize", 0 );
        DBCollection dbcl = dbcs.createCollection( tclName, options );

        // 集合插入数据
        dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
    }

    @AfterClass
    public void tearDown() {
        if ( db1 != null ) {
            db1.close();
        }
        try {
            if ( dbcs.isCollectionExist( clName ) ) {
                dbcs.dropCollection( clName );
            }
            if ( dbcs.isCollectionExist( tclName ) ) {
                dbcs.dropCollection( tclName );
            }
        } finally {
            sdb.close();
        }
    }

    private class Query extends ResultStore {
        private DBCollection dbcl = null;

        @ExecuteOrder(step = 1)
        private void beginTransaction() {
            db1.beginTransaction();
            dbcl = db1.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void test() {
            DBCursor cursor = dbcl.query();
            while ( cursor.hasNext() ) {
                cursor.getNext();
            }
            cursor.close();
        }

        @ExecuteOrder(step = 3)
        private void commit() {
            try {
                db1.commit();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class Reelect extends ResultStore {

        @ExecuteOrder(step = 2)
        private void test() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms内，防止reelect每次都先执行
                int waitTime = new Random().nextInt( 500 );
                try {
                    Thread.sleep( waitTime );
                } catch ( InterruptedException e ) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                ReplicaGroup group = sdb.getReplicaGroup( groupNames.get( 0 ) );
                group.reelect();
            }
        }
    }
}
