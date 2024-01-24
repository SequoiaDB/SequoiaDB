package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.options.InsertOption;
import com.sequoiadb.base.result.InsertResult;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-26316:insert数据检查返回记录数/seqDB-26620:insert返回结果集增加getModifiedNum
 * @author ZhangYanan
 * @createDate 2021.04.01
 * @updateUser ZhangYanan
 * @updateDate 2021.04.01
 * @updateRemark
 * @version v1.0
 */
public class TestInsertResult26316 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clname = "cl_26316";
    private DBCollection cl = null;
    private CollectionSpace cs = null;
    private int insertNum = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clname ) ) {
            cs.dropCollection( clname );
        }
        cl = cs.createCollection( clname );
    }

    @Test
    public void test() {
        InsertOption option = new InsertOption();

        List< BSONObject > bulkInsertList = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "no", i );
            bulkInsertList.add( obj );
        }
        InsertResult bulkInsertResult1 = cl.bulkInsert( bulkInsertList );
        Assert.assertEquals( bulkInsertResult1.getInsertNum(), insertNum );
        Assert.assertEquals( bulkInsertResult1.getDuplicatedNum(), 0 );
        Assert.assertEquals( bulkInsertResult1.getModifiedNum(), 0 );

        checkRecords( cl, bulkInsertList, true );

        InsertResult bulkInsertResult2 = cl.bulkInsert( bulkInsertList,
                option.setFlag( InsertOption.FLG_INSERT_CONTONDUP ) );
        Assert.assertEquals( bulkInsertResult2.getInsertNum(), 0 );
        Assert.assertEquals( bulkInsertResult2.getDuplicatedNum(), insertNum );
        Assert.assertEquals( bulkInsertResult2.getModifiedNum(), 0 );
        checkRecords( cl, bulkInsertList, true );

        BSONObject insertData = new BasicBSONObject();
        insertData.put( "no", insertNum );

        InsertResult insertResult1 = cl.insertRecord( insertData );
        Assert.assertEquals( insertResult1.getInsertNum(), 1 );
        Assert.assertEquals( insertResult1.getDuplicatedNum(), 0 );
        Assert.assertEquals( insertResult1.getModifiedNum(), 0 );
        bulkInsertList.add( insertData );
        checkRecords( cl, bulkInsertList, false );

        InsertResult insertResult2 = cl.insertRecord( insertData,
                option.setFlag( InsertOption.FLG_INSERT_REPLACEONDUP ) );
        Assert.assertEquals( insertResult2.getInsertNum(), 1 );
        Assert.assertEquals( insertResult2.getDuplicatedNum(), 0 );
        Assert.assertEquals( insertResult2.getModifiedNum(), 0 );
        bulkInsertList.add( insertData );
        checkRecords( cl, bulkInsertList, false );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clname ) ) {
                cs.dropCollection( clname );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void checkRecords( DBCollection dbcl, List< BSONObject > expRecords,
            boolean checkId ) {
        DBCursor cursor = dbcl.query( "", "", "", "" );
        ArrayList< BSONObject > actRecords = getReadActList( cursor, checkId );
        if ( !checkId ) {
            for ( BSONObject expObj : expRecords ) {
                expObj.removeField( "_id" );
            }
        }
        Assert.assertEqualsNoOrder( expRecords.toArray(),
                actRecords.toArray() );
    }

    public ArrayList< BSONObject > getReadActList( DBCursor cursor,
            boolean checkId ) {
        ArrayList< BSONObject > actRList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            if ( !checkId ) {
                record.removeField( "_id" );
            }
            actRList.add( record );
        }
        cursor.close();
        return actRList;
    }
}