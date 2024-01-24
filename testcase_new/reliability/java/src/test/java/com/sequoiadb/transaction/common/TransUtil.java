package com.sequoiadb.transaction.common;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

public class TransUtil {
    /**
     * 转账线程执行标志
     */
    public static volatile boolean runFlag = true;

    /**
     * 当前正在构造的异常
     */
    public static FaultMakeTask currentTask;

    /**
     * 集群恢复时间,时间s
     */
    public static int ClusterRestoreTimeOut = 600;

    /**
     * 初始化线程执行标记和构造异常
     * 
     * @param task
     */
    public static void setTimeTask( TaskMgr taskMgr, FaultMakeTask task ) {
        runFlag = true;
        currentTask = task;
        taskMgr.addTask( new TransUtil().new ForceOtherThs() );
    }

    /**
     * 等待当前正在构造的异常构造成功，构造成功后，触发10秒钟执行标记为 false
     * 
     * @description TransUtil.java
     * @author yinzhen
     * @date 2019年10月30日
     */
    private class ForceOtherThs extends OperateTask {

        @Override
        public void exec() throws Exception {
            int count = 0;
            while ( !currentTask.isMakeSuccess() ) {
                Thread.sleep( 100 );
                if ( count++ == 3000 ) {
                    break;
                }
            }
            if ( currentTask.isMakeSuccess() ) {
                Timer timer = new Timer();
                timer.schedule( new TaskEndTime(), 10 * 1000 );
            }
        }

        /**
         * 
         * @description TransUtil.java
         * @author yinzhen
         * @date 2019年7月22日
         */
        class TaskEndTime extends TimerTask {
            @Override
            public void run() {
                TransUtil.runFlag = false;
                System.gc();
            }
        }
    }

    /**
     * 插入数据 10000 个账户，每个账户 10000 元
     * 
     * @param cl
     */
    public static void insertTransData( DBCollection cl ) {
        List< BSONObject > reocrds = new ArrayList<>();
        for ( int i = 0; i < 10000; i++ ) {
            reocrds.add( ( BSONObject ) JSON
                    .parse( "{'balance':10000, 'account':" + i + "}" ) );
        }
        cl.insert( reocrds );
    }

    /**
     * 插入记录
     * 
     * @param cl
     * @param start
     * @param end
     * @return
     */
    public static List< BSONObject > insertData( DBCollection cl, int start,
            int end ) {
        List< BSONObject > records = new ArrayList<>();
        for ( int i = start; i < end; i++ ) {
            BSONObject obj = ( BSONObject ) JSON
                    .parse( "{_id:" + i + ", a:" + i + ", b:" + i + "}" );
            records.add( obj );
        }
        List< BSONObject > expList = new ArrayList<>( records );
        Collections.shuffle( records );
        cl.insert( records );
        return expList;
    }

    /**
     * 获取当前 sdb 的 coord 节点的 NodeWrapper
     * 
     * @param groupMgr
     * @param sdb
     * @return
     * @throws ReliabilityException
     */
    public static NodeWrapper getCoordNode( Sequoiadb sdb )
            throws ReliabilityException {
        GroupMgr groupMgr = GroupMgr.getInstance();
        groupMgr.setSdb( new Sequoiadb( SdbTestBase.coordUrl, "", "" ) );
        GroupWrapper group = groupMgr.getGroupByName( "SYSCoord" );
        List< NodeWrapper > nodes = group.getNodes();
        String hostName = sdb.getHost();
        String svcName = String.valueOf( sdb.getPort() );
        for ( NodeWrapper node : nodes ) {
            if ( hostName.equals( node.hostName() )
                    && svcName.equals( node.svcName() ) ) {
                return node;
            }
        }
        return null;
    }

    /**
     * 获取非当前 sdb 的 coord 的连接 coordUrl
     * 
     * @param sdb
     * @return
     */
    public static String getCoordUrl( Sequoiadb sdb ) {
        String coordUrl = null;
        List< String > nodeAddress = CommLib.getNodeAddress( sdb, "SYSCoord" );
        for ( String nodeAddr : nodeAddress ) {
            String hostName = nodeAddr.split( ":" )[ 0 ];
            String svcName = nodeAddr.split( ":" )[ 1 ];
            if ( !hostName.equals( sdb.getHost() ) ) {
                coordUrl = hostName + ":" + svcName;
                break;
            }
        }
        return coordUrl;
    }

    /**
     * 创建 hash 分区表/主子表(主表下挂载多个子表，子表覆盖分区表)，replSize 设置为1，且已切分到所有组上，切分键为账户字段
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     * @param mainCLName
     * @param subCLName1
     * @param subCLName2
     */
    public static void createCLs( Sequoiadb sdb, String csName,
            String hashCLName, String mainCLName, String subCLName1,
            String subCLName2 ) {
        sdb.getCollectionSpace( csName ).createCollection( hashCLName,
                ( BSONObject ) JSON.parse(
                        "{'ShardingKey':{'account':1}, 'ShardingType':'hash', 'AutoSplit':true, 'ReplSize':1}" ) );
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .createCollection( mainCLName, ( BSONObject ) JSON.parse(
                        "{'ShardingKey':{'account':1}, 'ShardingType':'range', 'IsMainCL':true, 'ReplSize':1}" ) );
        sdb.getCollectionSpace( csName ).createCollection( subCLName1 );
        sdb.getCollectionSpace( csName ).createCollection( subCLName2,
                ( BSONObject ) JSON.parse(
                        "{'ShardingKey':{'account':1}, 'ShardingType':'hash', 'AutoSplit':true, 'ReplSize':1}" ) );
        mainCL.attachCollection( csName + "." + subCLName1, ( BSONObject ) JSON
                .parse( "{LowBound:{'account':{'$minKey':1}}, UpBound:{'account':3000}}" ) );
        mainCL.attachCollection( csName + "." + subCLName2, ( BSONObject ) JSON
                .parse( "{LowBound:{'account':3000}, UpBound:{'account':{'$maxKey':1}}}" ) );
    }

    /**
     * 使用 createCLs 创建 hash 分区表/主子表并插入记录
     * 
     * @param sdb
     * @param csName
     * @param hashCLName
     * @param mainCLName
     * @param subCLName1
     * @param subCLName2
     */
    public static void createCLsAndInsertData( Sequoiadb sdb, String csName,
            String hashCLName, String mainCLName, String subCLName1,
            String subCLName2 ) {
        createCLs( sdb, csName, hashCLName, mainCLName, subCLName1,
                subCLName2 );
        DBCollection hashCL = sdb.getCollectionSpace( csName )
                .getCollection( hashCLName );
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        insertTransData( hashCL );
        insertTransData( mainCL );
    }

    /**
     * 获取游标读取的记录
     * 
     * @param cursor
     * @return
     */
    public static ArrayList< BSONObject > getReadActList( DBCursor cursor ) {
        ArrayList< BSONObject > actRList = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            actRList.add( record );
        }
        cursor.close();
        return actRList;
    }

    /**
     * 清理环境规避 -190 错误码，可以删除 CS，删除 CL 以及关闭 SDB
     * 
     * @param sdb
     * @param csName
     * @param clNames
     *            集合名，可以填写多个
     * @throws InterruptedException
     */
    public static void cleanEnv( Sequoiadb sdb, String csName,
            String... clNames ) throws InterruptedException {
        cleanEnv( sdb, csName, false, clNames );
    }

    /**
     * 检查账户总和是否与预期相等
     * 
     * @param sdb
     * @param csName
     * @param clName
     * @throws InterruptedException
     */
    public static void checkSum( Sequoiadb db, String csName, String clName,
            int sum, String className ) throws InterruptedException {
        int count = 0;
        int checkTimes = 1800;
        while ( count++ < checkTimes ) {
            DBCursor cursor = db.exec( "select sum(balance) as balance from "
                    + csName + "." + clName );
            double balance = ( double ) cursor.getNext().get( "balance" );
            cursor.close();
            if ( sum != ( int ) balance ) {
                if ( count == checkTimes ) {
                    Assert.fail( "testCase name: " + className
                            + ",check clName: " + csName + "." + clName
                            + " amount of general ledger timeout,"
                            + " actual sum(balance): " + balance
                            + ",expect sum(balance): " + sum );
                }
                Thread.sleep( 1000 );
                continue;
            }
            break;
        }
    }

    /**
     * 清理环境规避 -190 错误码，可以删除 CS，删除 CL 以及关闭 SDB
     * 
     * @param sdb
     * @param csName
     * @param dropCS
     *            为 true 时直接删除 cs
     * @param clNames
     *            集合名，可以填写多个
     * @throws InterruptedException
     */
    public static void cleanEnv( Sequoiadb sdb, String csName, boolean dropCS,
            String... clNames ) throws InterruptedException {
        try {
            if ( dropCS ) {
                int count = 0;
                while ( count < 600 ) {
                    try {
                        sdb.dropCollectionSpace( csName );
                        break;
                    } catch ( BaseException e ) {
                        if ( -190 != e.getErrorCode() || count > 600 ) {
                            throw e;
                        }
                    }
                    Thread.sleep( 1000 );
                    count++;
                }
            } else {
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                for ( String clName : clNames ) {
                    int count = 0;
                    while ( count < 600 ) {
                        try {
                            cs.dropCollection( clName );
                            break;
                        } catch ( BaseException e ) {
                            if ( -190 != e.getErrorCode() || count > 600 ) {
                                throw e;
                            }
                        }
                        Thread.sleep( 1000 );
                        count++;
                    }
                }
            }
        } finally {
            if ( sdb != null ) {
                sdb.closeAllCursors();
                sdb.close();
            }
        }
    }
}
