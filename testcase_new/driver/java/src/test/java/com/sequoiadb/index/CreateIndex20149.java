package com.sequoiadb.index;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-20149:createIndex支持NotNull
 * @Author XiaoNi Huang
 * @Date 2019.11.4
 */

public class CreateIndex20149 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl20149";
    private String indexName = "idx";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        cl = cs.createCollection( clName );
    }

    @Test
    public void test() {
        // test: a is not null
        cl.insert( new BasicBSONObject( "a", 1 ) );
        cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "NotNull", true ) );
        Assert.assertEquals( cl.getCount(), 1 );
        cl.dropIndex( indexName );
        cl.truncate();

        // test: a is null
        cl.insert( new BasicBSONObject( "a", null ) );
        try {
            cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "NotNull", true ) );
            Assert.fail(
                    "index key is null, create index, expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -339 ) {
                throw e;
            }
        }
        cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "NotNull", false ) );
        Assert.assertEquals( cl.getCount(), 1 );
        cl.dropIndex( indexName );
        cl.truncate();

        // test: a does not exist
        cl.insert( new BasicBSONObject( "b", 1 ) );
        try {
            cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                    new BasicBSONObject( "NotNull", true ) );
            Assert.fail(
                    "index key does not exist, create index, expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -339 ) {
                throw e;
            }
        }
        cl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "NotNull", false ) );
        Assert.assertEquals( cl.getCount(), 1 );
        cl.dropIndex( indexName );
        cl.truncate();

        // test: old function
        cl.createIndex( indexName, new BasicBSONObject( "a", 1 ), true, true,
                1024 );
        cl.insert( new BasicBSONObject( "a", 1 ) );
        try {
            cl.insert( new BasicBSONObject( "a", 1 ) );
            Assert.fail(
                    "enforced index, insert duplicate index key, expect fail but success." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -38 ) {
                throw e;
            }
        }
        Assert.assertEquals( cl.getCount(), 1 );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }
}
