package com.sequoiadb.autoincrement.killnode;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

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
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-15970: 新增/修改/删除自增字段时catalog主节点异常重启 预置条件：集合中已存在多个自增字段
 *           操作步骤：1.并发执行如下操作：a.新增自增字段 b.修改自增字段属性 c.删除自增字段d.插入记录，同时catalog主节点异常重启
 *           2.待集群稳定后，再次插入记录 预期结果：1.自增字段操作失败
 *           2.待集群正常后，catalog主备节点上集合及自增字段信息一致；正常插入记录
 * @Author zhaoyu
 * @Date 2018-11-02
 * @Version 1.00
 */

public class OperateAutoIncrement15970 extends SdbTestBase {
    private String clName = "cl_15970";
    private int autoIncrementNum = 10;
    private GroupMgr groupMgr = null;
    private int expectInsertNum = 0;

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
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper cataMaster = cataGroup.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataMaster.hostName(), cataMaster.svcName(), 10 );
            TaskMgr mgr = new TaskMgr();
            CreateAutoIncrementTask createTask = new CreateAutoIncrementTask();
            AlterAutoIncrementTask alterTask = new AlterAutoIncrementTask();
            DropAutoIncrementTask dropTask = new DropAutoIncrementTask();
            InsertDataTask insertTask = new InsertDataTask();
            mgr.addTask( createTask );
            mgr.addTask( alterTask );
            mgr.addTask( dropTask );
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
            checkCatalogConsistency( cataGroup );
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

    private class CreateAutoIncrementTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                for ( int i = autoIncrementNum; i < autoIncrementNum
                        * 2; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON.parse(
                            "{Field:'id" + i + "',CacheSize:1,AcquireSize:1}" );
                    cl.createAutoIncrement( obj );
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

    private class AlterAutoIncrementTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                for ( int i = 0; i < autoIncrementNum / 2; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{AutoIncrement:{Field:'id" + i
                                    + "',CacheSize:10,AcquireSize:10}}" );
                    cl.alterCollection( obj );
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

    private class DropAutoIncrementTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                for ( int i = autoIncrementNum
                        / 2; i < autoIncrementNum; i++ ) {
                    cl.dropAutoIncrement( "id" + i );
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
                    .parse( "{Field:'id" + i + "',CacheSize:1,AcquireSize:1}" );
            cl.createAutoIncrement( obj );
        }
    }

    private void checkCatalogConsistency( GroupWrapper cataGroup ) {
        List< String > cataUrls = cataGroup.getAllUrls();
        List< List< BSONObject > > results = new ArrayList< List< BSONObject > >();
        for ( String cataUrl : cataUrls ) {
            Sequoiadb cataDB = new Sequoiadb( cataUrl, "", "" );
            DBCursor cursor = cataDB.getCollectionSpace( "SYSGTS" )
                    .getCollection( "SEQUENCES" ).query();
            List< BSONObject > result = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                result.add( cursor.getNext() );
            }
            results.add( result );
            cursor.close();
            cataDB.close();
        }

        List< BSONObject > compareA = results.get( 0 );
        sortByName( compareA );
        for ( int i = 1; i < results.size(); i++ ) {
            List< BSONObject > compareB = results.get( i );
            sortByName( compareB );
            if ( !compareA.equals( compareB ) ) {
                System.out.println( cataUrls.get( 0 ) );
                System.out.println( compareA );
                System.out.println( cataUrls.get( i ) );
                System.out.println( compareB );
                Assert.fail( "data is different. see the detail in console" );
            }
        }
    }

    private void sortByName( List< BSONObject > list ) {
        Collections.sort( list, new Comparator< BSONObject >() {
            public int compare( BSONObject a, BSONObject b ) {
                String aName = ( String ) a.get( "Name" );
                String bName = ( String ) b.get( "Name" );
                return aName.compareTo( bName );
            }
        } );
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
            BasicBSONList record = ( BasicBSONList ) cursorS.getNext()
                    .get( "AutoIncrement" );
            for ( int i = 0; i < record.size(); i++ ) {
                BSONObject autoIncrement = ( BSONObject ) record.get( i );
                arrList.add( ( String ) autoIncrement.get( "Field" ) );
            }
        }

        // 在自增字段上创建唯一索引
        for ( int i = 0; i < arrList.size(); i++ ) {
            cl.createIndex( "id" + i, "{" + arrList.get( i ) + ":1}", true,
                    false );
        }

        // 比较记录是否包含自增字段
        DBCursor cursorR = cl.query( "{'mustCheckAutoIncrement':{$exists:1}}",
                null, null, null );
        while ( cursorR.hasNext() ) {
            BSONObject record = cursorR.getNext();
            for ( int i = 0; i < arrList.size(); i++ ) {
                boolean hasAutoIncrementField = record
                        .containsField( arrList.get( i ) );
                Assert.assertTrue( hasAutoIncrementField,
                        record + arrList.get( i ) );
            }
        }

        // 自增字段任务检查
        BSONObject filter = ( BSONObject ) JSON
                .parse( "{\"StatusDesc\":{$ne:\"Finish\"}}" );
        DBCursor cursorTasks = db.listTasks( filter, null, null, null );
        while ( cursorTasks.hasNext() ) {
            Assert.fail( "exists tasks ：" + cursorTasks.getNext().toString() );
        }

    }

}
