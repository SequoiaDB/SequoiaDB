package com.sequoiadb.cappedcl;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;

public class CappedCLUtils {
    /**
     * 批量插入记录
     * 
     * @param
     * @return 无返回值
     * @throws Exception
     */
    public static void insertRecords( DBCollection cl, BSONObject insertObj,
            int recordNum ) {
        for ( int i = 0; i < recordNum; i++ ) {
            cl.insert( insertObj );
        }
    }

    /**
     * 生成随机记录长度
     * 
     * @param
     * @return int，记录长度
     * @throws Exception
     */
    public static int getRandomStringLength( int minLength, int maxLength ) {
        // int minLength = 1; // 100k
        // int maxLength = 10 * 1024; // 1M
        int stringLength = ( int ) ( minLength + Math.random() * maxLength );// [100k,1M]
        return stringLength;
    }

    /**
     * 检查固定集合主节点的logicalID是否符合预期
     * 
     * @param
     * @return boolean logicalID符合预期返回true，否则返回false
     * @throws Exception
     */
    public static boolean checkLogicalID( Sequoiadb db, String csName,
            String clName, int stringLength ) {

        // 固定集合只能在1个组上，获取组名
        String groupName = getCLGroupName( db, csName, clName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );

        try ( Sequoiadb master = rg.getMaster().connect()) {
            DBCollection cl = master.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.query( null, "{'_id':1}", "{'_id':1}", null );

            // 比较具体id值
            BSONObject transConfig = CommLib.getTransConfig( db, "data" );
            int headLength;// 记录头，mvcc分支与master长度不一致
            if ( transConfig.get( "mvccon" ).equals( "TRUE" ) ) {
                headLength = 67;
            } else {
                headLength = 55;
            }

            final int gap = 4; // 4字节对齐
            int recordLength = stringLength + headLength;
            int blockCount = 1; // 块数
            final int blockSize = 33554396; // 块大小，减去块头的大小，略小于32M
            long expectId = 0;
            while ( cursor.hasNext() ) {
                long actId = ( long ) cursor.getNext().get( "_id" );
                recordLength = ( 0 == recordLength % gap ) ? recordLength
                        : ( recordLength - recordLength % gap + gap );
                long nextRecordId = expectId + recordLength;
                // 跨块
                if ( nextRecordId > ( blockCount * blockSize ) ) {
                    expectId = blockCount * blockSize;
                    ++blockCount;
                }

                if ( expectId != actId ) {
                    System.out.println(
                            "logicalID in masterNode is wrong,expectId: "
                                    + expectId + "  actIdMaster: " + actId );
                    return false;
                }
                expectId = actId + recordLength;

            }
            cursor.close();
        }
        return true;
    }

    /**
     * 检查主备节点集合中记录是否一致
     * 
     * @param
     * @return boolean 主备节点一致则返回true，不一致则返回false
     * @throws Exception
     */
    public static boolean isRecordConsistency( Sequoiadb db, String csName,
            String clName ) throws Exception {

        // 获取集合所在的组
        String groupName = getCLGroupName( db, csName, clName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );

        // 在第一个节点上的集合上查询数据，作为预期结果
        try ( Sequoiadb node0 = rg.getNode( ( String ) nodeNames.get( 0 ) )
                .connect()) {
            DBCollection nodecl0 = node0.getCollectionSpace( csName )
                    .getCollection( clName );
            long count0 = nodecl0.getCount();
            DBCursor cursor0 = nodecl0.query( null, null, "{'_id':1}", null );
            for ( int j = 1; j < nodeNames.size(); j++ ) {
                try ( Sequoiadb node1 = rg
                        .getNode( ( String ) nodeNames.get( j ) ).connect()) {
                    DBCollection nodecl1 = node1.getCollectionSpace( csName )
                            .getCollection( clName );
                    long count1 = nodecl1.getCount();
                    // 比较记录数
                    if ( count0 != count1 ) {
                        System.out.println( "testCase:"
                                + new Exception().getStackTrace()[ 1 ]
                                        .getClassName()
                                + ",clName:" + csName + "." + clName
                                + " in nodeName:" + nodeNames.get( 0 )
                                + ",getCount:" + count0 + ",in nodeName:"
                                + nodeNames.get( j ) + ",getCount:" + count1 );
                        return false;
                    }

                    // 比较记录
                    DBCursor cursor1 = nodecl1.query( null, null, "{'_id':1}",
                            null );
                    while ( cursor0.hasNext() && cursor1.hasNext() ) {
                        BSONObject record0 = cursor0.getNext();
                        BSONObject record1 = cursor1.getNext();

                        if ( !record0.equals( record1 ) ) {
                            System.out.println( "testCase:"
                                    + new Exception().getStackTrace()[ 1 ]
                                            .getClassName()
                                    + ",clName:" + csName + "." + clName
                                    + " in nodeName:" + nodeNames.get( 0 )
                                    + ",record0:" + record0 + ",cl in nodeName:"
                                    + nodeNames.get( j ) + ",record1:"
                                    + record1 );
                            return false;
                        }
                    }
                    cursor1.close();
                }

            }
            cursor0.close();
        }
        return true;

    }

    /**
     * 获取集合所在的组
     * 
     * @param
     * @return boolean 主备节点一致则返回true，不一致则返回false
     * @throws Exception
     */
    public static String getCLGroupName( Sequoiadb db, String csName,
            String clName ) {
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{Name:'" + csName + "." + clName + "'}", "{CataInfo:''}",
                null );

        String groupName = null;
        while ( cursor.hasNext() ) {
            BasicBSONList records = ( BasicBSONList ) cursor.getNext()
                    .get( "CataInfo" );
            groupName = ( String ) ( ( BSONObject ) records.get( 0 ) )
                    .get( "GroupName" );
        }

        cursor.close();
        return groupName;
    }

    /**
     * 检查CL主备节点集合CompleteLSN一致
     * 
     * @param cl
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isLSNConsistency( Sequoiadb db, String groupName )
            throws Exception {
        boolean isConsistency = false;
        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );

        try ( Sequoiadb masterNode = rg.getMaster().connect()) {
            long completeLSN = -2;
            DBCursor cursor = masterNode.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                    null, "{CompleteLSN: ''}", null );
            if ( cursor.hasNext() ) {
                BasicBSONObject snapshot = ( BasicBSONObject ) cursor.getNext();
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
                try ( Sequoiadb nodeConn = rg.getNode( nodeName ).connect()) {
                    DBCursor cur = null;
                    long checkCompleteLSN = -3;
                    for ( int i = 0; i < 600; i++ ) {
                        cur = nodeConn.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                                null, "{CompleteLSN: ''}", null );
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
                        System.out.println( "Group [" + groupName
                                + "] node system snapshot is not the same, masterNode "
                                + masterNode.getNodeName() + " CompleteLSN: "
                                + completeLSN + ", " + nodeName
                                + " CompleteLSN: " + checkCompleteLSN );
                    }
                }
            }
        }

        return isConsistency;
    }
}
