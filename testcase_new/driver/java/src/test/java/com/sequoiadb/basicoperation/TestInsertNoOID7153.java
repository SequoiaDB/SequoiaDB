package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.result.InsertResult;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.AssertJUnit;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

import static com.sequoiadb.testcommon.CommLib.checkRecordsResult;

/**
 * @Description: seqDB-7153:ensureOID接口验证
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestInsertNoOID7153 extends SdbTestBase {
    private String clName = "cl_7153";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        createCL();
    }

    private void createCL() {
        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void bulkInsert() {
        //* test isOIDEnsured() ,and ensureOID(boolean flag), set flag=true
        List< BSONObject > list = new ArrayList<>();
        BSONObject obj1 = new BasicBSONObject();
        BSONObject obj2 = new BasicBSONObject();
        obj1.put( "no", 1 );
        obj1.put( "str", "test_" + String.valueOf( 100 ) );
        obj2.put( "no", 2 );
        obj2.put( "str", "test_" + String.valueOf( 10 ) );
        list.add( obj1 );
        list.add( obj2 );
        cl.ensureOID( true );
        cl.bulkInsert( list );

        // check if there is a _id, if _id not exist then error
        for( int i = 0; i< list.size();i ++){
            BSONObject expRecord = list.get(i);
            Assert.assertTrue(expRecord.containsField("_id"));
        }

        // check the interface:isOIDEnsured(),the return is true
        Assert.assertTrue( cl.isOIDEnsured() );

        // check the _id of client generation insert success.
        DBCursor tmpCursor = cl.query();
        checkRecordsResult(tmpCursor, list );

        //test isOIDEnsured() ,and ensureOID(boolean flag), set flag=false
        cl.truncate();
        cl.ensureOID( false );
        cl.bulkInsert( list, 0 );

        // check the interface:isOIDEnsured()
        Assert.assertFalse( cl.isOIDEnsured() );
        // check if there is a _id, if _id not exist then error
        for( int i = 0; i< list.size();i ++){
            BSONObject expRecord = list.get(i);
            Assert.assertTrue(expRecord.containsField("_id"));
        }

        DBCursor tmpCursor1 = cl.query();
        checkRecordsResult(tmpCursor1, list );
    }

    @AfterClass
    public void tearDown() {
        if (cs.isCollectionExist(clName)) {
            cs.dropCollection(clName);
        }
        sdb.close();
    }
}
