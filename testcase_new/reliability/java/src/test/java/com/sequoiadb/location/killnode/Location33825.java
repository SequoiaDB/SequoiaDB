package com.sequoiadb.location.killnode;

import java.util.ArrayList;

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
 * @Description seqDB-33825:remotelocationconsistency为true，异地节点异常后不满足ReplSize
 * @Author liuli
 * @Date 2023.10.12
 * @UpdateAuthor liuli
 * @UpdateDate 2023.10.12
 * @version 1.10
 */
@Test(groups = "location")
public class Location33825 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl = null;
    private String csName = "cs_33825";
    private String clName = "cl_33825";
    private String primaryLocation = "guangzhou.nansha_33825";
    private String sameCityLocation = "guangzhou.panyu_33825";
    private String offsiteLocation = "shenzhan.nanshan_33825";
    private String expandGroupName;

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

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        // 创建集合，ReplSize为5，ConsistencyStrategy为1
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 6 );
        option1.put( "ConsistencyStrategy", 1 );
        option1.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option1 );
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

        // remotelocationconsistency设置为true，异地节点参与同步一致性计算
        // ReplSize为6的集合插入数据
        try {
            dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                    .getErrorCode() )
                throw ( e );
        }

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
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
