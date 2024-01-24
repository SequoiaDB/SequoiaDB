package com.sequoiadb.index;

import java.util.*;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;

import static org.testng.Assert.assertTrue;

/**
 * FileName: IndexUtils.java public call function for test index
 */
public class IndexUtils {

    public static DBCollection createCSAndCL( Sequoiadb sdb, String csName,
            String clName, int pagesize ) {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", pagesize );
        CollectionSpace cs = sdb.createCollectionSpace( csName, options );
        DBCollection dbcl = cs.createCollection( clName );
        return dbcl;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNum = 5000;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                String stringValue = getRandomString( length );
                int value = count++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", stringValue );
                obj.put( "testb", value );
                obj.put( "no", value );
                obj.put( "testno", value );
                obj.put( "teststr", "teststr" + value );
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
        return insertRecord;
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        return insertData( dbcl, recordNum, 50 );
    }

    public static void insertDataWithOutReturn( DBCollection dbcl,
            int recordNum, int length ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        int batchNum = 5000;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        int count = 0;
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                String stringValue = getRandomString( length );
                int value = count++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", stringValue );
                obj.put( "testb", value );
                obj.put( "no", value );
                obj.put( "testno", value );
                obj.put( "teststr", "teststr" + value );
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
    }

    public static void insertDataWithOutReturn( DBCollection dbcl,
            int recordNum ) {
        insertDataWithOutReturn( dbcl, recordNum, 50 );
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String matcher, String hint ) {
        DBCursor cursor = dbcl.query( matcher, "", "{'no':1}", hint );
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        Assert.assertEquals( count, expRecords.size() );
    }

    public static String getRandomString( int length ) {
        String str = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^asssgggg!@#$";
        StringBuilder sbBuilder = new StringBuilder();

        // random generation 80-length string.
        Random random = new Random();
        StringBuilder subBuilder = new StringBuilder();
        int strLen = str.length();
        for ( int i = 0; i < strLen; i++ ) {
            int number = random.nextInt( strLen );
            subBuilder.append( str.charAt( number ) );
        }

        // generate a string at a specified length by subBuffer
        int times = length / str.length();
        for ( int i = 0; i < times; i++ ) {
            sbBuilder.append( subBuilder );
        }
        int subTimes = length % str.length();
        if ( subTimes != 0 ) {
            sbBuilder.append( str.substring( 0, subTimes ) );
        }
        return sbBuilder.toString();
    }

    // 获取cl所在的所有数据节点
    public static List< String > getClNodes( Sequoiadb sdb, String csName,
            String clName ) {
        List< String > groupNames = getCLGroupNames( sdb, csName, clName );

        List< String > nodes = new ArrayList<>();
        for ( int i = 0; i < groupNames.size(); i++ ) {
            String groupName = groupNames.get( i );
            List< String > nodeInfo = CommLib.getNodeAddress( sdb, groupName );
            nodes.addAll( nodeInfo );
        }
        return nodes;
    }

    /**
     * 检查CL主备节点集合CompleteLSN一致 *
     *
     * @param db
     *            new db连接
     * @param groupName
     *            组名
     * @return boolean 如果主节点CompleteLSN小于等于备节点CompleteLSN返回true,否则返回false
     * @throws Exception
     * @author luweikang
     */
    public static boolean isLSNConsistency( Sequoiadb db, String groupName )
            throws Exception {
        boolean isConsistency = false;
        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        Node masterNode = rg.getMaster();
        try ( Sequoiadb masterSdb = new Sequoiadb(
                masterNode.getHostName() + ":" + masterNode.getPort(), "",
                "" )) {
            long completeLSN = -2;
            DBCursor cursor = masterSdb.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                    null, "{CompleteLSN: ''}", null );
            if ( cursor.hasNext() ) {
                BasicBSONObject snapshot = ( BasicBSONObject ) cursor.getNext();
                if ( snapshot.containsField( "CompleteLSN" ) ) {
                    completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                }
            } else {
                throw new Exception( masterSdb.getNodeName()
                        + " can't not find system snapshot" );
            }
            cursor.close();

            for ( String nodeName : nodeNames ) {
                if ( masterNode.getNodeName().equals( nodeName ) ) {
                    continue;
                }
                isConsistency = false;
                try ( Sequoiadb nodeConn = new Sequoiadb( nodeName, "", "" )) {
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

    // 检查索引主备节点一致性
    public static void checkIndexConsistent( Sequoiadb sdb, String csName,
            String clName, String idxName, boolean isexist ) throws Exception {
        List< String > groupNames = getCLGroupNames( sdb, csName, clName );
        // 校验lsn是否一致
        for ( String groupName : groupNames ) {
            Assert.assertTrue( isLSNConsistency( sdb, groupName ) );
        }

        int sleepTime = 1000;
        int timeOut = 300000;
        int doTime = 0;
        int sucNodes = 0;
        // 校验主备节点索引信息一致
        List< String > clNodes = getClNodes( sdb, csName, clName );
        if ( isexist ) {
            while ( doTime <= timeOut && sucNodes < clNodes.size() ) {
                BSONObject expIndexDef = null;
                sucNodes = 0;
                for ( String clNode : clNodes ) {
                    try ( Sequoiadb sequoiadb = new Sequoiadb( clNode, "",
                            "" )) {
                        BSONObject index = sequoiadb
                                .getCollectionSpace( csName )
                                .getCollection( clName )
                                .getIndexInfo( idxName );
                        BSONObject indexDef = ( BSONObject ) index
                                .get( "IndexDef" );
                        String indexFlag = ( String ) index.get( "IndexFlag" );
                        if ( !indexFlag.equals( "Normal" ) ) {
                            break;
                        }
                        if ( expIndexDef == null ) {
                            expIndexDef = indexDef;
                            sucNodes++;
                        } else {
                            Assert.assertEquals( indexDef, expIndexDef );
                            sucNodes++;
                        }
                    }
                }
                Thread.sleep( sleepTime );
                doTime += sleepTime;
            }
            if ( doTime > timeOut ) {
                Assert.fail(
                        "create index not consistent, timeout " + clNodes );
            }
        } else {
            while ( doTime <= timeOut && sucNodes < clNodes.size() ) {
                sucNodes = 0;
                for ( String clNode : clNodes ) {
                    try ( Sequoiadb sequoiadb = new Sequoiadb( clNode, "",
                            "" ) ;) {
                        boolean indexExist = sequoiadb
                                .getCollectionSpace( csName )
                                .getCollection( clName )
                                .isIndexExist( idxName );
                        if ( !indexExist ) {
                            sucNodes++;
                        } else {
                            break;
                        }
                    }
                }
                Thread.sleep( sleepTime );
                doTime += sleepTime;
            }
            if ( doTime > timeOut ) {
                Assert.fail( "drop index not consistent, timeout " + clNodes );
            }
        }
    }

    // 获取cl所在所有组名
    public static List< String > getCLGroupNames( Sequoiadb sdb, String csName,
            String clName ) {
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{'Name': '" + csName + "." + clName + "'}", "", "" );
        List< String > groupNames = new ArrayList<>();
        while ( cursor.hasNext() ) {
            BSONObject clInfo = cursor.getNext();
            BasicBSONList cataInfo = ( BasicBSONList ) clInfo.get( "CataInfo" );
            for ( int i = 0; i < cataInfo.size(); ++i ) {
                BasicBSONObject groupInfo = ( BasicBSONObject ) cataInfo
                        .get( i );
                String groupName = groupInfo.getString( "GroupName" );
                groupNames.add( groupName );
            }
        }
        cursor.close();
        return groupNames;
    }

    /**
     * @description: 等待任务执行完成
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param taskTypeDesc
     *            //任务类型
     */
    public static void waitTaskFinish( Sequoiadb sdb, String csName,
            String clName, String taskTypeDesc ) {
        waitTaskFinish( sdb, csName, clName, taskTypeDesc, 1 );
    }

    public static void waitTaskFinish( Sequoiadb sdb, String csName,
            String clName, String taskTypeDesc, int taskNum ) {
        // status取值 0:Ready 9:Finish
        int status = 9;
        int times = 0;
        int sleepTime = 100;
        int maxWaitTimes = 20000;

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );

        while ( true ) {
            DBCursor cursor = sdb.listTasks( matcher, null, null, null );
            int finshTaskNum = 0;
            BSONObject taskInfo = null;
            while ( cursor.hasNext() ) {
                taskInfo = cursor.getNext();
                int actStatus = ( int ) taskInfo.get( "Status" );
                if ( actStatus != status ) {
                    break;
                }
                finshTaskNum++;
            }
            cursor.close();

            if ( finshTaskNum == taskNum ) {
                break;
            } else if ( times * sleepTime > maxWaitTimes ) {
                throw new Error( "waiting task time out! waitTimes="
                        + times * sleepTime + "\ntask=" + taskInfo );
            }

            try {
                Thread.sleep( sleepTime );
            } catch ( InterruptedException e ) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            times++;
        }
    }

    /**
     * @description: 检查copy任务信息（选择copy子表名、索引名、结果和状态码比较）
     * @param csName
     *            主表所在cs
     * @param mainclName
     *            主表名
     * @param indexNames
     *            检验测索引名,复制索引操作需要传入索引名数组
     * @param subclNames
     *            检验子表名
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param status
     *            //任务状态码 完成:9
     */
    @SuppressWarnings("unchecked")
    public static void checkCopyTask( Sequoiadb db, String csName,
            String mainclName, List< String > indexNames,
            List< String > subclNames, int resultCode, int status ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + mainclName );
        matcher.put( "TaskTypeDesc", "Copy index" );
        DBCursor cursor = db.listTasks( matcher, null, null, null );
        BSONObject taskInfo = new BasicBSONObject();
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        // 校验索引名
        List< String > actIndexNames = ( List< String > ) taskInfo
                .get( "IndexNames" );
        Collections.sort( actIndexNames );
        Collections.sort( indexNames );
        Assert.assertEquals( actIndexNames, indexNames,
                "actTaskInfo= " + taskInfo );

        // 校验结果状态码和结果码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode,
                "actTaskInfo= " + taskInfo );

        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo= " + taskInfo );

        // 校验copy子表信息
        List< String > actSubCLNames = ( List< String > ) taskInfo
                .get( "CopyTo" );
        Collections.sort( actSubCLNames );
        Collections.sort( subclNames );
        Assert.assertEquals( actSubCLNames, subclNames );

        Assert.assertEquals( taskNum, 1, "copy index task num should be 1!" );
    }

    public static void checkCopyTask( Sequoiadb db, String csName,
            String mainclName, List< String > indexNames,
            List< String > subclNames ) {
        checkCopyTask( db, csName, mainclName, indexNames, subclNames, 0, 9 );
    }

    /**
     * @description: 检查create / drop
     *               index任务信息（通过cl、索引名和任务类型获取任务，检查索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param indexName
     *            检验测索引名
     * @param resultCode
     *            预期任务结果码
     */
    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode ) {
        checkIndexTask( db, taskTypeDesc, csName, clName, indexName, resultCode,
                true );
    }

    /**
     * @description: 检查create / drop
     *               index任务信息（通过cl、索引名和任务类型获取任务，检查索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param indexName
     *            检验测索引名
     * @param resultCode
     *            预期任务结果码
     * @param isCheckGroupInfo
     *            判断是否需要校验任务中组信息，设置为true校验任务中展示组，设置为false不需要校验任务中的组
     */

    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode,
            boolean isCheckGroupInfo ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        matcher.put( "ResultCode", resultCode );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        ArrayList< BSONObject > taskInfos = new ArrayList< BSONObject >();
        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskInfos.add( taskInfo );
            taskNum++;
        }
        cursor.close();
        Assert.assertEquals( taskNum, 1,
                "index task num should be 1!" + taskInfos );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo= " + taskInfo );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask && isCheckGroupInfo ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int groupResultCode = ( int ) groupInfo.get( "ResultCode" );
                Assert.assertEquals( groupResultCode, resultCode,
                        "check group task error! groupName=" + groupName );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            Collections.sort( expGroupNames );
            Collections.sort( groupNames );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo );
        }
    }

    /**
     * @description: 检查create / drop index任务信息结果信息（通过索引名、集合、结果码获取任务）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCode
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param indexName
     *            检验测索引名
     * @resultCode 预期任务结果码
     */
    public static void checkIndexTaskResult( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName, int resultCode ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        matcher.put( "ResultCode", resultCode );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        ArrayList< BSONObject > taskInfos = new ArrayList< BSONObject >();
        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskInfos.add( taskInfo );
            taskNum++;
        }
        cursor.close();
        Assert.assertEquals( taskNum, 1,
                "index task num should be 1!" + taskInfos );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo=" + taskInfo );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int groupResultCode = ( int ) groupInfo.get( "ResultCode" );
                Assert.assertEquals( groupResultCode, resultCode,
                        "check group task error! groupName=" + groupName );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo );
        }
    }

    /**
     * @description: 检查create / drop index任务信息（索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param csName
     * @param clName
     * @param indexName
     *            检验测索引名
     */
    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName ) {
        checkIndexTask( db, taskTypeDesc, csName, clName, indexName, 0 );
    }

    /**
     * @description: 检查create / drop index任务信息（索引名、组名、结果状态码比较）
     * @param taskTypeDesc
     *            任务类型
     * @param resultCodes
     *            任务错误码 成功:0 失败:错误码
     * @param csName
     * @param clName
     * @param indexName
     *            检验测索引名
     */
    public static void checkIndexTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName, String indexName,
            int[] resultCodes ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1,
                "index task num should be 1!" + taskInfo );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        boolean isEqual = intArrayContainsField( resultCodes, actResultCode );
        Assert.assertTrue( isEqual,
                "The act resultCode=" + actResultCode + ",act resultCodes = "
                        + resultCodes.toString() + ",taskInfo=" + taskInfo );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo= " + taskInfo );

        boolean isMainTask = taskInfo.containsField( "IsMainTask" );
        if ( !isMainTask ) {
            // 校验组信息
            @SuppressWarnings("unchecked")
            List< BSONObject > actGroupInfos = ( List< BSONObject > ) taskInfo
                    .get( "Groups" );
            List< String > groupNames = new ArrayList<>();
            for ( int i = 0; i < actGroupInfos.size(); i++ ) {
                BSONObject groupInfo = actGroupInfos.get( i );
                String groupName = ( String ) groupInfo.get( "GroupName" );
                groupNames.add( groupName );
                int actResultCode1 = ( int ) groupInfo.get( "ResultCode" );
                assertTrue(
                        intArrayContainsField( resultCodes, actResultCode1 ),
                        "check resultcode error! groupName=" + groupName
                                + ".act resultCode=" + actResultCode1 );
            }

            List< String > expGroupNames = getCLGroupNames( db, csName,
                    clName );
            Collections.sort( expGroupNames );
            Collections.sort( groupNames );
            Assert.assertEquals( groupNames, expGroupNames,
                    "check group error!task=" + taskInfo );
        }
    }

    /**
     * @description: 判断int数组中是否存在某个值
     * @param intArr
     * @param intValue
     *            需要找的值
     * @return 存在返回true，不存在返回false *
     */
    public static boolean intArrayContainsField( int[] intArr, int intValue ) {
        String value = intValue + "";
        for ( int i : intArr ) {
            if ( value.equals( i + "" ) ) {
                return true;
            }
        }
        return false;
    }

    /**
     * @description: 检查未创建任务
     * @param db
     *            db连接
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param taskTypeDesc
     *            //任务类型
     */
    public static void checkNoTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        int taskNum = 0;
        BSONObject taskInfo = null;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        Assert.assertEquals( taskNum, 0,
                "check task should be no exist! act task =" + taskInfo );
        cursor.close();
    }

    public static boolean isExistTask( Sequoiadb db, String taskTypeDesc,
            String csName, String clName ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        DBCursor cursor = db.listTasks( matcher, null, null, null );

        boolean isExist = false;

        while ( cursor.hasNext() ) {
            cursor.getNext();
            isExist = true;
        }
        cursor.close();
        return isExist;
    }

    /**
     * @description: 按查询匹配范围检查查询访问计划信息（检测索引名和查询类型）
     * @param dbcl
     *            集合连接
     * @param matcherCond
     *            查询匹配范围
     * @param scanType
     *            预期扫描类型
     * @param indexName
     *            预期使用索引，如果未使用索引则为“”
     */
    public static void checkExplain( DBCollection dbcl, String matcherCond,
            String scanType, String indexName ) {
        BSONObject matcher = ( BSONObject ) JSON.parse( matcherCond );
        BSONObject explainOption = ( BSONObject ) JSON.parse( "{Run:true}" );
        DBCursor explainCursor = dbcl.explain( matcher, null, null, null, 0, -1,
                0, explainOption );
        BSONObject explainInfo = null;
        while ( explainCursor.hasNext() ) {
            explainInfo = explainCursor.getNext();
            boolean isMainCLExplain = explainInfo
                    .containsField( "SubCollections" );
            if ( isMainCLExplain ) {
                BasicBSONList subInfoList = ( BasicBSONList ) explainInfo
                        .get( "SubCollections" );
                BSONObject subInfo = ( BSONObject ) subInfoList.get( 0 );
                String actScanType = ( String ) subInfo.get( "ScanType" );
                String actIndexName = ( String ) subInfo.get( "IndexName" );
                Assert.assertEquals( actScanType, scanType );
                Assert.assertEquals( actIndexName, indexName );
            } else {
                String actScanType = ( String ) explainInfo.get( "ScanType" );
                String actIndexName = ( String ) explainInfo.get( "IndexName" );
                Assert.assertEquals( actScanType, scanType );
                Assert.assertEquals( actIndexName, indexName );
            }
        }
        explainCursor.close();
    }

    /**
     * @description: 切分后检查切分组上索引信息（检测索引名和查询类型）
     * @param sdb
     *            coord连接
     * @param groupName
     *            切分组（源组或者目标组）
     * @param indexName
     *            预期使用索引，如果未使用索引则为“”
     * @param csName
     *            cs名
     * @param clName
     *            cl名
     * @param isExistIndex
     *            预期索引是否存在，存在为true，不存在为false
     */
    public static void checkIsExistIndexOnGroup( Sequoiadb sdb,
            String groupName, String indexName, String csName, String clName,
            boolean isExistIndex ) {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        String hostName = group.getMaster().getHostName();
        int port = group.getMaster().getPort();

        try ( Sequoiadb db = new Sequoiadb( hostName + ":" + port, "", "" )) {
            if ( isExistIndex ) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                boolean actIsExistIndex = dbcl.isIndexExist( indexName );
                assertTrue( actIsExistIndex,
                        indexName + " should be exist on the group！" );
            } else {
                try {
                    DBCollection dbcl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    boolean actIsExistIndex = dbcl.isIndexExist( indexName );
                    Assert.assertFalse( actIsExistIndex,
                            indexName + " should be not exist on the group！" );
                } catch ( BaseException e ) {
                    // cl被删除后无法查看索引，索引同步删除
                    if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                    .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }

    /**
     * @description: 随机获取group一个节点
     * @param {Sequoiadb}
     *            db
     * @param {String}
     *            groupName
     * @return {*}
     */
    public static String getGroupOneNode( Sequoiadb db, String groupName ) {

        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getGroupNodes( db, groupName );
        Random random = new Random();
        int serialNum = random.nextInt( nodeAddrs.size() );
        String nodeName = nodeAddrs.get( serialNum ).getString( "hostName" )
                + ":" + nodeAddrs.get( serialNum ).getString( "svcName" );
        return nodeName;
    }

    /**
     * @description: 随机获取CL的一个节点
     * @param {Sequoiadb}
     *            db
     * @param {String}
     *            csName
     * @param {String}
     *            clName
     * @return {*}
     */
    public static String getCLOneNode( Sequoiadb db, String csName,
            String clName ) {
        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getCLNodes( db, csName, clName );
        Random random = new Random();
        int serialNum = random.nextInt( nodeAddrs.size() );
        String nodeName = nodeAddrs.get( serialNum ).getString( "hostName" )
                + ":" + nodeAddrs.get( serialNum ).getString( "svcName" );
        return nodeName;
    }

    /**
     * @description: 检测节点上是否存在本地索引信息
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     * @param indexNodeName
     * @param isExist
     */
    public static void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, String indexName, String indexNodeName,
            Boolean isExist ) throws Exception {
        // 校验lsn是否一致
        List< String > groupNames = getCLGroupNames( db, csName, clName );
        for ( int i = 0; i < groupNames.size(); i++ ) {
            Assert.assertTrue( isLSNConsistency( db, groupNames.get( i ) ) );
        }

        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                if ( isExist ) {
                    if ( indexNodeName.equals( nodeUrl ) ) {
                        assertTrue( dbcl.isIndexExist( indexName ) );
                    } else {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ) );
                    }
                } else {
                    if ( indexNodeName.equals( nodeUrl ) ) {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ) );
                    }
                }
            }
        }
    }

    /**
     * @description: 检测节点上是否存在本地索引信息
     * @param db
     * @param csName
     * @param clName
     * @param indexName
     * @param indexNodeNames
     *            多个创建索引节点
     * @param isExist
     */
    public static void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, String indexName, List< String > indexNodeNames,
            Boolean isExist ) throws Exception {
        // 校验lsn是否一致
        List< String > groupNames = getCLGroupNames( db, csName, clName );
        for ( int i = 0; i < groupNames.size(); i++ ) {
            Assert.assertTrue( isLSNConsistency( db, groupNames.get( i ) ) );
        }

        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                if ( isExist ) {
                    if ( indexNodeNames.contains( nodeUrl ) ) {
                        assertTrue( dbcl.isIndexExist( indexName ),
                                indexName + " should be in " + nodeUrl );
                    } else {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ),
                                indexName + " should not be in " + nodeUrl );
                    }
                } else {
                    if ( indexNodeNames.contains( nodeUrl ) ) {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ),
                                indexName + " should not be in " + nodeUrl );
                    }
                }
            }
        }
    }

    /**
     * @description: 检测节点上是否存在本地索引信息
     * @param db
     * @param csName
     * @param clName
     * @param indexNames
     *            多个索引名
     * @param indexNodeName
     * @param isExist
     */
    public static void checkStandaloneIndexOnNode( Sequoiadb db, String csName,
            String clName, List< String > indexNames, String indexNodeName,
            boolean isExist ) throws Exception {
        // 校验lsn是否一致
        List< String > groupNames = getCLGroupNames( db, csName, clName );
        for ( int i = 0; i < groupNames.size(); i++ ) {
            Assert.assertTrue( isLSNConsistency( db, groupNames.get( i ) ) );
        }

        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                for ( int i = 0; i < indexNames.size(); i++ ) {
                    String indexName = indexNames.get( i );
                    if ( isExist ) {
                        if ( indexNodeName.equals( nodeUrl ) ) {
                            assertTrue( dbcl.isIndexExist( indexName ),
                                    indexName + " should be in "
                                            + indexNodeName );
                        } else {
                            Assert.assertFalse( dbcl.isIndexExist( indexName ),
                                    indexName + " should not be in "
                                            + nodeUrl );
                        }
                    } else {
                        Assert.assertFalse( dbcl.isIndexExist( indexName ),
                                indexName + " should not be in " + nodeUrl );
                    }
                }
            }
        }
    }

    /**
     * @description 检查独立索引任务
     * @param db
     * @param taskTypeDesc
     * @param csName
     * @param clName
     * @param nodeName
     * @param indexName
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName ) {
        checkStandaloneIndexTask( db, taskTypeDesc, csName, clName, nodeName,
                indexName, 0 );
    }

    /**
     * @description 检查独立索引任务
     * @param db
     * @param taskTypeDesc
     * @param csName
     * @param clName
     * @param nodeName
     * @param indexName
     * @param resultCode
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName, int resultCode ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        BSONObject taskInfo = null;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, 1, "index task num should be 1!" );
        // 校验结果状态码
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode,
                "actTaskInfo= " + taskInfo );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status, "actTaskInfo= " + taskInfo );

        // 校验索引名
        String actIndexName = ( String ) taskInfo.get( "IndexName" );
        Assert.assertEquals( actIndexName, indexName,
                "actTaskInfo= " + taskInfo );

        // 校验节点信息
        String actNodeName = ( String ) taskInfo.get( "NodeName" );
        Assert.assertEquals( actNodeName, nodeName,
                "actTaskInfo= " + taskInfo );

        // 校验独立索引
        BasicBSONObject indexDef = ( BasicBSONObject ) taskInfo
                .get( "IndexDef" );
        Boolean standalone = ( Boolean ) indexDef.get( "Standalone" );
        assertTrue( standalone );
    }

    /**
     * @description 检查create / drop 多个独立索引任务信息（索引名、组名、结果状态码比较）
     * @param db
     * @param taskTypeDesc
     * @param csName
     * @param clName
     * @param nodeName
     *            创建索引指定的节点名
     * @param indexNames
     *            多个索引名
     */
    public static void checkStandaloneIndexTasks( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            List< String > indexNames ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        List< BSONObject > taskInfos = new ArrayList<>();
        List< String > actIndexNames = new ArrayList<>();
        int status = 9;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            BSONObject taskInfo = cursor.getNext();
            taskInfos.add( taskInfo );
            String indexName = ( String ) taskInfo.get( "IndexName" );
            actIndexNames.add( indexName );
            // 校验结果状态码
            int actResultCode = ( int ) taskInfo.get( "ResultCode" );
            Assert.assertEquals( actResultCode, 0, "The act resultCode="
                    + actResultCode + ",task info = " + taskInfo );

            int actStatus = ( int ) taskInfo.get( "Status" );
            Assert.assertEquals( actStatus, status,
                    "check status fail! task =" + taskInfo );

            // 校验节点信息
            String actNodeName = ( String ) taskInfo.get( "NodeName" );
            Assert.assertEquals( actNodeName, nodeName,
                    "check nodename fail! task=" + taskInfo );

            // 校验独立索引
            BasicBSONObject indexDef = ( BasicBSONObject ) taskInfo
                    .get( "IndexDef" );
            Boolean standalone = ( Boolean ) indexDef.get( "Standalone" );
            assertTrue( standalone, "check standalone fail! task=" + taskInfo );
            taskNum++;
        }
        cursor.close();

        // 校验索引名
        Collections.sort( actIndexNames );
        Collections.sort( indexNames );
        Assert.assertEquals( actIndexNames, indexNames,
                "check indexName fail! tasks = " + taskInfos );
        // 检查任务数量
        Assert.assertEquals( taskNum, indexNames.size(),
                "check index num error! taskInfo=" + taskInfos );
    }

    /**
     * @description 检查create / drop 独立索引任务信息（索引名、组名、结果状态码比较）
     * @param db
     * @param taskTypeDesc
     *            任务类型
     * @param csName
     * @param clName
     * @param nodeName
     *            指定创建独立索引的节点名
     * @param indexName
     * @param resultCodes
     *            预期任务错误码，针对多个任务不同错误码校验可以指定多个错误码
     */
    public static void checkStandaloneIndexTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName, int[] resultCodes, int taskNums ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        BSONObject taskInfo = null;
        int status = 9;
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            // 校验结果状态码
            int actResultCode = ( int ) taskInfo.get( "ResultCode" );
            boolean isEqual = intArrayContainsField( resultCodes,
                    actResultCode );
            assertTrue( isEqual,
                    "The act resultCode=" + actResultCode
                            + ",act resultCodes = " + resultCodes.toString()
                            + ", taskInfo =" + taskInfo );

            int actStatus = ( int ) taskInfo.get( "Status" );
            Assert.assertEquals( actStatus, status, "taskInfo =" + taskInfo );

            // 校验索引名
            String actIndexName = ( String ) taskInfo.get( "IndexName" );
            Assert.assertEquals( actIndexName, indexName,
                    "taskInfo =" + taskInfo );

            // 校验节点信息
            String actNodeName = ( String ) taskInfo.get( "NodeName" );
            Assert.assertEquals( actNodeName, nodeName,
                    "taskInfo =" + taskInfo );

            // 校验独立索引
            BasicBSONObject indexDef = ( BasicBSONObject ) taskInfo
                    .get( "IndexDef" );
            Boolean standalone = ( Boolean ) indexDef.get( "Standalone" );
            assertTrue( standalone, "taskInfo =" + taskInfo );
            taskNum++;
        }
        cursor.close();

        Assert.assertEquals( taskNum, taskNums,
                "index task num should be 1! indexTask is " + taskInfo );
    }

    /**
     * @description 检查节点上是否存在独立索引任务
     * @param db
     * @param taskTypeDesc
     *            任务类型
     * @param csName
     * @param clName
     * @param nodeName
     *            指定创建独立索引的节点名
     * @param indexName
     */
    public static void checkNoIndexStandaloneTask( Sequoiadb db,
            String taskTypeDesc, String csName, String clName, String nodeName,
            String indexName ) {
        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + clName );
        matcher.put( "TaskTypeDesc", taskTypeDesc );
        matcher.put( "NodeName", nodeName );
        matcher.put( "IndexName", indexName );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TASKS, matcher,
                null, null );

        int taskNum = 0;
        BSONObject taskInfo = null;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();
        Assert.assertEquals( taskNum, 0,
                "check task should be no exist! act task =" + taskInfo );
    }
}
