package com.sequoiadb.faulttolerance.killnode;

import java.util.ArrayList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-26621:默认容错级别，ReplSize为-1的集合写入数据时备节点异常
 * @Author liuli
 * @Date 2022.06.20
 * @UpdateAuthor liuli
 * @UpdateDate 2022.06.20
 * @version 1.10
 */
public class Faulttolerance26621 extends SdbTestBase {

    private String csName = "cs_26621";
    private String clName = "cl_26621";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private ArrayList< BSONObject > records = new ArrayList<>();

    @BeforeClass
    public void setUp() throws ReliabilityException {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        dbcl = cs.createCollection( clName,
                new BasicBSONObject( "ReplSize", -1 ).append( "Group",
                        groupName ) );

    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        // 构造插入数据
        int recsNum = 100000;
        for ( int j = 0; j < recsNum; j++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", j );
            record.put( "b", j );
            record.put( "order", j );
            records.add( record );
        }

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();
        TaskMgr mgr = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( slave.hostName(),
                slave.svcName(), 3 );

        mgr.addTask( faultTask );
        mgr.addTask( new Insert() );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        DBCursor cursor = dbcl.query( null, null, new BasicBSONObject( "a", 1 ),
                null );
        ArrayList< BSONObject > actRecords = getReadActList( cursor );
        Assert.assertEqualsNoOrder( records.toArray(), actRecords.toArray() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class Insert extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.bulkInsert( records );
            }
        }
    }

    public ArrayList< BSONObject > getReadActList( DBCursor cursor ) {
        ArrayList< BSONObject > actRList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            actRList.add( record );
        }
        cursor.close();
        return actRList;
    }
}
