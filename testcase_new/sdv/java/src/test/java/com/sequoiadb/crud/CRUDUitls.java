package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import com.sun.org.apache.xpath.internal.objects.XString;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;

public class CRUDUitls {

    /**
     * 批量插入数据
     * 
     * @param dbcl
     * @param beginNo
     *            插入数据初始值
     * @param endNo
     *            插入数据结束值
     */
    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int beginNo, int endNo ) {
        int batchNum = 5000;
        int recordNum = endNo - beginNo;
        ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                int value = beginNo++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", value );
                obj.put( "no", value );
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            allRecords.addAll( batchRecords );
            batchRecords.clear();
        }
        return allRecords;
    }

    /**
     * 结果比较
     * 
     * @param dbcl
     * @param expRecords
     *            预期数据
     * @param matcher
     *            查询条件
     */
    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", "" );
        checkRecords( expRecords, cursor );
    }

    /**
     * 结果比较
     * 
     * @param expRecords
     *            预期数据
     * @param cursor
     *            实际数据
     */
    public static void checkRecords( List< BSONObject > expRecords,
            DBCursor cursor ) {
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            if ( !expRecord.equals( record ) ) {
                throw new BaseException( 0,
                        "record: " + record.toString() + "\nexp: "
                                + expRecord.toString() + "------count"
                                + count );
            }
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        Assert.assertEquals( count, expRecords.size() );
    }

    /**
     * 结果比较
     * 
     * @param expRecords
     *            预期数据
     * @param actRecords
     *            实际数据
     */
    public static void checkRecords( List< BSONObject > expRecords,
            List< BSONObject > actRecords ) {
        if ( actRecords.size() == expRecords.size() ) {
            for ( int i = 0; i < actRecords.size(); i++ ) {
                Assert.assertEquals( actRecords.get( i ), expRecords.get( i ) );
            }
        } else {
            Assert.fail(
                    "The actRecords and expRecords size are not equal!expRecords size ="
                            + expRecords.size() + "buf actRecords size ="
                            + actRecords.size() );
        }
    }

    public static class OrderBy implements Comparator< BSONObject > {
        private String str;

        /**
         * list排序方法
         *
         * @param str
         *            排序字段
         */
        public OrderBy( String str ) {
            this.str = str;
        }

        public int compare( BSONObject obj1, BSONObject obj2 ) {
            int flag = 0;
            int no1 = ( int ) obj1.get( str );
            int no2 = ( int ) obj2.get( str );
            if ( no1 > no2 ) {
                flag = 1;
            } else if ( no1 < no2 ) {
                flag = -1;
            }
            return flag;
        }
    }
}
