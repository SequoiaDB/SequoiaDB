package com.sequoiadb.faulttolerance.diskfull;

import java.util.ArrayList;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22213 停止组内一个备节点后启动该节点，在replSize=0、-1、4、1的集合中插入数据
 * @author luweikang
 * @date 2020年6月5日
 */
public class Faulttolerance22213 extends SdbTestBase {

    private String csName = "cs22213";
    private String clName = "cl22213_";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private boolean shutoff = false;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private DBCollection cl3 = null;
    private DBCollection cl4 = null;

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

        cs = sdb.createCollectionSpace( csName );
        cl1 = cs.createCollection( clName + 0, ( BSONObject ) JSON
                .parse( "{'Group': '" + groupName + "', 'ReplSize': 0}" ) );
        cl2 = cs.createCollection( clName + 1, ( BSONObject ) JSON
                .parse( "{'Group': '" + groupName + "', 'ReplSize': -1}" ) );
        cl3 = cs.createCollection( clName + 2, ( BSONObject ) JSON
                .parse( "{'Group': '" + groupName + "', 'ReplSize': 2}" ) );
        cl4 = cs.createCollection( clName + 3, ( BSONObject ) JSON
                .parse( "{'Group': '" + groupName + "', 'ReplSize': 1}" ) );

    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new Insert( cl1.getName(), true ) );
        mgr.addTask( new Insert( cl2.getName(), false ) );
        mgr.addTask( new Insert( cl3.getName(), false ) );
        mgr.addTask( new Insert( cl4.getName(), false ) );
        mgr.addTask( new TestStopNode() );
        mgr.execute();
        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        try {
            cl1.insert( "{a: 1}" );
            Assert.fail( "the cl insert record with 0 replsize shuold fail." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -105 ) {
                throw e;
            }
        }
        cl2.insert( "{a: 1}" );
        cl3.insert( "{a: 1}" );
        cl4.insert( "{a: 1}" );

        sdb.getReplicaGroup( groupName ).start();

        try {
            cl1.insert( "{a: 1}" );
            Assert.fail( "the cl insert record with 0 replsize shuold fail." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -105 ) {
                throw e;
            }
        }
        cl2.insert( "{a: 1}" );
        cl3.insert( "{a: 1}" );
        cl4.insert( "{a: 1}" );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        cl1.insert( "{a: 1}" );
        cl2.insert( "{a: 1}" );
        cl3.insert( "{a: 1}" );
        cl4.insert( "{a: 1}" );
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

        private String clName = null;
        private boolean catchErr = false;

        public Insert( String clName, boolean catchErr ) {
            this.clName = clName;
            this.catchErr = catchErr;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clName );
                for ( int i = 0; i < 3000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 100; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqefiuokdjf" );
                        records.add( record );
                    }
                    cl.insert( records );
                }
            } catch ( BaseException e ) {
                if ( !catchErr ) {
                    throw e;
                } else if ( e.getErrorCode() != -105
                        && e.getErrorCode() != -252 ) {
                    throw e;
                }
            }
        }
    }

    class TestStopNode extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 60000 );
                db.getReplicaGroup( groupName ).getSlave().stop();
            }
        }
    }

}
