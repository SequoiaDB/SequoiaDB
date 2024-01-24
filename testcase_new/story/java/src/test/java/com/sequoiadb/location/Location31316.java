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
 * @Description seqDB-31316: 集合同步一致性策略设置为节点优先
 * @Author TangTao
 * @Date 2023.05.04
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.05.04
 */
@Test(groups = "location")
public class Location31316 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private DBCollection dbcl1 = null;
    private DBCollection dbcl2 = null;
    private String csName = "cs_31316";
    private String clName1 = "cl_31316_1";
    private String clName2 = "cl_31316_2";
    private String primaryLocation = "guangzhou.nansha_31316";
    private String sameCityLocation = "guangzhou.panyu_31316";
    private String offsiteLocation2 = "shenzhen.nanshan_31316";
    private int recordNum = 100000;

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
        option1.put( "ReplSize", 2 );
        option1.put( "ConsistencyStrategy", 1 );
        option1.put( "Group", expandGroupName );
        dbcl1 = dbcs.createCollection( clName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "ReplSize", 3 );
        option2.put( "ConsistencyStrategy", 1 );
        option2.put( "Group", expandGroupName );
        dbcl2 = dbcs.createCollection( clName2, option2 );
    }

    @Test
    public void test() {
        String groupName = SdbTestBase.expandGroupName;
        ArrayList< BasicBSONObject > otherLocationNodes = LocationUtils
                .getGroupLocationSlaveNodes( sdb, groupName, primaryLocation );
        otherLocationNodes.addAll( LocationUtils.getGroupLocationNodes( sdb,
                groupName, sameCityLocation ) );
        otherLocationNodes.addAll( LocationUtils.getGroupLocationNodes( sdb,
                groupName, offsiteLocation2 ) );

        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl1,
                recordNum );
        LocationUtils.checkRecordSync( csName, clName1, recordNum,
                otherLocationNodes );

        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl2,
                recordNum );
        LocationUtils.checkRecordSync( csName, clName2, recordNum,
                otherLocationNodes );

        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl1, batchRecords1, orderBy );
        CommLib.checkRecords( dbcl2, batchRecords2, orderBy );
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
