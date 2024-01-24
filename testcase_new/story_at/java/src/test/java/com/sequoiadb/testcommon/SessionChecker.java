/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:SessionChecker.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月1日下午1:42:48
 *  @version 1.00
 */
package com.sequoiadb.testcommon;

import java.util.HashSet;
import java.util.Set;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

public class SessionChecker implements IChecker {

    @Override
    public boolean check(Sequoiadb db) {
        // TODO Auto-generated method stub
        final String sType = "Type" ;
        final String sShardAgent = "ShardAgent" ;
        final String sSesssionID = "SessionID" ;
        boolean isSuccess = true ;
        BasicBSONObject cond = new BasicBSONObject();
        cond.put( sType, sShardAgent ) ;
        BasicBSONObject sel = new BasicBSONObject();
        sel.put( sSesssionID, "" ) ;
        Set<Integer> selfSessionIDS = new HashSet<>();
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS_CURRENT, null, sel, null ) ;
        while (cursor.hasNext()) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext() ;
            selfSessionIDS.add( obj.getInt( sSesssionID ) ) ;
        }
        cursor.close(); 
        
        
        cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_SESSIONS_CURRENT, cond, null, null ) ;
        while (cursor.hasNext()) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext() ;
            if (!selfSessionIDS.contains( obj.getInt( sSesssionID ) )) {
                isSuccess = false ;
                System.out.println( obj.toString() );
            }
            selfSessionIDS.add( obj.getInt( sSesssionID ) ) ;
        }
        cursor.close(); 
        
        return isSuccess;
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Session checker";
    }

}
