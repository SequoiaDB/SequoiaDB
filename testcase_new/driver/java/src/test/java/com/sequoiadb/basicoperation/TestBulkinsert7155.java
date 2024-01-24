package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.DateInterceptUtil;
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
 * FileName: TestBulkinsert7155.java test interface:bulkInsert (List< BSONObject
 * > insertor, int flag) and test flag values:FLG_INSERT_CONTONDUP,0
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestBulkinsert7155 extends SdbTestBase {
    private String clName = "cl_7155";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    new BasicBSONObject( "PreferedInstance", "M" ) );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
        createCL();
    }

    @Test
    private void testBulkinsert() {
        try {
            bulkInsertContOnDup();
            bulkInsertReplaceOnDup();
            bulkInsertDuplicateKey();
            // TODO:bug:SEQUOIADBMAINSTREAM-1989
            bulkInsertFlagError();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        } finally {
            sdb.close();
        }
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
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    /**
     * test bulkInsert (List< BSONObject > insertor, int flag) set
     * flag=FLG_INSERT_CONTONDUP
     */
    @SuppressWarnings("deprecation")
    private void bulkInsertContOnDup() {
        try {
            List< BSONObject > list = insertRecords();

            // 准备新插入的list，覆盖部分索引记录冲突（如'_id'字段冲突）和全冲突的情况
            List< BSONObject > partialConflictList = new ArrayList< BSONObject >();
            // 取list第一条记录的_id字段不变，其他字段随机修改、添加，再另外添加一条不与集合已插入记录冲突的记录
            BSONObject sameIdObj = new BasicBSONObject();
            sameIdObj.put( "_id", list.get( 0 ).get( "_id" ) );
            sameIdObj.put( "no", "111" );
            sameIdObj.put( "str", "123456" );
            sameIdObj.put( "appendField", "test7155" );
            partialConflictList.add( sameIdObj );

            BSONObject differentIdObj = new BasicBSONObject();
            ObjectId id = new ObjectId();
            differentIdObj.put( "_id", id );
            differentIdObj.put( "no", "222" );
            partialConflictList.add( differentIdObj );

            // insert again with flag FLG_INSERT_CONTONDUP
            List< BSONObject > expRecordList = new ArrayList< BSONObject >();
            expRecordList.addAll( list );
            expRecordList.add( differentIdObj );
            cl.bulkInsert( partialConflictList,
                    DBCollection.FLG_INSERT_CONTONDUP );
            checkResult( expRecordList );

            // 全记录冲突情况
            List< BSONObject > totalConflictList = new ArrayList< BSONObject >();
            // 取list两条记录的_id字段不变，其他部分字段随机修改、添加
            BSONObject sameIdObj1 = new BasicBSONObject();
            sameIdObj1.put( "_id", list.get( 0 ).get( "_id" ) );
            sameIdObj1.put( "no", "11111111111" );
            sameIdObj1.put( "str", "123456789" );
            sameIdObj1.put( "appendField", "test715511111" );
            totalConflictList.add( sameIdObj1 );

            BSONObject sameIdObj2 = new BasicBSONObject();
            sameIdObj2.put( "_id", list.get( 1 ).get( "_id" ) );
            sameIdObj2.put( "no", "22222222222" );
            sameIdObj2.put( "str", "12345678900000" );
            sameIdObj2.put( "appendField", "test7155222222" );
            totalConflictList.add( sameIdObj2 );

            // insert again with flag FLG_INSERT_CONTONDUP
            cl.bulkInsert( totalConflictList,
                    DBCollection.FLG_INSERT_CONTONDUP );
            checkResult( expRecordList );
            cl.delete( "" );

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "bulkinsert with flag FLG_INSERT_CONTONDUP fail "
                            + e.getErrorCode() + e.getMessage() );
        }
    }

    @SuppressWarnings("deprecation")
    private void bulkInsertReplaceOnDup() {
        try {
            List< BSONObject > list = insertRecords();

            // 准备新插入的list，覆盖部分索引记录冲突（如'_id'字段冲突）和全冲突的情况
            List< BSONObject > partialConflictList = new ArrayList< BSONObject >();
            // 取list第一条记录的_id字段不变，其他字段随机修改、添加，再另外添加一条不与集合已插入记录冲突的记录
            BSONObject sameIdObj = new BasicBSONObject();
            sameIdObj.put( "_id", list.get( 0 ).get( "_id" ) );
            sameIdObj.put( "no", "111" );
            sameIdObj.put( "str", "123456" );
            sameIdObj.put( "appendField", "test11319" );
            partialConflictList.add( sameIdObj );

            BSONObject differentIdObj = new BasicBSONObject();
            ObjectId id = new ObjectId();
            differentIdObj.put( "_id", id );
            differentIdObj.put( "no", "222" );
            partialConflictList.add( differentIdObj );

            // insert again with flag FLG_INSERT_REPLACEONDUP
            List< BSONObject > expRecordList = new ArrayList< BSONObject >();
            expRecordList.add( partialConflictList.get( 0 ) );
            expRecordList.add( list.get( 1 ) );
            expRecordList.add( partialConflictList.get( 1 ) );
            cl.bulkInsert( partialConflictList,
                    DBCollection.FLG_INSERT_REPLACEONDUP );
            checkResult( expRecordList );

            cl.delete( "" );
            cl.insert( list );
            // 全记录冲突情况
            List< BSONObject > totalConflictList = new ArrayList< BSONObject >();
            // 取list两条记录的_id字段不变，其他部分字段随机修改、添加
            BSONObject sameIdObj1 = new BasicBSONObject();
            sameIdObj1.put( "_id", list.get( 0 ).get( "_id" ) );
            sameIdObj1.put( "no", "11111111111" );
            sameIdObj1.put( "str", "123456789" );
            sameIdObj1.put( "appendField", "test1131911111" );
            totalConflictList.add( sameIdObj1 );

            BSONObject sameIdObj2 = new BasicBSONObject();
            sameIdObj2.put( "_id", list.get( 1 ).get( "_id" ) );
            sameIdObj2.put( "no", "22222222222" );
            sameIdObj2.put( "str", "12345678900000" );
            sameIdObj2.put( "appendField", "test11319222222" );
            totalConflictList.add( sameIdObj2 );

            // insert again with flag FLG_INSERT_REPLACEONDUP
            cl.bulkInsert( totalConflictList,
                    DBCollection.FLG_INSERT_REPLACEONDUP );
            checkResult( totalConflictList );
            cl.delete( "" );

        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "bulk insert with flag FLG_INSERT_REPLACEONDUP fail "
                            + e.getErrorCode() + e.getMessage() );
        }
    }

    /**
     * test bulkInsert (List< BSONObject > insertor, int flag) set flag=0
     */
    @SuppressWarnings("deprecation")
    private void bulkInsertDuplicateKey() {
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( long i = 0; i < 2; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "_id", 1 );
                String str = "32345.067891234567890123456789";
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimal", decimal );
                obj.put( "no", 5 );
                list.add( obj );
            }
            try {
                cl.bulkInsert( list, 0 );
                Assert.fail(
                        "bulkInsert will interrupt when Duplicate key exist" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -38, e.getMessage() );
            }
            long count = cl.getCount();
            Assert.assertEquals( count, 1, "the actDatas is :" + count );
            cl.delete( "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "bulkinsert fail " + e.getMessage() );
        }
    }

    /**
     * test bulkInsert (List< BSONObject > insertor, int flag) set
     * flag=1/-1,ignore flag value
     */
    @SuppressWarnings("deprecation")
    private void bulkInsertFlagError() {
        List< BSONObject > list = new ArrayList< BSONObject >();
        BSONObject obj = new BasicBSONObject();
        obj.put( "no", 1 );
        list.add( obj );
        try {
            cl.bulkInsert( list, -1 );
            Assert.fail( "when flag is -1,it should fail!" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -6,
                    "unexpected error code" );
        }

        try {
            cl.bulkInsert( list, 1 );
            long count = cl.getCount();
            Assert.assertEquals( count, 1,
                    "the 3th insert actDatas is :" + count );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "bulkinsertFlag fail " + e.getMessage() );
        }
    }

    private void checkResult( List< BSONObject > list ) {
        // check the bulkInsert result
        DBCursor tmpCursor = cl.query();
        BasicBSONObject temp = null;
        List< BSONObject > listActDatas = new ArrayList< BSONObject >();
        while ( tmpCursor.hasNext() ) {
            temp = ( BasicBSONObject ) tmpCursor.getNext();
            listActDatas.add( temp );
        }
        Assert.assertEquals( listActDatas.equals( list ), true,
                "check datas are unequal\n" + "actDatas: "
                        + listActDatas.toString() );
    }

    private List< BSONObject > insertRecords() {
        List< BSONObject > list = new ArrayList< BSONObject >();
        long num = 2;
        for ( long i = 0; i < num; i++ ) {
            BSONObject obj = new BasicBSONObject();
            ObjectId id = new ObjectId();
            obj.put( "_id", id );
            // insert the decimal type data
            String str = "32345.067891234567890123456789" + i;
            BSONDecimal decimal = new BSONDecimal( str );
            obj.put( "decimal", decimal );
            obj.put( "no", i );
            obj.put( "str", "test_" + String.valueOf( i ) );
            // the numberlong type data
            BSONObject numberlong = new BasicBSONObject();
            numberlong.put( "$numberLong", "-9223372036854775808" );
            obj.put( "numlong", numberlong );
            // the obj type
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 + i );
            obj.put( "obj", subObj );
            // the array type
            BSONObject arr = new BasicBSONList();
            arr.put( "0", ( int ) ( Math.random() * 100 ) );
            arr.put( "1", "test" );
            arr.put( "2", 2.34 );
            obj.put( "arr", arr );
            obj.put( "boolf", false );
            // the data type
            Date now = new Date();
            Date expdate = DateInterceptUtil.interceptDate( now, "yyyy-MM-dd" );
            obj.put( "date", expdate );
            // the regex type
            Pattern regex = Pattern.compile( "^2001",
                    Pattern.CASE_INSENSITIVE );
            obj.put( "binary", regex );
            list.add( obj );
        }

        cl.insert( list );
        return list;
    }
}
