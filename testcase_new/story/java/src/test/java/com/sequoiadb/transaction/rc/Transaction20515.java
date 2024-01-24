package com.sequoiadb.transaction.rc;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import com.sequoiadb.transaction.TransUtils;

/**
 * @testcase seqDB-20515:存在事务操作，事务外切分卡住，事务提交后，读记录符合RR隔离级别
 * @date updated 2020-05-18
 * @author Lena,moodify zhaoyu 2020.2.27
 * 
 */
@Test(groups = "rc")
public class Transaction20515 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String clName = "cl20515";
    private DBCollection cl = null;
    private CollectionSpace cs;
    private String srcGroup;
    private String desGroup;
    private List< BSONObject > expList = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() {
        sdb = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone!" );
        }
        List< String > groupsNames = CommLib.getDataGroupNames( sdb );
        if ( groupsNames.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroup = groupsNames.get( 0 );
        desGroup = groupsNames.get( 1 );
        cs = sdb.getCollectionSpace( csName );
    }

    @Test
    public void transCommit() throws Exception {
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'b':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        cl.createIndex( "a", "{a:1}", false, false );
        expList.clear();
        expList = TransUtils.insertRandomDatas( cl, 0, 6 );

        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        Sequoiadb db3 = null;
        Sequoiadb db4 = null;
        Sequoiadb db5 = null;
        try {
            // 开启读事务T1
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            db1.beginTransaction();

            // 开启写事务T2,更新记录
            db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            db2.beginTransaction();
            cl2.update( "", "{$inc:{a:1}}", null );

            // 其他连接上执行切分
            Split split = new Split();
            split.start();

            // 避免切分任务未启动,循环检测切分任务,超时时间10s
            boolean isTaskExist = false;
            for ( int i = 0; i < 10; i++ ) {
                Thread.sleep( 1000 );
                BSONObject matcher = new BasicBSONObject( "Name", csName + "." + clName );
                matcher.put( "TaskType", 0 ); // split task
                DBCursor cursor = sdb.listTasks( matcher, null, null, null );
                while ( cursor.hasNext() ) {
                    isTaskExist = true;
                    break;
                }
                cursor.close();
            }
            Assert.assertTrue( isTaskExist );
            Assert.assertEquals(
                    TransUtils.getSplitTaskStatus( sdb, csName + "." + clName ),
                    1 );

            // 开启读事务T3
            db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl3 = db3.getCollectionSpace( csName )
                    .getCollection( clName );
            db3.beginTransaction();

            // T1读记录
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':'a'}", expList );

            // T2读记录
            List< BSONObject > expList1 = TransUtils.getIncDatas( 0, 6, 1 );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // T3读记录
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':'a'}", expList );

            // 提交写事务T2
            db2.commit();

            // 校验切分任务
            Assert.assertTrue( split.isSuccess(), split.getErrorMsg() );

            // 非事务读记录
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // T1读记录
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // T3 读记录
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // 开启事务T4读记录
            db4 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl4 = db4.getCollectionSpace( csName )
                    .getCollection( clName );
            db4.beginTransaction();
            TransUtils.queryAndCheck( cl4, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl4, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // 开启写事务写记录
            db5 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl5 = db5.getCollectionSpace( csName )
                    .getCollection( clName );
            db5.beginTransaction();
            cl5.update( "", "{$inc:{a:1}}", null );
            db5.commit();

            // 集合中记录正确
            expList1.clear();
            expList1 = TransUtils.getIncDatas( 0, 6, 2 );
            TransUtils.queryAndCheck( cl, null, "{a:1}", "{'':'a'}", expList1 );

        } finally {
            if ( db1 != null && !db1.isClosed() ) {
                db1.rollback();
                db1.close();
            }
            if ( db2 != null && !db2.isClosed() ) {
                db2.rollback();
                db2.close();
            }
            if ( db3 != null && !db3.isClosed() ) {
                db3.rollback();
                db3.close();
            }
            if ( db4 != null && !db4.isClosed() ) {
                db4.rollback();
                db4.close();
            }
            if ( db5 != null && !db5.isClosed() ) {
                db5.rollback();
                db5.close();
            }

            cs.dropCollection( clName );
        }

    }

    @Test
    public void transRollback() throws Exception {
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'b':1},ShardingType:'range',Group:'"
                                + srcGroup + "'}" ) );
        cl.createIndex( "a", "{a:1}", false, false );
        expList.clear();
        expList = TransUtils.insertRandomDatas( cl, 0, 6 );

        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        Sequoiadb db3 = null;
        Sequoiadb db4 = null;
        Sequoiadb db5 = null;
        try {
            // 开启读事务T1
            db1 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            db1.beginTransaction();

            // 开启写事务T2,更新记录
            db2 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            db2.beginTransaction();
            cl2.update( "", "{$inc:{a:1}}", null );

            // 其他连接上执行切分
            Split split = new Split();
            split.start();

            // 避免切分任务未启动,循环检测切分任务,超时时间10s
            boolean isTaskExist = false;
            for ( int i = 0; i < 10; i++ ) {
                Thread.sleep( 1000 );
                BSONObject matcher = new BasicBSONObject( "Name", csName + "." + clName );
                matcher.put( "TaskType", 0 ); // split task
                DBCursor cursor = sdb.listTasks( matcher, null, null, null );
                while ( cursor.hasNext() ) {
                    isTaskExist = true;
                    break;
                }
                cursor.close();
            }
            Assert.assertTrue( isTaskExist );
            Assert.assertEquals(
                    TransUtils.getSplitTaskStatus( sdb, csName + "." + clName ),
                    1 );

            // 开启读事务T3
            db3 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl3 = db3.getCollectionSpace( csName )
                    .getCollection( clName );
            db3.beginTransaction();

            // T1读记录
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':'a'}", expList );

            // T2读记录
            List< BSONObject > expList1 = TransUtils.getIncDatas( 0, 6, 1 );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':null}",
                    expList1 );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':'a'}",
                    expList1 );

            // T3读记录
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':'a'}", expList );

            // 提交写事务T2
            db2.rollback();

            // 校验切分任务
            Assert.assertTrue( split.isSuccess(), split.getErrorMsg() );

            // 非事务读记录
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl2, null, "{a:1}", "{'':'a'}", expList );

            // T1读记录
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl1, null, "{a:1}", "{'':'a'}", expList );

            // T3 读记录
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl3, null, "{a:1}", "{'':'a'}", expList );

            // 开启事务T4读记录
            db4 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl4 = db4.getCollectionSpace( csName )
                    .getCollection( clName );
            db4.beginTransaction();
            TransUtils.queryAndCheck( cl4, null, "{a:1}", "{'':null}",
                    expList );
            TransUtils.queryAndCheck( cl4, null, "{a:1}", "{'':'a'}", expList );

            // 开启写事务写记录
            db5 = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
            DBCollection cl5 = db5.getCollectionSpace( csName )
                    .getCollection( clName );
            db5.beginTransaction();
            cl5.update( "", "{$inc:{a:1}}", null );
            db5.commit();

            // 集合中记录正确
            expList1.clear();
            expList1 = TransUtils.getIncDatas( 0, 6, 1 );
            TransUtils.queryAndCheck( cl, null, "{a:1}", "{'':'a'}", expList1 );

        } finally {
            if ( db1 != null && !db1.isClosed() ) {
                db1.rollback();
                db1.close();
            }
            if ( db2 != null && !db2.isClosed() ) {
                db2.rollback();
                db2.close();
            }
            if ( db3 != null && !db3.isClosed() ) {
                db3.rollback();
                db3.close();
            }
            if ( db4 != null && !db4.isClosed() ) {
                db4.rollback();
                db4.close();
            }
            if ( db5 != null && !db5.isClosed() ) {
                db5.rollback();
                db5.close();
            }

            cs.dropCollection( clName );
        }

    }

    @AfterClass
    public void tearDown() {
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    public class Split extends SdbThreadBase {
        Sequoiadb db = TransUtils.getRandomSequoiadb( SdbTestBase.testGroup );
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );

        @Override
        public void exec() throws BaseException {
            try {
                cl.split( srcGroup, desGroup, 50 );
            } finally {
                db.close();
            }

        }
    }

}
