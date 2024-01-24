package com.sequoiadb.metadata;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestMeta15153 extends SdbTestBase {
    /**
     * description: cl.enableSharding() a. cl.enableSharding() modify
     * shardingKey、shardingType、partition、EnsureShardingIndex and check result
     * b. disableSharding and check result testcase: 15153 author: chensiqin
     * date: 2018/04/26
     */
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clName = "cl15153";

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( coordAddr, "", "" );
        CommLib commLib = new CommLib();
        if ( commLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone" );
        }
    }

    @Test
    public void test15153() {
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        BSONObject option = new BasicBSONObject();
        option.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        option.put( "ShardingType", "hash" );
        option.put( "Partition", 2048 );
        option.put( "EnsureShardingIndex", true );

        cl.enableSharding( option );
        checkCLAlter( true, option );

        cl.disableSharding();
        checkCLAlter( false, new BasicBSONObject() );

        cs.dropCollection( clName );
    }

    private void checkCLAlter( boolean enableSharding, BSONObject expected ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject actual = new BasicBSONObject();
        DBCursor cur = null;
        if ( enableSharding ) {
            matcher.put( "Name", SdbTestBase.csName + "." + clName );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNotNull( cur.getNext() );
            actual = cur.getCurrent();
            Assert.assertEquals( actual.get( "ShardingKey" ).toString(),
                    expected.get( "ShardingKey" ).toString() );
            Assert.assertEquals( actual.get( "ShardingType" ).toString(),
                    expected.get( "ShardingType" ).toString() );
            Assert.assertEquals( actual.get( "Partition" ).toString(),
                    expected.get( "Partition" ).toString() );
            Assert.assertEquals( actual.get( "EnsureShardingIndex" ).toString(),
                    expected.get( "EnsureShardingIndex" ).toString() );
            cur.close();
        } else {
            matcher.put( "Name", SdbTestBase.csName + "." + clName );
            matcher.put( "EnsureShardingIndex", true );
            cur = sdb.getSnapshot( 8, matcher, null, null );
            Assert.assertNull( cur.getNext() );
            cur.close();
        }
    }

    @AfterClass
    public void tearDown() {
        if ( this.sdb != null ) {
            this.sdb.close();
        }
    }
}
