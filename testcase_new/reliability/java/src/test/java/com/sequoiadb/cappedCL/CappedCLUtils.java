package com.sequoiadb.cappedCL;

import java.util.ArrayList;
import org.bson.types.BasicBSONList;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import java.util.Random;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;

public class CappedCLUtils {

    private static String base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%^&*()!";
    private static String baseString;
    private static final int size = 100;

    static {
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < size; i++ ) {
            sb.append( base );
        }
        baseString = sb.toString();
    }

    /*
     * 插入数据
     * @param cl
     * @param insertNums
     * @param strLength
     */
    public static void insertRecords( DBCollection cl, int insertNums,
            int strLength ) {
        BSONObject insertObj = new BasicBSONObject();
        insertObj.put( "a", getRandomString( strLength ) );
        for ( int i = 0; i < insertNums; i++ ) {
            cl.insert( insertObj );
        }
    }

    /*
     * pop记录
     * @param cl
     * @param logicalID
     * @param direction
     */
    public static void pop( DBCollection cl, long logicalID, int direction ) {
        BSONObject popObj = new BasicBSONObject();
        popObj.put( "LogicalID", logicalID );
        popObj.put( "Direction", direction );
        cl.pop( popObj );
    }

    /*
     * 获取固定集合的LID
     * @param cl
     * @param skip
     */
    public static long getLogicalID( DBCollection cl, long skip ) {
        long logicalID = -1;
        BSONObject orderBy = new BasicBSONObject();
        orderBy.put( "_id", 1 );
        DBCursor query = null;
        try {
            long returnOne = 1;
            query = cl.query( null, null, orderBy, null, skip, returnOne );
            while ( query.hasNext() ) {
                logicalID = ( long ) query.getNext().get( "_id" );
            }
        } finally {
            if ( query != null ) {
                query.close();
            }
            System.out.println( "logicalID: " + logicalID );
        }
        return logicalID;
    }

    /*
     * 获取随机字符串
     * @param length
     */
    public static String getRandomString( int length ) {
        final int baseLen = baseString.length();
        if ( length == baseLen ) {
            return baseString;
        } else if ( length < baseLen ) {
            int start = new Random().nextInt( baseLen );
            int end = start + length;
            if ( end > baseLen - 1 ) {
                start = baseLen - length - 1;
            }
            return baseString.substring( start, start + length );
        } else {
            StringBuffer sb = new StringBuffer();
            int expectLength = length;
            while ( expectLength > 0 ) {
                if ( expectLength > baseLen ) {
                    sb.append( baseString );
                    expectLength -= baseLen;
                } else {
                    sb.append( baseString.substring( 0, expectLength ) );
                    expectLength = 0;
                }
            }
            return sb.toString();
        }
    }

    public static int getRandomStringLength( int minLength, int maxLength ) {
        int stringLength = ( int ) ( minLength + Math.random() * maxLength );
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
        ArrayList< String > groupNames = getCLGroupNames( db, csName, clName );
        ReplicaGroup rg = db.getReplicaGroup( groupNames.get( 0 ) );

        try ( Sequoiadb master = rg.getMaster().connect()) {
            DBCollection cl = master.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cursor = cl.query( null, "{'_id':1}", "{'_id':1}", null );

            // 比较具体id值
            final int headLength = 55; // head length
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
     * 获取集合所在的组
     * 
     * @param
     * @return boolean 主备节点一致则返回true，不一致则返回false
     * @throws Exception
     */
    public static ArrayList< String > getCLGroupNames( Sequoiadb db,
            String csName, String clName ) {
        ArrayList< String > groupNames = new ArrayList< String >();
        DBCursor cursorSnapshot = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{Name:'" + csName + "." + clName + "'}", "{CataInfo:''}",
                null );

        BasicBSONList cataInfo = ( BasicBSONList ) cursorSnapshot.getNext()
                .get( "CataInfo" );
        cursorSnapshot.close();
        for ( int i = 0; i < cataInfo.size(); i++ )

        {
            BasicBSONObject record = ( BasicBSONObject ) cataInfo.get( i );
            String groupName = record.getString( "GroupName" );
            if ( !groupNames.isEmpty() && groupNames.contains( groupName ) ) {
                continue;
            }
            groupNames.add( groupName );
        }
        return groupNames;

    }
}
