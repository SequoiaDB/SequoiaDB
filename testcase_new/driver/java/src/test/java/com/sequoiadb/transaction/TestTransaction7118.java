package com.sequoiadb.transaction;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-7118:������񲢷���ͬʱ����/ɾ��cl����ͬ��¼���ύ����
 * @Author chensiqin
 * @Date 2016-09-19
 */

public class TestTransaction7118 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb sdb2;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7118";
    private String commCSName;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            this.sdb2 = new Sequoiadb( coordAddr, "", "" );
            if ( !this.sdb.isCollectionSpaceExist( this.commCSName ) ) {
                try {
                    this.cs = this.sdb.createCollectionSpace( this.commCSName );
                } catch ( BaseException e ) {
                    Assert.assertEquals( -33, e.getErrorCode(),
                            e.getMessage() );
                }
            } else {
                this.cs = this.sdb.getCollectionSpace( this.commCSName );
            }
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.cl = this.cs.createCollection( clName );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestTransaction7118 setUp error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestTransaction7118 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    @Test
    public void testTrans7118() {
        if ( !Util.isCluster( this.sdb ) ) {
            return;
        }
        try {
            BSONObject insertBson = new BasicBSONObject();
            insertBson.put( "_id", 0 );
            insertBson.put( "age", 21 );
            insertBson.put( "name", "Sequoiadb" );
            this.cl.insert( insertBson );
            this.sdb.beginTransaction();
            this.sdb2.beginTransaction();
            DBCollection cl1 = this.sdb.getCollectionSpace( commCSName )
                    .getCollection( clName );
            DBCollection cl2 = this.sdb2.getCollectionSpace( commCSName )
                    .getCollection( clName );
            DBQuery query = new DBQuery();
            query.setModifier( ( BSONObject ) JSON.parse( "{$set:{age:22}}" ) );
            cl1.update( query );
            try {
                cl2.delete( "{_id:{$et:0}}" );
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -13 );
            }
            this.sdb.commit();
            DBCursor cursor = this.cl.query();
            BSONObject actualBson = new BasicBSONObject();
            BSONObject expectedBson = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                actualBson = cursor.getNext();
            }
            cursor.close();
            expectedBson.put( "_id", 0 );
            expectedBson.put( "age", 22 );
            expectedBson.put( "name", "Sequoiadb" );
            Assert.assertEquals( actualBson, expectedBson );
        } catch ( BaseException e ) {
            System.out.println(
                    "Sequoiadb driver TestTransaction7118 testTrans7118 error, error description:"
                            + e.getMessage() );
            Assert.fail(
                    "Sequoiadb driver TestTransaction7118 testTrans7118 error, error description:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.sdb.close();
        this.sdb2.close();
    }
}
