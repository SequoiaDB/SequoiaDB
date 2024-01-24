package com.sequoiadb.metaopr.killnode;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.metaopr.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @FileName seqDB-2273: 删除domain时catalog备节点异常重启
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
 * 2、执行删除domian操作（构造脚本循环执行删除多个domain操作） 3、删除domain时catalog备节点异常重启（如执行kill
 * -9杀掉节点进程，构造节点异常重启） 3、查看domain信息和catalog组内节点状态 4、节点启动成功后（查看节点进程存在）
 * 5、再次执行删除domain操作 6、查看domain信息（执行db.listDomain命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该domain相关信息
 */

public class DropDomain2273 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String domNameBase = "domain_2273";
    private static final int DOMAIN_NUM = 1000;
    private List< String > groupNames = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            groupNames = groupMgr.getAllDataGroupName();
            db = new Sequoiadb( coordUrl, "", "" );
            createDomains( db );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper slvNode = cataGroup.getSlave();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    slvNode.hostName(), slvNode.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DropDomainTask dTask = new DropDomainTask();
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dropDomainAgain( db );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            checkListDomain( db );
            // groupMgr.refresh() ;
            cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
            Utils.checkConsistency( groupMgr );
            runSuccess = true;
        } catch ( ReliabilityException | InterruptedException e ) {
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
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class DropDomainTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                for ( int i = 0; i < DOMAIN_NUM; i++ ) {
                    String domainName = domNameBase + "_" + i;
                    db.dropDomain( domainName );
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void createDomains( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            String domainName = domNameBase + "_" + i;
            BSONObject option = new BasicBSONObject();
            BSONObject groups = new BasicBSONList();
            int groupNum = i % groupNames.size() + 1;
            for ( int j = 0; j < groupNum; j++ ) {
                groups.put( "" + j, groupNames.get( j ) );
            }
            option.put( "Groups", groups );
            option.put( "AutoSplit", ( i % 2 == 0 ) );
            db.createDomain( domainName, option );
        }
    }

    private void dropDomainAgain( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            try {
                String domainName = domNameBase + "_" + i;
                db.dropDomain( domainName );
            } catch ( BaseException e ) {
                // -214 SDB_CAT_DOMAIN_NOT_EXIST 域不存在
                if ( e.getErrorCode() != -214 ) {
                    throw e;
                }
            }
        }
    }

    private void checkListDomain( Sequoiadb db ) {
        DBCursor cursor = db.listDomains( null, null, null, null );
        if ( cursor.hasNext() ) {
            Assert.fail( "domain remains" );
        }
        cursor.close();
    }
}