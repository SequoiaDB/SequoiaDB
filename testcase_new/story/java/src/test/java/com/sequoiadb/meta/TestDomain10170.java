package com.sequoiadb.meta;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.truncate.TruncateUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbTestException;

/**
 * @FileName:seqDB-10170: split时指定的group在域中不存在 1、创建域、CS（CS关联该域）
 *                        2、创建CL，指定组覆盖：group在域中；group存在但不在域中；group不存在
 *                        3、split操作，指定组覆盖：目标组/源组存在且在域中；组存在但不在域中；组不存在 4、检查操作结果
 * @Author linsuqiang
 * @Date 2016-12-19
 * @Version 1.00
 */
public class TestDomain10170 extends SdbTestBase {
    private static Sequoiadb sdb = null;
    private String domainName = "domain_10170";
    private String csName = "cs_10170";
    private String clName = "cl_10170";
    private String srcGroup = null;
    private String dstGroup = null;
    private String outGroup = null;
    private String inexistGroup = "usadeodsdff";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( TruncateUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone, skip testcase" );
        }
        if ( TruncateUtils.getDataGroups( sdb ).size() < 3 ) {
            throw new SkipException( "less then 3 groups, skip testcase" );
        }
        initGroups();
        createDomain();
        createCS();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            if ( sdb.isDomainExist( domainName ) ) {
                sdb.dropDomain( domainName );
            }
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( csName );
            DBCollection cl = createCL( cs );
            splitCL( cl );
            checkSplit( db );
        } finally {
            db.close();
        }
    }

    private ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        ArrayList< String > groupList = null;
        groupList = sdb.getReplicaGroupNames();
        groupList.remove( "SYSCatalogGroup" );
        groupList.remove( "SYSCoord" );
        groupList.remove( "SYSSpare" );
        return groupList;
    }

    private void initGroups() {
        ArrayList< String > dataGroupNames = null;
        dataGroupNames = getDataGroups( sdb );
        srcGroup = dataGroupNames.get( 0 );
        dstGroup = dataGroupNames.get( 1 );
        outGroup = dataGroupNames.get( 2 );
    }

    private void createDomain() {
        BSONObject option = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        groups.put( "0", srcGroup );
        groups.put( "1", dstGroup );
        option.put( "Groups", groups );
        sdb.createDomain( domainName, option );
    }

    private void createCS() {
        BSONObject option = new BasicBSONObject();
        option.put( "Domain", domainName );
        sdb.createCollectionSpace( csName, option );
    }

    private DBCollection createCL( CollectionSpace cs ) {
        DBCollection cl = null;
        BSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", ( BSONObject ) JSON.parse( "{a:1}" ) );
        option.put( "ShardingType", "hash" );
        // create a CL not in domain
        try {
            option.put( "Group", outGroup );
            cl = cs.createCollection( clName, option );
            throw new SdbTestException(
                    "creating CL shouldn't success, which's group is not in domain" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -216, e.getMessage() );
        }
        // create a CL in an inexistent group
        try {
            option.removeField( "Group" );
            option.put( "Group", inexistGroup );
            cl = cs.createCollection( clName, option );
            throw new SdbTestException(
                    "creating CL shouldn't success, which's group in an inexistent" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -154, e.getMessage() );
        }
        // create a CL in domain
        option.removeField( "Group" );
        option.put( "Group", srcGroup );
        cl = cs.createCollection( clName, option );
        return cl;
    }

    private void splitCL( DBCollection cl ) {
        // split (dst group not in domain)
        try {
            cl.split( srcGroup, outGroup, 50 );
            throw new SdbTestException(
                    "split shouldn't success, when dst group is not in domain " );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -216, e.getMessage() );
        }
        // split (dst group in an inexistent group)
        try {
            cl.split( srcGroup, inexistGroup, 50 );
            throw new SdbTestException(
                    "split shouldn't success, when dst group is inexistent " );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -154, e.getMessage() );
        }
        // split (dst group in domain)
        cl.split( srcGroup, dstGroup, 50 );
    }

    private void checkSplit( Sequoiadb db ) {
        // get CataInfo
        BSONObject option = new BasicBSONObject();
        option.put( "Name", csName + '.' + clName );
        DBCursor snapshot = db.getSnapshot( 8, option, null, null );
        BasicBSONList cataInfo = ( BasicBSONList ) snapshot.getNext()
                .get( "CataInfo" );
        snapshot.close();

        // justify source group catalog information
        BSONObject srcInfo = ( BSONObject ) cataInfo.get( 0 );
        int expSrcLowBound = 0;
        int expSrcUpBound = 2048;
        if ( !( ( ( String ) srcInfo.get( "GroupName" ) ).equals( srcGroup )
                && ( ( BasicBSONObject ) srcInfo.get( "LowBound" ) )
                        .getInt( "" ) == expSrcLowBound
                && ( ( BasicBSONObject ) srcInfo.get( "UpBound" ) )
                        .getInt( "" ) == expSrcUpBound ) ) {
            Assert.fail( "split fail: source group cataInfo is wrong" );
        }

        // justify destination group catalog information
        BSONObject dstInfo = ( BSONObject ) cataInfo.get( 1 );
        int expDstLowBound = 2048;
        int expDstUpBound = 4096;
        if ( !( ( ( String ) dstInfo.get( "GroupName" ) ).equals( dstGroup )
                && ( ( BasicBSONObject ) dstInfo.get( "LowBound" ) )
                        .getInt( "" ) == expDstLowBound
                && ( ( BasicBSONObject ) dstInfo.get( "UpBound" ) )
                        .getInt( "" ) == expDstUpBound ) ) {
            Assert.fail( "split fail: destination group cataInfo is wrong" );
        }
    }
}
