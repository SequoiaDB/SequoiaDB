package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-33819:remotelocationconsistency设置为false，同城备中心故障
 * @Author liuli
 * @Date 2023.10.12
 * @UpdateAuthor liuli
 * @UpdateDate 2023.10.12
 * @version 1.10
 */
@Test(groups = "location")
public class Location33819 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private String csName = "cs_33819";
    private String clName1 = "cl_33819_1";
    private String clName2 = "cl_33819_2";
    private String primaryLocation = "guangzhou.nansha_33819";
    private String sameCityLocation = "guangzhou.panyu_33819";
    private String offsiteLocation = "shenzhan.nanshan_33819";
    private String expandGroupName;
    private int recordNum = 50000;

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

        // 两地三中心部署，不设置ActiveLocation
        expandGroupName = SdbTestBase.expandGroupNames.get( 0 );
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        group.setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        sdb.updateConfig(
                new BasicBSONObject( "remotelocationconsistency", false ) );
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        // 创建集合，ReplSize为5，ConsistencyStrategy为1
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 5 );
        option1.put( "ConsistencyStrategy", 1 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        // 创建集合，ReplSize为3，ConsistencyStrategy为1
        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 3 );
        option2.put( "ConsistencyStrategy", 1 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );
    }

    @Test
    public void test() throws ReliabilityException {
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName,
                        sameCityLocation );

        // 停止同城备中心所有节点
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }

        // remotelocationconsistency设置为false，不计算异地节点，认为存活节点为3
        // ReplSize为5的集合插入数据
        try {
            dbcl1.insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                    .getErrorCode() )
                throw ( e );
        }

        // ReplSize为3的集合插入数据
        List< BSONObject > batchRecords = CommLib.insertData( dbcl2,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl2, batchRecords, orderBy );

        // 恢复环境
        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        LocationUtils.cleanLocation( sdb,
                SdbTestBase.expandGroupNames.get( 0 ) );
        sdb.deleteConfig( new BasicBSONObject( "remotelocationconsistency", 1 ),
                new BasicBSONObject() );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
