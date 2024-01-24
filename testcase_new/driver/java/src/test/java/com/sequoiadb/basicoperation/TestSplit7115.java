package com.sequoiadb.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSplit7115.java test java client retrun fail,split two
 * times,then the last split is fail testlink cases:seqDB-7115
 * 
 * @author wuyan
 * @Date 2016.10.9
 * @version 1.00
 */
public class TestSplit7115 extends SdbTestBase {

    private String clName = "cl_7115";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    String sourceRGName;
    String targetRGName;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        if ( Commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( Commlib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        createCL();
    }

    public void createCL() {
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        String test = "{ShardingKey:{no:1},ShardingType:'range'}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
    }

    @DataProvider(name = "recordNumsProvider")
    public Object[][] recordNumsProvider() {
        return new Object[][] { { sourceRGName, 7 }, { targetRGName, 3 }, };
    }

    public void splitCL() {
        try {
            sourceRGName = Commlib.getSourceRGName( sdb, SdbTestBase.csName,
                    clName );
            targetRGName = Commlib.getTarRgName( sdb, sourceRGName );

            BSONObject cond = new BasicBSONObject();
            BSONObject endCond = new BasicBSONObject();
            cond.put( "no", 1 );
            endCond.put( "no", 3 );
            cl.split( sourceRGName, targetRGName, cond, endCond );
            cl.split( sourceRGName, targetRGName, cond, endCond );
        } catch ( BaseException e ) {
            // -176 exception error,ignore exceptions
            Assert.assertEquals( -176, e.getErrorCode(), e.getMessage() );
        }
    }

    @Test
    public void testSplit7115() {
        Commlib.bulkInsert( cl );
        splitCL();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }
}
