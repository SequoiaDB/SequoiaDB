/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:GroupCheckResult.java 类的详细描述
 *
 * @author 类创建者姓名 Date:2017-2-27下午12:54:17
 * @version 1.00
 */
package com.sequoiadb.crud;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;

public class CRUDUtils {
    /**
     * 结果比较
     * 
     * @param dbcl
     * @param expRecords
     *            预期数据
     * @param 查询条件
     */
    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String matcher ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", "" );
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
        }
        cursor.close();
        Assert.assertEquals( count, expRecords.size() );
    }

    /**
     * 批量插入数据
     * 
     * @param dbcl
     * @param recordNum
     *            插入数据量
     * @param begin
     *            插入数据初始值
     * @param insertRecord
     *            构造预期数据list
     */
    public static void insertData( DBCollection dbcl, int recordNum,
            int beginNo, ArrayList< BSONObject > insertRecord ) {
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", i );
            obj.put( "no", i );
            obj.put( "num", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
    }

    public static class OrderBy implements Comparator< BSONObject > {
        private String str;

        /**
         * List排序
         *
         * @param str
         *         排序字段
         */
        public OrderBy(String str) {
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
