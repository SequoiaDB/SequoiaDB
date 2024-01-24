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
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @version 1.0
 * @Description seqDB-31319:设置3个Location都具有亲和性，集合设置同步一致性策略
 * @Author TangTao
 * @Date 2023.05.04
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.04
 */
@Test(groups = "location")
public class Location31319 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private DBCollection dbcl3 = null;
    private DBCollection dbcl4 = null;
    private String csName = "cs_31319";
    private String clName1 = "cl_31319_1";
    private String clName2 = "cl_31319_2";
    private String clName3 = "cl_31319_3";
    private String clName4 = "cl_31319_4";
    private String primaryLocation = "guangzhou.nansha_31319";
    private String offsiteLocation1 = "guangzhou.panyu_31319";
    private String offsiteLocation2 = "guangzhou.huangpu_31319";
    private int recordNum = 100000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        LocationUtils.setTwoCityAndThreeLocation( sdb, expandGroupName,
                primaryLocation, offsiteLocation1, offsiteLocation2 );

        if ( !CommLib.isLSNConsistency( sdb, SdbTestBase.expandGroupName ) ) {
            Assert.fail( "LSN is not consistency" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace dbcs = sdb.createCollectionSpace( csName );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "ReplSize", 2 );
        option1.put( "ConsistencyStrategy", 2 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 2 );
        option2.put( "ConsistencyStrategy", 3 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );

        BasicBSONObject option3 = new BasicBSONObject();
        option3.put( "ReplSize", 3 );
        option3.put( "ConsistencyStrategy", 2 );
        option3.put( "Group", expandGroupName );
        dbcl3 = dbcs.createCollection( clName3, option3 );

        BasicBSONObject option4 = new BasicBSONObject();
        option4.put( "ReplSize", 3 );
        option4.put( "ConsistencyStrategy", 3 );
        option4.put( "Group", expandGroupName );
        dbcl4 = dbcs.createCollection( clName4, option4 );
    }

    @Test
    public void test() {
        String groupName = SdbTestBase.expandGroupName;
        ArrayList< BasicBSONObject > primaryLocationSlaveNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationNodes( sdb, groupName, offsiteLocation1 );
        otherLocationNodes.addAll( LocationUtils.getGroupLocationNodes( sdb,
                groupName, offsiteLocation2 ) );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        LocationUtils.checkRecordSync( csName, clName1, recordNum,
                otherLocationNodes );

        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        LocationUtils.checkRecordSync( csName, clName2, recordNum,
                primaryLocationSlaveNodes );

        List< BSONObject > batchRecords3 = CommLib.insertData( dbcl3,
                recordNum );
        LocationUtils.checkRecordSync( csName, clName3, recordNum,
                otherLocationNodes );
        LocationUtils.checkRecordSync( csName, clName3, recordNum,
                primaryLocationSlaveNodes );

        List< BSONObject > batchRecords4 = CommLib.insertData( dbcl4,
                recordNum );
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
        LocationUtils.cleanLocation( sdb, SdbTestBase.expandGroupName );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

}
