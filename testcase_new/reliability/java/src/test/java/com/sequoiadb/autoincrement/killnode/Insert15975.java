package com.sequoiadb.autoincrement.killnode;

/**
 * @FileName:seqDB-15975： CacheSize及AcquireSize均设置为1，不指定自增字段插入时catalog主节点异常重启 
 * 预置条件：集合中已存在自增字段且CacheSize及AcquireSize均设置为1
 * 测试步骤：1.不指定自增字段连多个coord节点插入记录，同时catalog主节点异常 2.待节点正常后，不指定自增字段继续插入记录
 * 预期结果：1.catalog异常后，插入失败，错误信息正确 2.记录插入成功，自增字段值从起始到结束值唯一且递增，值正确，主备节点数据一致
 * @Author zhaoxiaoni
 * @Date 2018-11-16
 * @Version 1.00
 */
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import org.bson.BSONObject;
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

public class Insert15975 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl_15975";
    private GroupMgr groupMgr = null;
    private int cacheSize = 1;
    private int acquireSize = 1;
    private List< String > coordNodes = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{AutoIncrement:{Field:'id',CacheSize:" + cacheSize
                                + ",AcquireSize:" + acquireSize + "}}" ) );
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            GroupWrapper coordGroup = groupMgr.getGroupByName( "SYSCoord" );
            coordNodes = coordGroup.getAllUrls();
            if ( coordNodes.size() < 2 ) {
                throw new SkipException( "skip one coordNode" );
            }
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        DBCollection cl = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper cataMaster = cataGroup.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    cataMaster.hostName(), cataMaster.svcName(), 1 );
            TaskMgr mgr = new TaskMgr();
            InsertTask insertTask0 = new InsertTask( coordNodes.get( 0 ) );
            InsertTask insertTask1 = new InsertTask( coordNodes.get( 1 ) );
            mgr.addTask( insertTask0 );
            mgr.addTask( insertTask1 );
            mgr.addTask( faultTask );
            mgr.execute();

            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            checkCatalogConsistency( cataGroup );
            insertData( cl, 100 );

            long count = ( long ) cl.getCount();
            if ( count < 100 ) {
                Assert.fail( "records count error!" );
            }
        } catch ( ReliabilityException e ) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        ;
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        if ( !sdb.isClosed() ) {
            sdb.close();
        }
    }

    private class InsertTask extends OperateTask {
        private String coordNode;

        public InsertTask( String coordNode ) {
            this.coordNode = coordNode;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( coordNode, "", "" );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                for ( int i = 0; i < 10000; i++ ) {
                    BSONObject obj = ( BSONObject ) JSON
                            .parse( "{a:" + i + "}" );
                    cl.insert( obj );
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
        List< BSONObject > arrList = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{mustCheckAutoIncrement:" + i + "}" );
            arrList.add( obj );
        }
        cl.insert( arrList );
    }

}
