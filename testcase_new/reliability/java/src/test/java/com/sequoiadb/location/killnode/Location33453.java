package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.location.LocationUtils;

/**
 * @Description seqDB-33453:备中启动运维模式，手动停止运维模式
 * @Author liuli
 * @Date 2023.09.22
 * @UpdateAuthor liuli
 * @UpdateDate 2023.09.22
 * @version 1.10
 */
@Test(groups = "location")
public class Location33453 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private DBCollection dbcl = null;
    private String csName = "cs_33453";
    private String clName = "cl_33453";
    private String primaryLocation = "guangzhou.nansha_33453";
    private String sameCityLocation = "guangzhou.panyu_33453";
    private String offsiteLocation = "shenzhan.nanshan_33453";
    private String expandGroupName;
    private int recordNum = 1000;

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

        expandGroupName = SdbTestBase.expandGroupNames.get( 0 );
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        if ( !CommLib.isLSNConsistency( sdb, expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option = new BasicBSONObject();
        option.put( "Group", expandGroupName );
        option.put( "ReplSize", 0 );
        dbcl = dbcs.createCollection( clName, option );
    }

    @Test
    public void test() throws ReliabilityException {
        // 指定同城备中启动运维模式
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, expandGroupName,
                        sameCityLocation );
        ReplicaGroup group = sdb.getReplicaGroup( expandGroupName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "MinKeepTime", 10 );
        options.put( "MaxKeepTime", 20 );
        options.put( "Location", sameCityLocation );
        group.startMaintenanceMode( options );

        // 停止启动运维模式的节点
        for ( BasicBSONObject sameCityLocationNode : sameCityLocationNodes ) {
            String nodeName = sameCityLocationNode.getString( "hostName" ) + ":"
                    + sameCityLocationNode.getString( "svcName" );
            group.getNode( nodeName ).stop();
        }

        // 集合插入数据
        List< BSONObject > batchRecords = CommLib.insertData( dbcl, recordNum );

        // 解除运维模式，节点未恢复
        group.stopMaintenanceMode();

        // SEQUOIADBMAINSTREAM-9976
        // 立即插入数据会成功，增加一个sleep
        try {
            Thread.sleep( 3000 );
        } catch ( InterruptedException e ) {
            throw new RuntimeException( e );
        }

        // 集合插入数据
        try {
            dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                    .getErrorCode() ) {
                throw e;
            }
        }

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords, orderBy );

        group.start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupName ).start();
        sdb.getReplicaGroup( expandGroupName ).stopMaintenanceMode();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}
