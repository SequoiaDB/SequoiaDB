package com.sequoiadb.sequence.killnode;

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

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-23213:rename序列时，catalog主节点异常
 * @Author chenxiaodan
 * @Date 2021年6月3日
 */
public class Sequence23213 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private List< String > oldSeqNameList = new ArrayList<>();
    private List< String > newSeqNameList = new ArrayList<>();
    private String seq_old_name = "seq_23213";
    private String seq_new_name = "newSeq_23213";
    private int seqNum = 20;
    private int completeTimes = 0;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standAlone mode" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusiness return false" );
        }
        for ( int i = 0; i < seqNum; i++ ) {
            sdb.createSequence( seq_old_name + "_" + i, ( BSONObject ) JSON
                    .parse( "{'Increment':2,'Cycled':true}" ) );
            oldSeqNameList.add( seq_old_name + "_" + i );
            newSeqNameList.add( seq_new_name + "_" + i );
        }
    }

    @Test
    public void test() throws Exception {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper cataMaster = cataGroup.getMaster();
        // 建立并行任务
        TaskMgr task = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                cataMaster.hostName(), cataMaster.svcName(), 0 );
        Rename renameTask = new Rename();
        task.addTask( faultTask );
        task.addTask( renameTask );
        task.execute();
        Assert.assertTrue( task.isAllSuccess(), faultTask.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        for ( int i = 0; i < completeTimes; i++ ) {
            BSONObject expobj = new BasicBSONObject();
            expobj.put( "Name", newSeqNameList.get( i ) );
            List< BSONObject > exprList = new ArrayList<>();
            exprList.add( expobj );
            SequenceUtils.checkSequence( sdb, newSeqNameList.get( i ),
                    exprList );
        }
        for ( int i = completeTimes; i < seqNum; i++ ) {
            List< BSONObject > exprList2 = new ArrayList<>();
            exprList2.add( new BasicBSONObject() );
            SequenceUtils.checkSequence( sdb, oldSeqNameList.get( i ),
                    exprList2 );
        }
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < completeTimes; i++ ) {
                sdb.dropSequence( newSeqNameList.get( i ) );
            }
            for ( int i = completeTimes; i < seqNum; i++ ) {
                sdb.dropSequence( oldSeqNameList.get( i ) );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class Rename extends OperateTask {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                for ( int i = 0; i < oldSeqNameList.size(); i++ ) {
                    db.renameSequence( oldSeqNameList.get( i ),
                            newSeqNameList.get( i ) );
                    completeTimes++;
                    Thread.sleep( 10 );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            } finally {
                db.close();
            }
        }
    }
}
