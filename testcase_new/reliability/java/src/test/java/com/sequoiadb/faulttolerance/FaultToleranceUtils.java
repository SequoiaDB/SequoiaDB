package com.sequoiadb.faulttolerance;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class FaultToleranceUtils {
    public static void changeNodeStatus( String csName, String clName,
            int errorCode ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            StringBuffer strBuffer = new StringBuffer();
            for ( int len = 0; len < 10000; len++ ) {
                strBuffer.append( "aaaaaaaaaa" );
            }

            for ( int i = 0; i < 1000; i++ ) {
                List< BSONObject > insertor = new ArrayList<>();
                for ( int j = 0; j < 1000; j++ ) {
                    insertor.add( ( BSONObject ) JSON.parse(
                            "{ 'a': '" + strBuffer.toString() + "' }" ) );
                }
                sdb.getCollectionSpace( csName ).getCollection( clName )
                        .insert( insertor, 0 );
            }
            Assert.fail( "node status is not be changed!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -252 && e.getErrorCode() != errorCode ) {
                throw e;
            }
        }
    }

    public static String getNodeFTStatus( Sequoiadb db, String nodeName ) {
        try {
            String ftStatus = null;
            DBCursor cursor1 = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                    "{ 'RawData': true, 'NodeName': '" + nodeName + "' }",
                    "{ 'FTStatus': '' }", null );
            while ( cursor1.hasNext() ) {
                ftStatus = ( String ) cursor1.getNext().get( "FTStatus" );
            }
            cursor1.close();
            return ftStatus;
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                    .getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_NET_SEND_ERR
                            .getErrorCode() ) {
                throw e;
            }
            // 磁盘满可能导致节点异常，节点异常时获取快照会报错
            return e.getMessage();
        }
    }

    /**
     * 检查指定节点的状态
     * 
     * @param nodeName
     * @param ftmask
     *            预期结果节点的状态
     * @return
     */
    public static void checkNodeStatus( String nodeName, String ftmask ) {
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            int eachSleepTime = 4;
            int doTimes = 0;
            int timeOut = 700000;
            int checkNum = 0;
            // 检测ftmask为" "时，检测通过后增加循环检测1分钟
            if ( !ftmask.equals( "" ) ) {
                checkNum += 60000;
            }
            String ftStatus = "";
            do {
                try {
                    try {
                        Thread.sleep( eachSleepTime );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                    doTimes += eachSleepTime;
                    DBCursor cursor = db
                            .getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                                    "{ 'RawData': true, 'NodeName': '"
                                            + nodeName + "' }",
                                    "{ 'FTStatus': '' }", null );
                    while ( cursor.hasNext() ) {
                        ftStatus = ( String ) cursor.getNext()
                                .get( "FTStatus" );
                    }
                    cursor.close();
                    if ( ftmask.equals( ftStatus ) ) {
                        checkNum += eachSleepTime;
                    }
                } catch ( BaseException e ) {
                    // 磁盘满可能导致节点异常，节点异常时获取快照会报错
                    if ( e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_NET_SEND_ERR
                                    .getErrorCode() ) {
                        throw e;
                    }
                    try {
                        Thread.sleep( 10000 );
                    } catch ( InterruptedException err ) {
                        err.printStackTrace();
                    }
                    doTimes += 10000;
                }
            } while ( ( !ftmask.equals( ftStatus ) || checkNum < 60000 )
                    && doTimes < timeOut );

            if ( doTimes >= timeOut ) {
                Assert.fail( "The expected ftStatus: " + ftmask
                        + ", but actual ftStatus: " + ftStatus );
            }
        }
    }

    public static void insertError( String csName, String clName,
            int errorCode ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            sdb.getCollectionSpace( csName ).getCollection( clName )
                    .insert( "{ 'a': 0 } " );
            if ( errorCode != 0 ) {
                Assert.fail(
                        "insert record to " + clName + " should be failed!" );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != errorCode ) {
                throw e;
            }
        }
    }

    /**
     * 向集合中插入Lob，预期报错
     * 
     * @param csName
     * @param clName
     * @param errorCode：预期错误码
     * @return
     */
    public static void putLob( String csName, String clName, int insertNum,
            int errorCode ) {
        int writeLobSize = 1024 * 1024 * 100;
        try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            byte[] lobBuff = getRandomBytes( writeLobSize );
            DBCollection dbcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( int i = 0; i < insertNum; i++ ) {
                DBLob lob = dbcl.createLob();
                lob.write( lobBuff );
                lob.close();
                try {
                    Thread.sleep( 10000 );
                } catch ( InterruptedException err ) {
                    err.printStackTrace();
                }
            }
            Assert.fail( "insert records to " + clName + " should be failed!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_CLS_WAIT_SYNC_FAILED
                    .getErrorCode() && e.getErrorCode() != errorCode ) {
                throw e;
            }
        }
    }

    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    /**
     * 获取备节点信息
     * 
     * @param sdb
     * @param groupName
     * @return slaveNodes 备节点信息
     */
    public static List< NodeWrapper > getSlaveNodes( Sequoiadb sdb,
            String groupName, GroupMgr groupMgr ) throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        List< NodeWrapper > nodes = dataGroup.getNodes();
        for ( int i = 0; i < nodes.size(); i++ ) {
            if ( nodes.get( i ).isMaster() ) {
                nodes.remove( i );
            }
        }
        return nodes;
    }

    public static void insertData( DBCollection dbcl, int recordNum )
            throws Exception {
        int batchNum = 5000;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "a", j );
                obj.put( "b", j );
                obj.put( "order", j );
                obj.put( "str",
                        "fjsldkfjlksdjflsdljfhjdshfjksdhfssdljfhjdshfjksdhfsdfhsdjdfhsdjkfhjkdshfj"
                                + "kdshfkjdshfkjsdhfkjshafdsdljfhjdshfjksdhfsdfhsdjkhasdikuhsdjfls"
                                + "hsdjkfhjskdhfkjsdhfjkdssdljfhjdshfjksdhfsdfhsdjhfjkdshfkjhsdjkf"
                                + "hsdkjfhsdsafnweuhfuiwnqsdljfhjdshfjksdhfsdfhsdjefiuokdjf" );
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            batchRecords.clear();
        }

    }

}
