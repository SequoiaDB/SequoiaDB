/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:TransChecker.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月1日下午1:42:20
 *  @version 1.00
 */
package com.sequoiadb.testcommon;

import org.bson.BSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

public class TransChecker implements IChecker {

    @Override
    public boolean check(Sequoiadb db) {
        // TODO Auto-generated method stub
        boolean isSuccess = true ;
        BSONObject nullObj = null ;
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSACTIONS, nullObj, nullObj, nullObj ) ;
        while (cursor.hasNext()) {
            isSuccess = false ;
            BSONObject obj = cursor.getNext() ;
            System.out.println(obj.toString());
        }
        return isSuccess;
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Transaction checker";
    }

}
