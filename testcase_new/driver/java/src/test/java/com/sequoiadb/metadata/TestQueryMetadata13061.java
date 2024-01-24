package com.sequoiadb.metadata;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * Created by laojingtang on 17-10-25.
 */
public class TestQueryMetadata13061 extends SdbTestBase {

    private Sequoiadb sdb;
    private String coordAddr;
    private String commCSName;
    private String clName = "mycl";
    private DBCollection cl = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        sdb = new Sequoiadb( coordAddr, "", "" );
        if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
            sdb.createCollectionSpace( commCSName );
        }
        cs = sdb.getCollectionSpace( commCSName );

        if ( cs.isCollectionExist( clName ) )
            cl = cs.getCollection( clName );
        else
            cl = cs.createCollection( clName );
    }

    @Test
    public void test() {
        for ( int i = 0; i < 100; i++ ) {
            cl.insert( new BasicBSONObject( "a", 1 ).append( "b", "test" ) );
        }

        cl.createIndex( "a", "{a:1}", false, false );

        BSONObject matcher = ( BSONObject ) JSON
                .parse( "{'a':{'$gt':10,'$lt':20}}" );
        BSONObject orderby = ( BSONObject ) JSON.parse( "{'a':1}" );
        // hint is null
        DBCursor cursor = cl.getQueryMeta( matcher, orderby, null, 1, 10, 0 );
        Assert.assertEquals( cursor.getNext().get( "ScanType" ), "ixscan" );
        // hint is empty object
        cursor = cl.getQueryMeta( matcher, orderby, new BasicBSONObject(), 1,
                10, 0 );
        Assert.assertEquals( cursor.getNext().get( "ScanType" ), "ixscan" );
        // hint is {"":None}
        cursor = cl.getQueryMeta( matcher, orderby,
                ( BSONObject ) JSON.parse( "{'':null}" ), 1, 10, 0 );
        Assert.assertEquals( cursor.getNext().get( "ScanType" ), "tbscan" );
    }

    @AfterClass
    public void tearDown() {
        cs.dropCollection( clName );
        sdb.disconnect();
    }
}
