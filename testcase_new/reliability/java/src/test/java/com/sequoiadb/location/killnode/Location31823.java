package com.sequoiadb.location.killnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.location.LocationUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BasicBSONObject;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @version 1.0
 * @Description seqDB-31823:catalog并发设置不同Location为ActiveLocation
 * @Author TangTao
 * @Date 2023.06.02
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.02
 */
@Test(groups = "location")
public class Location31823 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private String primaryLocation = "guangzhou.nansha_31823";
    private String sameCityLocation = "guangzhou.panyu_31823";
    private String offsiteLocation = "shenzhen.nanshan_31823";
    private int successCount = 0;
    private String cataGroupName = "SYSCatalogGroup";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        ReplicaGroup group = sdb.getReplicaGroup( cataGroupName );
        group.getSlave( 1 ).setLocation( primaryLocation );
        group.getSlave( 2 ).setLocation( sameCityLocation );
        group.getSlave( 3 ).setLocation( offsiteLocation );

        // 并发设置ActiveLocation
        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < 10; i++ ) {
            mgr.addTask( new setAcLocation( cataGroupName, sameCityLocation ) );
            mgr.addTask( new setAcLocation( cataGroupName, offsiteLocation ) );
        }

        mgr.execute();

        System.out.println( "successCount:" + successCount );
        BasicBSONObject detail = ( BasicBSONObject ) group.getDetail();
        System.out.println(
                "ActiveLocation:" + detail.getString( "ActiveLocation" ) );
        Assert.assertTrue( successCount <= 20,
                "successCount must less than total" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class setAcLocation extends OperateTask {
        String groupName;
        String location;

        private setAcLocation( String groupName, String location ) {
            this.groupName = groupName;
            this.location = location;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                ReplicaGroup group = db.getReplicaGroup( groupName );
                group.setActiveLocation( location );
                successCount++;
            }
        }
    }
}
