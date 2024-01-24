package com.sequoiadb.autoincrement.killnode;

import java.util.ArrayList;

import org.bson.BSONObject;
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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-15974: 不指定自增字段插入时coord主节点异常重启
 *           预置条件：集合中已存在自增字段且CacheSize及AcquireSize均设置不为1 操作步骤：
 *           1.不指定自增字段插入记录，同时coord异常 2.待节点正常后，不指定自增字段继续插入记录
 *           预期结果：1.coord异常后，插入失败，错误信息正确
 *           2.coord异常时本地缓存未使用完的序列值会丢掉，重新从catalog上获取新的序列值插入记录，值正确，主备节点数据一致
 * @Author zhaoyu
 * @Date 2018-11-05
 * @Version 1.00
 */
public class Insert15974 extends SdbTestBase {
    private String clName = "cl_15974";
    private int autoIncrementNum = 5;
    private GroupMgr groupMgr = null;
    private int expectInsertNum = 0;
    private int cacheSize = 1000000;
    private long acquireSize = 100000;

    @BeforeClass
    public void setUp() {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            DBCollection scl = sdb.getCollectionSpace( csName )
                    .createCollection( clName );
            createAutoIncrement( scl, autoIncrementNum );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    SdbTestBase.coordUrl.split( ":" )[ 0 ],
                    SdbTestBase.coordUrl.split( ":" )[ 1 ], 1 );
            TaskMgr mgr = new TaskMgr();
            InsertDataTask insertTask = new InsertDataTask();
            mgr.addTask( insertTask );
            mgr.addTask( faultTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            insertData( cl, 100 );
            checkResult( db, expectInsertNum );

        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }

    }

    private class InsertDataTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                for ( int i = 0; i < 10000; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{a:" + i + "}" );
                    cl.insert( obj );
                    expectInsertNum++;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    public void createAutoIncrement( DBCollection cl, int autoIncrementNum ) {
        for ( int i = 0; i < autoIncrementNum; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{Field:'id" + i + "',CacheSize:" + cacheSize
                            + ",AcquireSize:" + acquireSize + "}" );
            cl.createAutoIncrement( obj );
        }
    }

    public void insertData( DBCollection cl, int insertNum ) {
        ArrayList< BSONObject > arrList = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{mustCheckAutoIncrement:" + i + "}" );
            arrList.add( obj );
        }
        cl.insert( arrList );
        expectInsertNum += insertNum;
    }

    public void checkResult( Sequoiadb db, int expectNum ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );

        // 校验记录数
        int count = ( int ) cl.getCount();
        if ( count != expectNum && count != expectNum + 1 ) {
            Assert.fail( "expect:" + expectNum + "or " + expectNum + 1
                    + ",but actual:" + count );
        }

        // 获取自增字段
        DBCursor cursorS = db.getSnapshot( 8,
                ( BSONObject ) JSON
                        .parse( "{Name:'" + csName + "." + clName + "'}" ),
                null, null );
        ArrayList< String > arrList = new ArrayList< String >();
        while ( cursorS.hasNext() ) {
            BSONObject record = cursorS.getNext();
            BasicBSONList autoIncrements = ( BasicBSONList ) record
                    .get( "AutoIncrement" );
            for ( int i = 0; i < autoIncrements.size(); i++ ) {
                BSONObject autoIncrement = ( BSONObject ) autoIncrements
                        .get( i );
                arrList.add( ( String ) autoIncrement.get( "Field" ) );
            }
        }

        // 在自增字段上创建唯一索引
        for ( int i = 0; i < arrList.size(); i++ ) {
            cl.createIndex( "id" + i, "{" + arrList.get( i ) + ":1}", true,
                    false );
        }

        // 比较记录自增字段值的正确性
        DBCursor cursorR = cl.query( "{'mustCheckAutoIncrement':{$exists:1}}",
                null, "{'id0':1}", null );
        int increment = 1;
        while ( cursorR.hasNext() ) {
            BSONObject record = cursorR.getNext();
            for ( int i = 0; i < arrList.size(); i++ ) {
                long autoIncrementValue = ( long ) record
                        .get( arrList.get( i ) );
                // 考虑到记录还未插入，coord已经重启的情况，因此自增字段的值可能为从1开始递增
                if ( autoIncrementValue != ( acquireSize + increment )
                        && autoIncrementValue != increment ) {
                    Assert.fail( "expect:" + acquireSize + increment
                            + "or 1,but actual:" + autoIncrementValue );
                }
            }
            increment++;
        }
    }
}
