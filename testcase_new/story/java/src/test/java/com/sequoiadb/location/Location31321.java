package com.sequoiadb.location;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.CommLib;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @version 1.0
 * @Description seqDB-31321:同城备中心与主中心具备亲和性，事务设置提交一致性策略
 * @Author TangTao
 * @Date 2023.05.04
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.04
 */
@Test(groups = "location")
public class Location31321 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private DBCollection dbcl4 = null;
    private String csName = "cs_31321";
    private String clName1 = "cl_31321_1";
    private String clName2 = "cl_31321_2";
    private String clName3 = "cl_31321_3";
    private String clName4 = "cl_31321_4";
    private String primaryLocation = "guangzhou.nansha_31321";
    private String sameCityLocation = "guangzhou.panyu_31321";
    private String offsiteLocation2 = "shenzhen.nanshan_31321";
    private int recordNum = 100000;
    String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, sameCityLocation, offsiteLocation2 );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "Group", expandGroupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );

        BasicBSONObject option4 = new BasicBSONObject();
        option4.put( "Group", expandGroupName );
        dbcl4 = dbcs.createCollection( clName4, option4 );
    }

    @Test
    public void test() {
        groupName = SdbTestBase.expandGroupName;
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, sameCityLocation );

        sdb.updateConfig(
                new BasicBSONObject( "transreplsize", 2 )
                        .append( "transconsistencystrategy", 2 ),
                new BasicBSONObject( "GroupName", groupName ) );
        sdb.beginTransaction();
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        sdb.commit();
        LocationUtils.checkRecordSync( csName, clName1, recordNum,
                otherLocationNodes );

        sdb.updateConfig(
                new BasicBSONObject( "transreplsize", 2 )
                        .append( "transconsistencystrategy", 3 ),
                new BasicBSONObject( "GroupName", groupName ) );
        sdb.beginTransaction();
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        sdb.commit();
        LocationUtils.checkRecordSync( csName, clName2, recordNum,
                primaryLocationSlaveNodes );

        sdb.updateConfig(
                new BasicBSONObject( "transreplsize", 3 )
                        .append( "transconsistencystrategy", 2 ),
                new BasicBSONObject( "GroupName", groupName ) );
        sdb.beginTransaction();
        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );
        sdb.commit();
        LocationUtils.checkRecordSync( csName, clName3, recordNum,
                otherLocationNodes );
        LocationUtils.checkRecordSync( csName, clName3, recordNum,
                primaryLocationSlaveNodes );

        sdb.updateConfig(
                new BasicBSONObject( "transreplsize", 3 )
                        .append( "transconsistencystrategy", 3 ),
                new BasicBSONObject( "GroupName", groupName ) );
        sdb.beginTransaction();
        List< BSONObject > batchRecords4 = CommLib.insertData( dbcl4,
                recordNum );
        sdb.commit();
        LocationUtils.checkRecordSync( csName, clName4, recordNum,
                primaryLocationSlaveNodes );
        LocationUtils.checkRecordSync( csName, clName4, recordNum,
                otherLocationNodes );

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
        CommLib.checkRecords( dbcl3, batchRecords3, orderBy );
        CommLib.checkRecords( dbcl4, batchRecords4, orderBy );
    }

    @AfterClass
    public void tearDown() {
        sdb.deleteConfig(
                new BasicBSONObject( "transreplsize", 1 )
                        .append( "transconsistencystrategy", 1 ),
                new BasicBSONObject( "GroupName", groupName ) );
        LocationUtils.cleanLocation( sdb, SdbTestBase.expandGroupName );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
