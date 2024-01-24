package com.sequoiadb.metaopr.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Domain;
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
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @FileName seqDB-2270: 更新domain时catalog主节点异常重启
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建domian，在域中添加两个数据组（如group1和group2），且设置AutoSplit参数为自动切分
 * 2、更新域属性（执行domain.alter（{Groups:[“group1”，“group3”}）），删除其中一个复制组，添加新复制组
 * （如group3） 3、更新域属性过程中catalog主节点异常重启（如执行kill -9杀掉节点进程，构造节点异常重启）
 * 3、查看domain更新结果和catalog主节点状态 4、节点启动成功后（查看节点进程存在）
 * 5、再次执行更新domain操作，并指定该domain创建CS/CL 6、向CL插入数据，查看数据是否正确落到该域对应的组内
 * 7、查看catalog主备节点上该domain信息是否正确
 */

public class AlterDomain2270 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String domNameBase = "domain_2270";
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
            NodeWrapper priNode = cataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 0 );
            TaskMgr mgr = new TaskMgr( faultTask );
            AlterDomainTask aTask = new AlterDomainTask();
            mgr.addTask( aTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            alterDomainAgain( db );
            checkGroupsByCreateCL( db );

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
            db = new Sequoiadb( coordUrl, "", "" );
            dropDomains( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class AlterDomainTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                for ( int i = 0; i < DOMAIN_NUM; i++ ) {
                    String domainName = domNameBase + "_" + i;
                    Domain domain = db.getDomain( domainName );
                    BSONObject option = new BasicBSONObject();
                    BSONObject groups = new BasicBSONList();
                    groups.put( "0", groupNames.get( 0 ) );
                    groups.put( "1", groupNames.get( 2 ) );
                    option.put( "Groups", groups );
                    domain.alterDomain( option );
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
            groups.put( "0", groupNames.get( 0 ) );
            groups.put( "1", groupNames.get( 1 ) );
            option.put( "Groups", groups );
            option.put( "AutoSplit", true );
            db.createDomain( domainName, option );
        }
    }

    private void alterDomainAgain( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            String domainName = domNameBase + "_" + i;
            Domain domain = db.getDomain( domainName );
            BSONObject option = new BasicBSONObject();
            BSONObject groups = new BasicBSONList();
            groups.put( "0", groupNames.get( 0 ) );
            groups.put( "1", groupNames.get( 2 ) );
            option.put( "Groups", groups );
            domain.alterDomain( option );
        }
    }

    private void checkGroupsByCreateCL( Sequoiadb db ) {
        String csName = "cs_2270";
        String domainName = domNameBase + "_" + 0;
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ Domain: '" + domainName + "' }" );
        CollectionSpace cs = db.createCollectionSpace( csName, option );
        String clName = "cl_2270";
        cs.createCollection( "cl_2270" );

        String clFullName = csName + "." + clName;
        DBCursor cursor = db.getSnapshot( 4, "{ Name: '" + clFullName + "' }",
                null, null );
        BasicBSONList details = ( BasicBSONList ) ( cursor.getNext() )
                .get( "Details" );
        cursor.close();

        List< String > expGroupNames = new ArrayList< String >();
        expGroupNames.add( groupNames.get( 0 ) );
        expGroupNames.add( groupNames.get( 2 ) );

        String actGroupName = ( String ) ( ( BSONObject ) details.get( 0 ) )
                .get( "GroupName" );

        if ( !( expGroupNames.contains( actGroupName ) ) ) {
            System.out.println( "actual: " + actGroupName );
            System.out.println( "expected: " + expGroupNames );
            Assert.fail(
                    "groups status is not expected. see the details on console. " );
        }

        db.dropCollectionSpace( csName );
    }

    private void checkListDomain( Sequoiadb db ) {
        BSONObject orderBy = ( BSONObject ) JSON.parse( "{ _id: 1 }" );
        DBCursor cursor = db.listDomains( null, null, orderBy, null );
        int i = 0;
        while ( cursor.hasNext() ) {
            BSONObject currDomain = cursor.getNext();
            // check Groups
            List< String > expGroupNames = new ArrayList< String >();
            expGroupNames.add( groupNames.get( 0 ) );
            expGroupNames.add( groupNames.get( 2 ) );

            BasicBSONList actGroups = ( BasicBSONList ) currDomain
                    .get( "Groups" );
            List< String > actGroupNames = new ArrayList< String >();
            for ( int j = 0; j < actGroups.size(); j++ ) {
                actGroupNames
                        .add( ( String ) ( ( BSONObject ) actGroups.get( j ) )
                                .get( "GroupName" ) );
            }

            if ( !( actGroupNames.containsAll( expGroupNames )
                    && expGroupNames.containsAll( actGroupNames ) ) ) {
                System.out.println( "actual: " + actGroupNames );
                System.out.println( "expected: " + expGroupNames );
                Assert.fail(
                        "groups status is not expected. see the details on console. " );
            }

            // check AutoSplit
            boolean autoSplitVal = ( boolean ) currDomain.get( "AutoSplit" );
            Assert.assertEquals( autoSplitVal, true,
                    currDomain.get( "Name" ) + ": AutoSplit" );
            i++;
        }
        int domainCnt = i;
        Assert.assertEquals( domainCnt, DOMAIN_NUM, "domain count" );
        cursor.close();
    }

    private void dropDomains( Sequoiadb db ) {
        for ( int i = 0; i < DOMAIN_NUM; i++ ) {
            String domainName = domNameBase + "_" + i;
            db.dropDomain( domainName );
        }
    }
}