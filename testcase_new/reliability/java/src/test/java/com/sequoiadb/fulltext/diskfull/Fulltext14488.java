package com.sequoiadb.fulltext.diskfull;

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
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-14488:数据操作时备节点磁盘满
 * @Author zhaoyu
 * @Date 2019-08-12
 */

public class Fulltext14488 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clName = "cl14488";
    private String indexName = "index14488";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int insertNum = 10000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        groupMgr = GroupMgr.getInstance();
        groupName = groupMgr.getAllDataGroupName().get( 0 );
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        FullTextDBUtils.insertData( cl, insertNum );
        cl.createIndex( indexName, "{a:'text',b:'text',c:'text',d:'text'}",
                false, false );
    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();

        FaultMakeTask faultMakeTask = DiskFull
                .getFaultMakeTask( slave.hostName(), slave.dbPath(), 0, 600 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new InsertThread() );
        mgr.addTask( new UpdateThread() );
        // 删除记录，无法测试到磁盘满的情况，暂时不添加该线程
        // mgr.addTask(new DeleteThread());
        mgr.addTask( new QueryThread() );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( FullTextUtils.checkAdapter(), true );

        cl.insert( "{a:'text14488'}" );
        int expCount = ( int ) cl.getCount();
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, expCount ) );

        // lsn一致，索引数据可能会不一致，因此这个步骤需要放到最后，避免主节点产生了大量数据，再调用inspect工具导致的用例执行效率问题
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );

    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class InsertThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10; i++ ) {
                    FullTextDBUtils.insertData( cl, insertNum );
                }
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

    private class UpdateThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 500; i++ ) {
                    cl.update( null, "{$set:{a:'update14488" + i + "'}}",
                            null );
                }

            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

    // private class DeleteThread extends OperateTask {
    // @Override
    // public void exec() throws Exception {
    // try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
    // DBCollection cl = db.getCollectionSpace(csName).getCollection(clName);
    // cl.delete("");
    // } catch (BaseException e) {
    // e.printStackTrace();
    // }
    // }
    // }

    private class QueryThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor cursor = cl.query(
                        "{\"\":{$Text:{query:{match_all:{}}}}}", null, null,
                        null );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                }
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }
}
