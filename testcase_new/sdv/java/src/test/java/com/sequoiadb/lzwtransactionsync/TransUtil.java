/**
 * 
 */
/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:TransUtil.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年3月27日下午3:03:26
 *  @version 1.00
 */
package com.sequoiadb.lzwtransactionsync;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;

/**
 * @author zhaoyu
 *
 */

public class TransUtil {
    /**
     * 检查CL主备节点集合CompleteLSN一致
     * 
     * @param cl
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isCLConsistency( DBCollection cl ) throws Exception {

        Sequoiadb db = cl.getSequoiadb();
        boolean isConsistency = false;

        List< String > groupNames = getCLGroups( cl );
        for ( String groupName : groupNames ) {
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );

            try ( Sequoiadb masterNode = rg.getMaster().connect()) {
                // 初始值为-2，是为了与获取到的实际lsn进行比较时，不可能相等
                long completeLSN = -2;
                DBCursor cursor = masterNode.getSnapshot(
                        Sequoiadb.SDB_SNAP_SYSTEM, null, "{CompleteLSN: ''}",
                        null );
                if ( cursor.hasNext() ) {
                    BasicBSONObject snapshot = ( BasicBSONObject ) cursor
                            .getNext();
                    if ( snapshot.containsField( "CompleteLSN" ) ) {
                        completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                    }
                } else {
                    throw new Exception( masterNode.getNodeName()
                            + " can't not find system snapshot" );
                }
                cursor.close();

                for ( String nodeName : nodeNames ) {
                    if ( masterNode.getNodeName().equals( nodeName ) ) {
                        continue;
                    }
                    isConsistency = false;
                    try ( Sequoiadb nodeConn = rg.getNode( nodeName )
                            .connect()) {
                        DBCursor cur = null;
                        // 初始值为-3，是为了与获取到的实际lsn进行比较时，不可能相等
                        long checkCompleteLSN = -3;
                        // 循环比较1800s
                        for ( int i = 0; i < 1800; i++ ) {
                            cur = nodeConn.getSnapshot(
                                    Sequoiadb.SDB_SNAP_SYSTEM, null,
                                    "{CompleteLSN: ''}", null );
                            if ( cur.hasNext() ) {
                                BasicBSONObject checkSnapshot = ( BasicBSONObject ) cur
                                        .getNext();
                                if ( checkSnapshot
                                        .containsField( "CompleteLSN" ) ) {
                                    checkCompleteLSN = ( long ) checkSnapshot
                                            .get( "CompleteLSN" );
                                }
                            }
                            cur.close();

                            if ( completeLSN <= checkCompleteLSN ) {
                                isConsistency = true;
                                break;
                            }
                            try {
                                Thread.sleep( 1000 );
                            } catch ( InterruptedException e ) {
                                e.printStackTrace();
                            }
                        }
                        if ( !isConsistency ) {
                            throw new Exception( "Group [" + groupName
                                    + "] node system snapshot is not the same, masterNode "
                                    + masterNode.getNodeName()
                                    + " CompleteLSN: " + completeLSN + ", "
                                    + nodeName + " CompleteLSN: "
                                    + checkCompleteLSN );
                        }
                    }
                }
            }
        }

        return isConsistency;
    }

    /**
     * 获取原始集合对应的数据组，原始集合可以是普通表、分区表
     * 
     * @param cl
     * @return List<String> 返回所有数据组
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< String > getCLGroups( DBCollection cl ) {
        List< String > groupNames = new ArrayList< String >();
        Sequoiadb db = cl.getSequoiadb();
        if ( CommLib.isStandAlone( db ) ) {
            return groupNames;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", cl.getFullName() );
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        HashSet< String > groupNamesSet = new HashSet< String >();
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                groupNamesSet.add( obj.getString( "GroupName" ) );
            }
        }
        groupNames.addAll( groupNamesSet );
        // groupNames数组元素排序,排序是为了cl的esIndexNames和cappedCLs能一一对应
        Collections.sort( groupNames, new Comparator< Object >() {
            @Override
            public int compare( Object o1, Object o2 ) {
                String str1 = ( String ) o1;
                String str2 = ( String ) o2;
                if ( str1.compareToIgnoreCase( str2 ) < 0 ) {
                    return -1;
                }
                return 1;
            }
        } );

        return groupNames;
    }

}
