package com.sequoiadb.transaction.restartnode;

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
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-18645:正常重启数据主节点后，执行count查询
 * @author yinzhen
 * @date 2019-6-19
 *
 */
@Test(groups = { "rc", "rcauto" })
public class Transaction18645 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb db1;
    private Sequoiadb db2;
    private String clName = "cl18645";
    private GroupMgr groupMgr;
    private String groupName;
    private DBCollection cl;
    private List< BSONObject > expList;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( !groupMgr.checkBusiness( TransUtil.ClusterRestoreTimeOut ) ) {
            throw new SkipException( "GROUP ERROR" );
        }
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cl = sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( "idx18645", "{a:1}", false, false );
        expList = TransUtil.insertData( cl, 0, 10000 );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        if ( db1 != null ) {
            db1.close();
        }
        if ( db2 != null ) {
            db2.close();
        }
        if ( sdb != null ) {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
            sdb.close();
        }
    }

    @Test
    public void test() throws ReliabilityException {
        // 开启2个并发事务
        db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        db1.beginTransaction();
        DBCollection cl1 = db1.getCollectionSpace( csName )
                .getCollection( clName );

        // 事务1插入记录后为R2s
        TransUtil.insertData( cl1, 10000, 12000 );

        // 正常重启集合所在组的主节点
        NodeWrapper node = groupMgr.getGroupByName( groupName ).getMaster();
        FaultMakeTask task = NodeRestart.getFaultMakeTask( node, 5, 10 );
        TaskMgr taskMgr = new TaskMgr( task );
        taskMgr.execute();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN(
                TransUtil.ClusterRestoreTimeOut ), "GROUP ERROR" );

        // 待集群正常后，分别在非事务及事务中执行count/query查询，覆盖：表扫描、索引扫描
        // 非事务表扫描/索引扫描
        long actCount = cl.getCount( null,
                ( BSONObject ) JSON.parse( "{'':null}" ) );
        Assert.assertEquals( actCount, expList.size() );

        DBCursor cursor = cl.query( null, null, "{a:1}", "{'':null}" );
        List< BSONObject > actList = TransUtil.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actCount = cl.getCount( null,
                ( BSONObject ) JSON.parse( "{'':'idx18645'}" ) );
        Assert.assertEquals( actCount, expList.size() );

        cursor = cl.query( null, null, "{a:1}", "{'':'idx18645'}" );
        actList = TransUtil.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        // 事务表扫描/索引扫描
        db2.beginTransaction();
        DBCollection cl2 = db2.getCollectionSpace( csName )
                .getCollection( clName );
        actCount = cl2.getCount( null,
                ( BSONObject ) JSON.parse( "{'':null}" ) );
        Assert.assertEquals( actCount, expList.size() );

        cursor = cl2.query( null, null, "{a:1}", "{'':null}" );
        actList = TransUtil.getReadActList( cursor );
        Assert.assertEquals( actList, expList );

        actCount = cl2.getCount( null,
                ( BSONObject ) JSON.parse( "{'':'idx18645'}" ) );
        Assert.assertEquals( actCount, expList.size() );

        cursor = cl2.query( null, null, "{a:1}", "{'':'idx18645'}" );
        actList = TransUtil.getReadActList( cursor );
        Assert.assertEquals( actList, expList );
        cursor.close();
    }
}
