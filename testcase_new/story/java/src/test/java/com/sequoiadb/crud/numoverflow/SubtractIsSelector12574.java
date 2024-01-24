package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: AdsIsSelector12574.java test content:Numeric value overflow for
 * many character using $subtract operation, and the $subtract is used as a
 * selector. testlink case:seqDB-12574
 * 
 * @author wuyan
 * @Date 2017.9.4
 * @version 1.00
 */

public class SubtractIsSelector12574 extends SdbTestBase {

    private String clName = "subtract_selector12574";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        if ( NumOverflowUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( NumOverflowUtils.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        String clOption = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true, StrictDataMode:false}";
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName, clOption );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'9223372036854775807'},"
                        + "'arr':[1,[1,{'$numberLong':'8223372036854775808'}],2],obj:{a:{b:4}}}" };

        NumOverflowUtils.insert( cl, records );
        splitCL();
    }

    @Test
    public void testSubtract() {
        try {
            String selector = "{no:{$subtract:1},tlong:{$subtract:-1000000000000000002},"
                    + "arr:{$subtract:-1000000000000000002},'obj.a.b':{$subtract:-2147483644},_id:{$include:0}}";
            String[] expRecords = {
                    "{'no':-2147483649,'tlong':{'$decimal':'10223372036854775809'},"
                            + "'arr':[1000000000000000003,null,1000000000000000004],obj:{a:{b:2147483648}}}" };
            NumOverflowUtils.multipleFieldOper( cl, selector, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "subtract is used as selector oper failed," + e.getMessage()
                            + e.getErrorCode() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void splitCL() {
        String sourceRGName = "";
        String targetRGName = "";
        try {
            sourceRGName = NumOverflowUtils.getSourceRGName( sdb,
                    SdbTestBase.csName, clName );
            targetRGName = NumOverflowUtils.getTarRgName( sdb, sourceRGName );
            int percent = 80;
            cl.split( sourceRGName, targetRGName, percent );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "split fail: sRG: " + sourceRGName + " targetRG:"
                            + targetRGName + e.getErrorCode()
                            + e.getMessage() );
        }
    }

}
