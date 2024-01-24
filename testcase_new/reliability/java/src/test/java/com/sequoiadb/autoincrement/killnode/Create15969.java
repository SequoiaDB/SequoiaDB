package com.sequoiadb.autoincrement.killnode;

/**
 * @FileName:seqDB-15969：指定自增字段创建集合时catalog整组异常重启 
 * 测试步骤：1.指定自增字段创建集合时catalog组异常重启2.待集群状态正常后，检查结果 
 * 预期结果：1.集合创建失败，2.待集群正常后，catalog主备节点上集合及自增字段信息一致；
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
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

public class Create15969 extends SdbTestBase {
    private String clName = "cl_15969";
    private String fieldName = "id";
    private GroupMgr groupMgr = null;
    private List< DBCollection > cls = new ArrayList< DBCollection >();

    @BeforeClass
    public void setUp() {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
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
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            List< String > cataAllUrls = cataGroup.getAllUrls();
            FaultMakeTask faultTask1 = KillNode.getFaultMakeTask(
                    cataAllUrls.get( 0 ).split( ":" )[ 0 ],
                    cataAllUrls.get( 0 ).split( ":" )[ 1 ], 10 );
            FaultMakeTask faultTask2 = KillNode.getFaultMakeTask(
                    cataAllUrls.get( 1 ).split( ":" )[ 0 ],
                    cataAllUrls.get( 1 ).split( ":" )[ 1 ], 10 );
            FaultMakeTask faultTask3 = KillNode.getFaultMakeTask(
                    cataAllUrls.get( 2 ).split( ":" )[ 0 ],
                    cataAllUrls.get( 2 ).split( ":" )[ 1 ], 10 );
            TaskMgr mgr = new TaskMgr();
            CreateTask createTask = new CreateTask();
            mgr.addTask( createTask );
            mgr.addTask( faultTask1 );
            mgr.addTask( faultTask2 );
            mgr.addTask( faultTask3 );
            mgr.execute();

            // faultTask.start();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            checkCatalogConsistency( cataGroup );

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

    private class CreateTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                for ( int i = 0; i < 50; i++ ) {
                    cls.add( cs.createCollection( clName + i,
                            ( BSONObject ) JSON.parse( "{AutoIncrement:{Field:'"
                                    + fieldName + i + "'}}" ) ) );
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

    @AfterClass
    public void tearDown() {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        for ( int i = 0; i < 50; i++ ) {
            if ( sdb.getCollectionSpace( csName )
                    .isCollectionExist( clName + i ) ) {
                sdb.getCollectionSpace( csName ).dropCollection( clName + i );
            }
        }
        sdb.close();
    }
}
