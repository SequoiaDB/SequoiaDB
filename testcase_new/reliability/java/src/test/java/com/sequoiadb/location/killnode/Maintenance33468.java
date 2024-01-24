package com.sequoiadb.location.killnode;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.location.LocationUtils;
import org.bson.BSONObject;
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
 * @Description seqDB-33468:同城主备data节点异常，启动运维模式，节点未恢复手动停止运维模式
 * @Author TangTao
 * @Date 2023.09.25
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.09.25
 */
@Test(groups = "location")
public class Maintenance33468 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_33468";
    private String clName = "cl_33468";
    private String primaryLocation = "guangzhou.nansha_33468";
    private String sameCityLocation = "guangzhou.panyu_33468";
    private String offsiteLocation = "shenzhen.nanshan_33468";
    private int recordNum = 10000;

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
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > sameCityLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        // 停止主中心与同城备中心节点，开启运维模式
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );
        LocationUtils.stopNodeAbnormal( sdb, groupName, sameCityLocationNodes );
        group.startMaintenanceMode(
                new BasicBSONObject( "Location", primaryLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 2 ) );
        group.startMaintenanceMode(
                new BasicBSONObject( "Location", sameCityLocation )
                        .append( "MinKeepTime", 1 )
                        .append( "MaxKeepTime", 2 ) );

        LocationUtils.checkGroupInMaintenanceMode( sdb, groupName );

        // 创建集合插入数据并校验
        CollectionSpace dbcs = sdb.createCollectionSpace( csName );
        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 0 );
        option1.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option1 );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        // 节点未恢复正常停止运维模式，插入数据
        group.stopMaintenanceMode();
        try {
            dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );

            Assert.fail( "Insert operation expected failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                    .getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                            .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        ReplicaGroup group = sdb
                .getReplicaGroup( SdbTestBase.expandGroupNames.get( 0 ) );
        group.start();
        group.stopMaintenanceMode();
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
