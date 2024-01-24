package com.sequoiadb.bsontypes;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * FileName: BasicBsonListTest10362.java* test interface: ObjectId () TestLink:
 * seqDB-10362:
 * 
 * @author wuyan
 * @Date 2016.10.17
 * @version 1.00
 */
public class BasicBsonListTest10362 extends SdbTestBase {

    private String clName = "cl_10362";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        createCL();
    }

    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }

        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testBasicBsonList1() {
        try {
            BasicBSONList list = new BasicBSONList();
            BasicBSONList list1 = new BasicBSONList();
            BSONObject obj = new BasicBSONObject();
            // test interface put (String key, Object v),insert arr [0,1,2]
            BSONObject arr = new BasicBSONList();
            BSONObject subArr = new BasicBSONObject();
            subArr.put( "no", 123 );
            arr.put( "0", subArr );
            arr.put( "1", 2 );
            // test interface :putAll (BSONObject o),insert arr["test",1]
            BSONObject nestValue = new BasicBSONObject();
            BSONObject nestObj = new BasicBSONList();
            nestValue.put( "0", "test" );
            nestValue.put( "1", 1 );
            nestObj.putAll( nestValue );
            // test interface:put (int key, Object v),insert "no":[ "testlist" ,
            // "testlist1"]
            list.put( 0, "tlist0" );
            list.put( 1, "tlist1" );
            // test interface:putAll (Map m)
            Map< String, BSONObject > map = new LinkedHashMap< String, BSONObject >();
            BSONObject bmap = new BasicBSONObject();
            bmap.put( "map", "bmap0" );
            map.put( "0", bmap );
            list1.putAll( map );
            obj.put( "no", list );
            obj.put( "a", arr );
            obj.put( "test", nestObj );
            obj.put( "tmap", list1 );
            cl.insert( obj );
            System.out.println( "obj=" + obj.toString() );
            // check the insert result
            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            tmpCursor.close();
            Assert.assertEquals( actRecs, obj,
                    "check datas are unequal\n" + "actDatas: " + actRecs + "\n"
                            + "expectDatas: " + obj.toString() );

            // test the interface:get (String key),eg: "a" : [ { "no" : 123} ,
            // 2]
            BasicBSONList subResult = ( BasicBSONList ) actRecs.get( "a" );
            // the return is the second value:2
            Object rValue = subResult.get( 1 );
            Assert.assertEquals( rValue.toString(), "2",
                    "get() return value is wrong" );

            // removeField (String key),remove the first key-value,then find by
            // asList()
            subResult.removeField( "0" );

            Object asListValue = subResult.asList();
            Assert.assertEquals( asListValue.toString(), "[2]",
                    "asListValue=" + asListValue.toString() );
            subResult.removeField( "1" );

            // query the key is "test", then test the interface:toMap()
            BasicBSONList subResult1 = ( BasicBSONList ) actRecs.get( "test" );
            Map< String, Object > rmap = subResult1.toMap();
            Map< String, Object > expected = new HashMap<>();
            expected.put( "1", 1 );
            expected.put( "0", "test" );
            Assert.assertEquals( rmap, expected );
            Set< String > keySet = subResult1.keySet();
            System.out.println( "keySet=" + keySet.toString() );

        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    // @Test
    public void testBasicBsonList2() {
        try {
            BasicBSONList list = new BasicBSONList();
            list.put( 0, "bar" );
            list.put( 1, "str" );
            list.asList();
            System.out.println( "list=" + list.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
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
