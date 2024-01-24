package com.sequoiadb.index.serial;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @descreption seqDB-24669:使用getIndexStat获取索引统计信息
 * @author YiPan
 * @date 2021/11/20
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */
public class TestGetIndexStat24669 extends SdbTestBase {
    private Sequoiadb sdb;
    private String csName = "cs_24669";
    private String clName = "cl_24669";
    private String idxName = "idx_24669";
    private int recordNum = 10;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace collectionSpace = sdb.createCollectionSpace( csName );
        cl = collectionSpace.createCollection( clName );
        for ( int i = 0; i < recordNum; i++ ) {
            cl.insert( "{a:" + i + "}" );
        }
        cl.createIndex( idxName, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "NotNull", false ) );
    }

    @Test
    public void test() {
        sdb.analyze();
        BSONObject indexStat = cl.getIndexStat( idxName );
        checkIndexStat( indexStat );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropCollectionSpace( csName );
        sdb.close();
    }

    private void checkIndexStat( BSONObject indexStat ) {
        // 校验索引统计信息
        Assert.assertEquals( indexStat.get( "Collection" ), cl.getFullName() );
        Assert.assertEquals( indexStat.get( "Index" ), idxName );
        Assert.assertEquals( indexStat.get( "Unique" ), false );
        Assert.assertEquals( indexStat.get( "KeyPattern" ).toString(),
                "{ \"a\" : 1 }" );
        Assert.assertNotEquals( indexStat.get( "TotalIndexLevels" ), 0 );
        Assert.assertNotEquals( indexStat.get( "TotalIndexPages" ), 0 );
        Assert.assertEquals( indexStat.get( "DistinctValNum" ).toString(),
                "[" + recordNum + "]" );
        Assert.assertEquals( indexStat.get( "MinValue" ).toString(),
                "{ \"a\" : 0 }" );
        Assert.assertEquals( indexStat.get( "MaxValue" ).toString(),
                "{ \"a\" : " + ( recordNum - 1 ) + " }" );
        Assert.assertEquals( indexStat.get( "NullFrac" ), 0 );
        Assert.assertEquals( indexStat.get( "UndefFrac" ), 0 );
        Assert.assertEquals( indexStat.get( "SampleRecords" ),
                Long.parseLong( recordNum + "" ) );
        Assert.assertEquals( indexStat.get( "TotalRecords" ),
                Long.parseLong( recordNum + "" ) );
        Assert.assertNotNull( indexStat.get( "StatTimestamp" ) );
    }
}