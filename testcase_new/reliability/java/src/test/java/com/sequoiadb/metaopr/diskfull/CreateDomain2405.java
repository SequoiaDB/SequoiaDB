package com.sequoiadb.metaopr.diskfull;

import java.util.Date;
import java.util.List;

import com.sequoiadb.metaopr.Utils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
 * @FileName seqDB-2405: 创建domain时catalog主节点所在服务器磁盘满
 * @Author linsuqiang
 * @Date 2017-04-14
 * @Version 1.00
 */

/*
 * 1、创建domian，构造脚本循环执行创建domain操作db.createDomain（）
 * 2、创建domian时catalog主节点所在主机磁盘满（构造主机磁盘满故障） 3、查看domain创建结果和catalog主节点状态
 * 4、恢复故障（清理磁盘空间） 5、再次创建domain，并指定该domain创建CS
 * 6、查看domain创建结果（执行db.listDomain命令查看domain/CS信息是否和实际一致
 * 7、查看catalog主备节点是否存在该domain相关信息
 */

public class CreateDomain2405 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String domNameBase = "domain_2405";
    private String csNameBase = "cs_2405";
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
            NodeWrapper priNode = cataGroup.getMaster();
            Sequoiadb cataDB = priNode.connect();
            DBCollection sysCataCL = cataDB.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );

            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    priNode.hostName(), SdbTestBase.reservedDir, 0, 10,
                    sysCataCL );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new CreateDomainTask() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createDomainAgain( db );
            operateOnDomain( db );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            checkListDomain( db );
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
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dropDomain( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class CreateDomainTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
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
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void createDomainAgain( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            try {
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
            } catch ( BaseException e ) {
                // -215 SDB_CAT_DOMAIN_EXIST 域已存在
                if ( e.getErrorCode() != -215 ) {
                    throw e;
                }
            }
        }
    }

    private void operateOnDomain( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            String csName = csNameBase + "_" + i;
            String domainName = domNameBase + "_" + i;
            BSONObject option = new BasicBSONObject();
            option.put( "Domain", domainName );
            db.createCollectionSpace( csName, option );
            db.dropCollectionSpace( csName );
        }
    }

    private void checkListDomain( Sequoiadb db ) {
        BSONObject orderBy = ( BSONObject ) JSON.parse( "{ _id: 1 }" );
        DBCursor cursor = db.listDomains( null, null, orderBy, null );
        int i = 0;
        while ( cursor.hasNext() ) {
            BSONObject currDomain = cursor.getNext();
            // check Groups
            int expGroupNum = i % groupNames.size() + 1;
            BasicBSONList actGroups = ( BasicBSONList ) currDomain
                    .get( "Groups" );
            int actGroupNum = actGroups.size();
            Assert.assertEquals( actGroupNum, expGroupNum,
                    currDomain.get( "Name" ) + ": groups count" );
            // check AutoSplit
            boolean expAttr = ( i % 2 == 0 );
            boolean actAttr = ( boolean ) currDomain.get( "AutoSplit" );
            Assert.assertEquals( actAttr, expAttr,
                    currDomain.get( "Name" ) + ": AutoSplit" );
            i++;
        }
        int domainCnt = i;
        Assert.assertEquals( domainCnt, DOMAIN_NUM, "domain count" );
        cursor.close();
    }

    private void dropDomain( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            String domainName = domNameBase + "_" + i;
            db.dropDomain( domainName );
        }
    }
}