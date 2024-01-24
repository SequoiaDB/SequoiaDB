/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:ContextChecker.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月1日下午1:43:13
 *  @version 1.00
 */
package com.sequoiadb.testcommon;

import java.util.HashSet;
import java.util.Set;

import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;

public class ContextChecker implements IChecker {

    @Override
    public boolean check(Sequoiadb db) {
        // TODO Auto-generated method stub
        boolean isSuccess = true ;
        final String sSesssionID = "SessionID" ;
        BasicBSONObject nullObj = null;
     
        BasicBSONObject sel = new BasicBSONObject();
        sel.put( sSesssionID, "" ) ;
        Set<Integer> selfSessionIDS = new HashSet<>();
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT, null, sel, null ) ;
        while (cursor.hasNext()) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext() ;
            selfSessionIDS.add( obj.getInt( sSesssionID ) ) ;
        }
        cursor.close(); 
        
        
        cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CONTEXTS, nullObj, nullObj, nullObj ) ;
        while (cursor.hasNext()) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext() ;
            if (!selfSessionIDS.contains( obj.getInt( sSesssionID ) )) {
                isSuccess = false ;
                boolean isFind = false ;
                BasicBSONList contexts = ( BasicBSONList ) obj.get( "Contexts" ) ;
                for (Object subobj: contexts) {
                    BasicBSONObject tmp = ( BasicBSONObject ) subobj ;
                    if ( tmp.getString( "Type" ).equals( "DUMP" )) {
                        isFind = true ;
                        break;
                    }
                }
                
                if ( !isFind ) {
                    System.out.println( obj.toString() );
                }
            }
        }
        cursor.close(); 
        
        return isSuccess;
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Context checker";
    }

}
