package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestIndex15933 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl15933";
    private String indexName = "aindex";

    @BeforeClass
    public void setUp() {
        this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() {
        createCL();
        this.cl.createIndex( this.indexName, "{\"a\":1}", true, false );
        BSONObject indexInfo = this.cl.getIndexInfo( this.indexName );
        BSONObject indexDefInfo = ( BSONObject ) indexInfo.get( "IndexDef" );
        // check index info
        Assert.assertEquals( indexDefInfo.get( "name" ), this.indexName );
        Assert.assertEquals( indexDefInfo.get( "key" ).toString(),
                "{ \"a\" : 1 }" );
        Assert.assertEquals( indexDefInfo.get( "unique" ), true );
        Assert.assertEquals( indexDefInfo.get( "enforced" ), false );
        // 不存在的索引
        try {
            this.cl.getIndexInfo( "csqindex" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -47 );
        }
        Assert.assertEquals( this.cl.isIndexExist( this.indexName ), true );
        Assert.assertEquals( this.cl.isIndexExist( "csqindex" ), false );
        DBCursor cur = null;
        cur = this.cl.getIndex( "csqindex" );
        Assert.assertNotNull( cur );
        cur.close();

        cur = this.cl.getIndex( null );
        List< String > actual = new ArrayList< String >();
        actual.add( this.indexName );
        actual.add( "$shard" );
        List< String > expected = new ArrayList< String >();
        while ( cur.hasNext() ) {
            BSONObject next = cur.getNext();
            indexDefInfo = ( BSONObject ) next.get( "IndexDef" );
            expected.add( indexDefInfo.get( "name" ).toString() );
        }
        Assert.assertEqualsNoOrder( actual.toArray(), expected.toArray() );
        cur.close();

        cur = this.cl.getIndex( this.indexName );
        while ( cur.hasNext() ) {
            BSONObject next = cur.getNext();
            indexDefInfo = ( BSONObject ) next.get( "IndexDef" );
            // check index info
            Assert.assertEquals( indexDefInfo.get( "name" ), this.indexName );
            Assert.assertEquals( indexDefInfo.get( "key" ).toString(),
                    "{ \"a\" : 1 }" );
            Assert.assertEquals( indexDefInfo.get( "unique" ), true );
            Assert.assertEquals( indexDefInfo.get( "enforced" ), false );
        }
        cur.close();

        this.cs.dropCollection( this.clName );
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
        try {
            String clOptions = "{ShardingKey:{a:1},ShardingType:'hash',Partition:1024,AutoIndexId:false}";
            BSONObject options = ( BSONObject ) JSON.parse( clOptions );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            this.cl = this.cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}
