package com.sequoiadb.location.killnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.location.LocationUtils;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @Descreption seqDB-31783:同城双中心，指定Enforced为false，切主失败
 * @Author huanghaimei
 * @CreateDate 2023/6/1
 * @UpdateUser huanghaimei
 * @UpdateDate 2023/6/7
 * @UpdateRemark
 * @Version
 */
@Test(groups = "location")
public class Critical31783 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31783";
    private String clName1 = "cl_31783_1";
    private String primaryLocation = "guangzhou.nansha_31783";
    private String sameCityLocation = "guangzhou.panyu_31783";
    private int recordNum = 100000;

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
        LocationUtils.setTwoLocationInSameCity( sdb, expandGroupName,
                primaryLocation, sameCityLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", -1 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止备中心所有节点
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }
        CommLib.insertData( dbcl1, recordNum * 2 );

        group.start();

        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 1 );
        options1.put( "MaxKeepTime", 3 );
        options1.put( "Location", sameCityLocation );
        try {
            // 同城备中心启动Critical模式，不指定Enforced
            group.startCriticalMode( options1 );
        } catch ( BaseException e ) {
            int eCode = e.getErrorCode();
            if ( eCode != SDBError.SDB_TIMEOUT.getErrorCode() ) {
                throw e;
            }
        }

        options1.put( "Enforced", false );
        try {
            // 同城备中心启动Critical模式，指定Enforced为false
            group.startCriticalMode( options1 );
        } catch ( BaseException e ) {
            int eCode = e.getErrorCode();
            if ( eCode != SDBError.SDB_TIMEOUT.getErrorCode() ) {
                throw e;
            }
        }
        group.stopCriticalMode();
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
