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
 * FileName: DropDomainAndKillPrimaryCatalog2272.java test content:when delete
 * domain, kill -9 the catalog group master node testlink case:seqDB-2272
 * 
 * @author wuyan
 * @Date 2017.4.25
 * @version 1.00
 */

public class DropDomainAndKillPrimaryCatalog2272 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private boolean clearFlag = false;
    private String preDomainName = "domain_2272";
    private final int DOMAIN_NUM = 500;
    private int count = 0;

    @BeforeClass
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createDomains();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper masterNode = cataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    masterNode.hostName(), masterNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DropDomainTask dTask = new DropDomainTask();
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 120S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            // check result
            checkDropDomainResult();
            Utils.checkConsistency( groupMgr );

            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException | InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( !clearFlag ) {
                throw new SkipException( "to save environment" );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropDomainTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                String domainName = "";
                for ( int i = 0; i < DOMAIN_NUM; i++ ) {
                    domainName = preDomainName + "_" + i;
                    db.dropDomain( domainName );
                    count++;
                }
            } catch ( BaseException e ) {
                int successDropNums = count;
                System.out.println(
                        "the drop domain num is =" + successDropNums );
            }
        }
    }

    private void createDomains() {
        try {
            List< String > groupNames = groupMgr.getAllDataGroupName();
            BSONObject option = new BasicBSONObject();
            BSONObject groups = new BasicBSONList();
            for ( int i = 0; i < groupNames.size(); i++ ) {
                groups.put( "" + i, groupNames.get( i ) );
            }
            option.put( "Groups", groups );
            option.put( "AutoSplit", true );
            for ( int i = 0; i < DOMAIN_NUM; i++ ) {
                String domainName = preDomainName + "_" + i;
                sdb.createDomain( domainName, option );
            }
        } catch ( BaseException e ) {
            Assert.fail( "create domain failed, errMsg:" + e.getMessage() );
        }

    }

    /**
     * check the result of drop domain the result: 1. to drop domain
     * success,drop the same domain again failed 2. to drop domain fail,drop the
     * same domain again successfully 3. to drop domain fail,the actual drop
     * domain successfully did not return, drop the same domain failed again
     */
    private void checkDropDomainResult() {
        if ( DOMAIN_NUM == count ) {
            try {
                String sameDomainName = preDomainName + "_" + ( count - 1 );
                sdb.dropDomain( sameDomainName );
                Assert.fail( "drop the same Domain should be fail" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -214 ) { // -214:SDB_CAT_DOMAIN_NOT_EXIST
                    e.printStackTrace();
                    Assert.fail( "expErrno=-214, actErrno= " + e.getErrorCode()
                            + ", " + e.getMessage() );
                }
            }
        } else {
            for ( int i = count; i < DOMAIN_NUM; i++ ) {
                String sameDomainName = preDomainName + "_" + i;
                if ( i == count ) {
                    try {
                        sdb.dropDomain( sameDomainName );
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != -214 ) {
                            e.printStackTrace();
                            Assert.fail( "expErrno=-214, actErrno= "
                                    + e.getErrorCode() + ", "
                                    + e.getMessage() );
                        }
                    }
                } else {
                    try {
                        sdb.dropDomain( sameDomainName );
                    } catch ( BaseException e ) {
                        e.printStackTrace();
                        Assert.fail( e.getMessage() );
                    }

                }
            }
        }
        checkListDomain();
    }

    private void checkListDomain() {
        DBCursor cursor = sdb.listDomains( null, null, null, null );
        if ( cursor.hasNext() ) {
            Assert.fail( "domain exist" );
        }
        cursor.close();
    }
}