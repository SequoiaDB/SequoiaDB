package com.sequoiadb.compress;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-7525:构建字典过程中磁盘满
 * @Author XiaoNi Huang
 * @Date 2020/05/11
 */

public class LzwDiskFull7525 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private String csName = "cs7525";
    // CS默认申请的磁盘空间足够CL插入足够量数据并生成字典，
    // 所以默认至少需要2~3个表插入大量数据把CS申请的那部分空间占用，已方便模拟构建字典过程磁盘满的场景
    // clNum 至少 3个
    private int clNum = 3;
    private String clNameBase = "cl7525_";
    // 准备数据
    private int recsNum = 400000;
    private int maxRecsNumForDict = 1000000;
    private List< BSONObject > newRecs = new ArrayList<>();

    @BeforeClass
    public void setUp() throws ReliabilityException {
        Sequoiadb db = null;
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed." );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            groupName = groupMgr.getAllDataGroupName().get( 0 );
            System.out.println( "cl group: " + groupName );

            // 清理CS
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }

            // 创建CS/CL
            CollectionSpace cs = db.createCollectionSpace( csName );
            for ( int j = 0; j < clNum; j++ ) {
                cs.createCollection( clNameBase + j,
                        new BasicBSONObject( "Group", groupName )
                                .append( "Compressed", true )
                                .append( "CompressionType", "lzw" ) );
            }

            // 往部分集合插入数据占用CS空间
            for ( int i = 0; i < clNum - 1; i++ ) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clNameBase + i );
                insertRecs( cl, recsNum, 0, false );
            }
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @Test
    public void test() throws InterruptedException, ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper mstNode = dataGroup.getMaster();

        FaultMakeTask faultTask = DiskFull.getFaultMakeTask( mstNode.hostName(),
                mstNode.dbPath(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        InsertTask insertTask = new InsertTask();
        mgr.addTask( insertTask );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
            Assert.fail( "checkBusiness occurs time out" );
        }

        // 检查字典未构建
        Assert.assertFalse( isDictCreated() );

        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl = db.getCollectionSpace( csName )
                    .getCollection( clNameBase + ( clNum - 1 ) );

            // 再次插入数据
            insertRecs( cl, maxRecsNumForDict, 0, false );

            // 检查字典已构建
            Assert.assertTrue( isDictCreated() );

            // 检查组内数据一致性
            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.dropCollectionSpace( csName );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    private class InsertTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clNameBase + ( clNum - 1 ) );
                insertRecs( cl, recsNum, 1, false );
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    /**
     * 
     * @param cl
     * @param recsNum
     *                            插入的记录数
     * @param flag
     *                            区分是哪个阶段插入的记录
     * @param checkDirCreated
     *                            是否检查字典已构建，不检查则插入recsNum条记录，检查则构建字典后就终止插入
     */
    private void insertRecs( DBCollection cl, int recsNum, int flag,
            boolean checkDirCreated ) {
        int batchRecsNum = 50000;
        for ( int k = 0; k < recsNum; k += batchRecsNum ) {
            if ( checkDirCreated ) {
                if ( this.isDictCreated() ) {
                    System.out.println(
                            "dictionary created, new recsNum = " + k );
                    break;
                }
            }

            for ( int i = k; i < k + batchRecsNum; i++ ) {
                newRecs.add( new BasicBSONObject( "atest", i )
                        .append( "btest", i ).append( "ctest", "test" + i )
                        .append( "dtest", "aaabbbccc测试哈哈abbaccbaacaacbacab" )
                        .append( "flag", flag ) );
            }
            try {
                cl.bulkInsert( newRecs, 0 );
            } catch ( BaseException e ) {
                System.out.println( "k = " + k );
                throw e;
            }
            newRecs.clear();
        }
    }

    private boolean isDictCreated() {
        Sequoiadb db = null;
        Sequoiadb nodeDB = null;
        boolean isCreated = false;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            nodeDB = db.getReplicaGroup( groupName ).getMaster().connect();
            DBCursor cursor = nodeDB.getSnapshot(
                    Sequoiadb.SDB_SNAP_COLLECTIONS,
                    new BasicBSONObject( "Name",
                            csName + "." + clNameBase + ( clNum - 1 ) ),
                    null, null );
            BasicBSONList info = ( BasicBSONList ) cursor.getNext()
                    .get( "Details" );
            BSONObject details = ( BSONObject ) info.get( 0 );
            isCreated = ( boolean ) details.get( "DictionaryCreated" );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
            if ( nodeDB != null ) {
                nodeDB.disconnect();
            }
        }
        return isCreated;
    }
}