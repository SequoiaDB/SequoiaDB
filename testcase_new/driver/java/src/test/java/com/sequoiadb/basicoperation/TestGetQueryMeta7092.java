package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.Random;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestGetQueryMeta7092 getQueryMeta (BSONObject query, BSONObject
 *                                orderBy, BSONObject hint, long skipRows, long
 *                                returnRows, int flag)
 * @author chensiqin
 * @version 1.00
 *
 */

public class TestGetQueryMeta7092 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl7092";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        String coordAddr = SdbTestBase.coordUrl;
        try {
            this.sdb = new Sequoiadb( coordAddr, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetQueryMeta7092 setUp error, error description:"
                            + e.getMessage() );
        }
    }

    public void createCL() {
        if ( this.cs.isCollectionExist( clName ) ) {
            this.cs.dropCollection( clName );
        }
        this.cl = this.cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{ShardingKey:{a:-1}}" ) );
        this.cl.createIndex( "ageIndex", "{\"age\":-1}", false, false );
    }

    @Test
    public void test() {
        insertData();
        checkGetQueryMeta();
    }

    public void insertData() {
        try {
            BSONObject bson;
            this.insertRecods = new ArrayList< BSONObject >();
            for ( int i = 1; i <= 100; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "a", i );
                bson.put( "name", "chensiqin" + i );
                bson.put( "age", new Random().nextInt( 50 ) );
                bson.put( "num", -i * 2 );
                this.insertRecods.add( bson );
            }
            this.cl.bulkInsert( this.insertRecods,
                    DBCollection.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetQueryMeta7092 insertData error, error description:"
                            + e.getMessage() );
        }
    }

    public void checkGetQueryMeta() {
        BSONObject empty = new BasicBSONObject();
        BSONObject query, orderBy, hint;
        try {
            BSONObject sessionAttrObj = new BasicBSONObject();
            sessionAttrObj.put( "PreferedInstance", "M" );
            sdb.setSessionAttr( sessionAttrObj );
            query = ( BSONObject ) JSON
                    .parse( "{$and:[{age:{$gte:0}},{age:{$lte:100}}]}" );
            orderBy = new BasicBSONObject();
            orderBy.put( "Indexblocks", 1 );
            DBCursor cursor = this.cl.getQueryMeta( query, orderBy, empty, 0,
                    -1, 0 );
            int i = 0;
            while ( cursor.hasNext() ) {
                BSONObject object = cursor.getNext();
                hint = new BasicBSONObject();
                hint.put( "Indexblocks", object.get( "Indexblocks" ) );
                DBCursor datacursor = cl.query( null, null, null, hint, 0, -1,
                        0 );
                while ( datacursor.hasNext() ) {
                    datacursor.getNext();
                    i++;
                }
                datacursor.close();
            }
            cursor.close();
            Assert.assertEquals( i, 100 );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Sequoiadb driver TestGetQueryMeta7092 checkGetQueryMeta error:"
                            + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "Sequoiadb driver TestGetQueryMeta7092 tearDown error:"
                    + e.getMessage() );
        }
    }
}
