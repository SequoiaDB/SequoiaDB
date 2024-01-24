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
 * @Description seqDB-31782:主中心异常，超过MaxKeepTime节点未恢复，自动停止Critical模式
 * @Author TangTao
 * @Date 2023.05.26
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.26
 */
@Test(groups = "location")
public class Critical31782 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31782";
    private String clName = "cl_31782";
    private String primaryLocation = "guangzhou.nansha_31782";
    private String sameCityLocation = "guangzhou.panyu_31782";
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
        LocationUtils.setTwoLocationInSameCity( sdb, expandGroupName,
                primaryLocation, sameCityLocation );
        sdb.getReplicaGroup( expandGroupName )
                .setActiveLocation( primaryLocation );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 0 );
        option1.put( "Group", expandGroupName );
        dbcl = dbcs.createCollection( clName, option1 );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        String groupName = SdbTestBase.expandGroupName;
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        ArrayList< BasicBSONObject > primaryLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, primaryLocation );

        // 主中心异常停止，然后stop节点模拟故障无法启动
        LocationUtils.stopNodeAbnormal( sdb, groupName, primaryLocationNodes );

        // 同城备中心启动Critical模式
        BasicBSONObject options1 = new BasicBSONObject();
        options1.put( "MinKeepTime", 1 );
        options1.put( "MaxKeepTime", 3 );
        options1.put( "Location", sameCityLocation );
        group.startCriticalMode( options1 );
        Date startTime = new Date();

        CommLib.insertData( dbcl, recordNum );

        // 等待超过MaxKeepTime，自动停止Critical模式
        LocationUtils.validateWaitTime( startTime, 4 );
        LocationUtils.checkGroupStopCriticalMode( sdb, groupName );

        // 插入数据并校验错误信息
        try {
            dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
            Assert.fail( "Insert operation expected failed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 集群环境恢复
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
