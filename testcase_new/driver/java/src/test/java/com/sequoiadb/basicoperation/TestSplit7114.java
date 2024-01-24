package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSplit7114.java test interface: splitAsync(String
 * sourceGroupName, String destGroupName, double percent) testlink
 * cases:seqDB-7114 testing strategy: 1.create hashcl 2.insert records 3.split
 * by percent,rg:30% 4.connect the sourceGroup and targetGroup,check the results
 * count
 * 
 * @author wuyan
 * @Date 2016.10.9
 * @version 1.00
 */
public class TestSplit7114 extends SdbTestBase {

    private String clName = "cl_7114";
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
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        String test = "{ShardingKey:{a:1},ShardingType:'hash',Partition:1024}";
        BSONObject options = ( BSONObject ) JSON.parse( test );
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public void splitCL() {
        try {
            sourceRGName = Commlib.getSourceRGName( sdb, SdbTestBase.csName,
                    clName );
            targetRGName = Commlib.getTarRgName( sdb, sourceRGName );
            double percent = 30;
            long taskID = -1;
            taskID = cl.splitAsync( sourceRGName, targetRGName, percent );
            Assert.assertTrue( taskID != -1,
                    "retrun incorrect taskID:" + taskID );
            long[] taskIDs = new long[ 1 ];
            taskIDs[ 0 ] = taskID;
            // we must use waitTasks to wait
            sdb.waitTasks( taskIDs );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "split fail:" + e.getErrorCode() + e.getMessage()
                            + "srcRGName:" + sourceRGName + " tarRGName:"
                            + targetRGName );
        }
    }

    /**
     * construct expected result values
     * 
     * @return expected result values,rg:["group1","{"":0}","{"":500}"]
     */
    private List< CataInfoItem > buildExpectResult() {
        List< CataInfoItem > cataInfo = new ArrayList< CataInfoItem >();
        CataInfoItem item = new CataInfoItem();
        item.groupName = sourceRGName;
        item.lowBound = 0;
        item.upBound = 717;

        cataInfo.add( item );
        item = new CataInfoItem();
        item.groupName = targetRGName;
        item.lowBound = 717;
        item.upBound = 1024;
        cataInfo.add( item );
        return cataInfo;
    }

    public void checkSplitResult() {
        String cond = String.format( "{Name:\"%s.%s\"}", SdbTestBase.csName,
                clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        List< CataInfoItem > cataInfo = buildExpectResult();
        while ( collections.hasNext() ) {
            BasicBSONObject doc = ( BasicBSONObject ) collections.getNext();
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            for ( int i = 0; i < cataInfo.size(); ++i ) {
                BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( i );
                String groupName = elem.getString( "GroupName" );
                BasicBSONObject obj = ( BasicBSONObject ) elem
                        .get( "LowBound" );
                int LowBound;
                if ( obj.containsField( "" ) ) {
                    LowBound = obj.getInt( "" );
                } else {
                    LowBound = obj.getInt( "partition" );
                }

                int UpBound;
                obj = ( BasicBSONObject ) elem.get( "UpBound" );
                if ( obj.containsField( "" ) ) {
                    UpBound = obj.getInt( "" );
                } else {
                    UpBound = obj.getInt( "partition" );
                }

                boolean compareResult = cataInfo.get( i ).Compare( groupName,
                        LowBound, UpBound );
                Assert.assertTrue( compareResult,
                        cataInfo.get( i ).toString() + "actResult:"
                                + "groupName:" + groupName + " LowBound:"
                                + LowBound + " UpBound:" + UpBound );
            }
        }
    }

    @Test
    public void testSplit7114() {
        Commlib.bulkInsert( cl );
        splitCL();
        checkSplitResult();
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
