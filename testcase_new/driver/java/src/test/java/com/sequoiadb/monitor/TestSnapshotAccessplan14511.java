package com.sequoiadb.monitor;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSnapshotAccessplan14511.java test interface: getSnapshot(int
 * snapType, String matcher, String selector, String orderBy) the snapType is
 * Sequoiadb.SDB_SNAP_ACCESSPLANS
 * 
 * @author wuyan
 * @Date 2018.2.12
 * @version 1.00
 */
public class TestSnapshotAccessplan14511 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl14511";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
        insertData();
    }

    @Test
    public void testSnapshotAccessplan() {
        // qurey retrurnNum is 100
        BSONObject queryConf = ( BSONObject ) JSON.parse( "{no:{$lt:100}}" );
        queryData( queryConf );
        // qurey retrurnNum is 500
        BSONObject queryConf1 = ( BSONObject ) JSON.parse( "{no:{$gte:500}}" );
        queryData( queryConf1 );
        // qurey retrurnNum is 200
        BSONObject queryConf2 = ( BSONObject ) JSON
                .parse( "{'$and':[{no:{$lt:400}},{no:{$gte:200}}]}" );
        queryData( queryConf2 );

        // get snapshot by accessplay
        String matcher = "{'Collection':'" + SdbTestBase.csName + "." + clName
                + "'}";
        String selector = "{'MinTimeSpentQuery.ReturnNum':{$include:1}}";
        String orderBy = "{'MinTimeSpentQuery.ReturnNum':1}";
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_ACCESSPLANS,
                matcher, selector, orderBy );

        List< BSONObject > actualList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actualList.add( obj );
        }
        cursor.close();

        // check the get accessplay snapshot result
        List< BSONObject > expectedList = new ArrayList< BSONObject >();
        String[] expAccessPlayInfo = {
                "{'MinTimeSpentQuery':{'ReturnNum':100}}",
                "{'MinTimeSpentQuery':{'ReturnNum':200}}",
                "{'MinTimeSpentQuery':{'ReturnNum':500}}" };
        for ( int i = 0; i < expAccessPlayInfo.length; i++ ) {
            BSONObject expRec = ( BSONObject ) JSON
                    .parse( expAccessPlayInfo[ i ] );
            expectedList.add( expRec );
        }
        Assert.assertEquals( actualList.toString(), expectedList.toString(),
                "actDatas: " + actualList.toString() + "\n" + "expRecs: "
                        + expectedList.toString() );

    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.close();

    }

    private void insertData() {
        List< BSONObject > list = new ArrayList< BSONObject >();
        for ( long i = 0; i < 1000; i++ ) {
            BSONObject obj = new BasicBSONObject();
            ObjectId id = new ObjectId();
            obj.put( "_id", id );
            // insert the decimal type data
            String str = "32345.067891234567890123456789" + i;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            obj.put( "no", i );
            obj.put( "str", "test_" + String.valueOf( i ) );
            list.add( obj );
        }
        cl.insert( list, DBCollection.FLG_INSERT_CONTONDUP );
    }

    private void queryData( BSONObject matcher ) {
        DBCursor cursor = cl.query( matcher, null, null, null );
        List< BSONObject > actualList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject object = cursor.getNext();
            actualList.add( object );
        }
        cursor.close();
    }
}
