package com.sequoiadb.fulltext.utils;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 全文索引的公共类，检查方法、其他与DB端和ES内部操作无关的方法均可放于此类
 */
public class FullTextUtils {

    // 插入记录数，所有用例公用此变量
    public static final int INSERT_NUMS = 200000; // insert 20w datas
    // 线程超时时间，所有用例公用此变量
    public static final int THREAD_TIMEOUT = 600000; // timeout 10 mins
    // 全文索引前缀
    private static String FULLTEXTPREFIX;

    // 初始化全文索引的前缀名作为全局变量
    public static void setFulltextPrefix( final String fulltextPrefix ) {
        FullTextUtils.FULLTEXTPREFIX = fulltextPrefix;
    }

    // 获取全文索引的前缀名
    public static String getFulltextPrefix() {
        return FullTextUtils.FULLTEXTPREFIX;
    }

    /**
     * 检查DB端中普通表或分区表下的全文索引数据是否完全同步到ES端，总共分三层检查: 1.先检查文索引名是否都映射到ES端
     * 2.再检查ES端全文索引的总记录数是否正确
     * 3.最后检查DB端各个固定集合的最大一条LID记录是否与对应ES端全文索引的SDBCOMMITID值一致
     * 
     * @param esClient
     * @param cl
     * @param textIndexName
     * @param expectCount
     * @return boolean 如果ES端的全文索引完成同步则返回true，否则抛出检查失败异常
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    private static boolean isFullSyncToES( DBCollection cl,
            String textIndexName, int expectCount ) throws Exception {
        List< String > esIndexNames = FullTextDBUtils.getESIndexNames( cl,
                textIndexName );
        List< DBCollection > cappedCLs = FullTextDBUtils.getCappedCLs( cl,
                textIndexName );

        if ( !isFulltextRebuild( cl, textIndexName ) ) {
            throw new Exception(
                    "The " + cl.getFullName() + "'s textIndex: " + textIndexName
                            + " didn't rebuild to completed in the ES" );
        }

        // 检查索引数是否已完全同步到ES
        if ( !isCountRightInES( esIndexNames, expectCount ) ) {
            throw new Exception( cl.getFullName() + " fulltext: " + esIndexNames
                    + " are not all sync to es" );
        }
        // 检查固定集合的最后一条lid是否等于ES端SDBCOMMIT._id
        if ( !isLastLidInES( esIndexNames, cappedCLs ) ) {
            throw new Exception( cl.getFullName()
                    + " cappedCL last record lid unequal to es SDBCOMMIT._id" );
        }

        return true;
    }

    /**
     * 检查DB端中主子表下的全文索引数据是否完全同步到ES端，总共分三层检查： 1. 先检查子表的全文索引名是否都映射到ES端 2.
     * 再检查ES端子表的全文索引总记录数是否正确 3.
     * 最后检查DB端各个固定集合的最大一条LID记录是否与对应ES端全文索引的dbCOMMITID值一致
     * 
     * @param esClient
     * @param cl
     * @param textIndexName
     * @param expectCount
     * @return boolean 如果主子表在ES端的全文索引完成同步则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    private static boolean isMainCLFullSyncToES( DBCollection cl,
            String textIndexName, int expectCount ) throws Exception {
        Sequoiadb db = cl.getSequoiadb();
        List< String > subCLFullNames = FullTextDBUtils.getSubCLNames( db,
                cl.getFullName() );

        // 获取主表下所有子表的全文索引和固定集合对象
        List< String > esIndexNames = new ArrayList< String >();
        List< DBCollection > cappedCLs = new ArrayList< DBCollection >();
        for ( String subCLFullName : subCLFullNames ) {
            String subCSName = subCLFullName.split( "\\." )[ 0 ];
            String subCLName = subCLFullName.split( "\\." )[ 1 ];
            DBCollection subCL = db.getCollectionSpace( subCSName )
                    .getCollection( subCLName );
            esIndexNames.addAll(
                    FullTextDBUtils.getESIndexNames( subCL, textIndexName ) );
            cappedCLs.addAll(
                    FullTextDBUtils.getCappedCLs( subCL, textIndexName ) );

            if ( !isFulltextRebuild( subCL, textIndexName ) ) {
                throw new Exception( "The " + subCL.getFullName()
                        + "'s textIndex: " + textIndexName
                        + " didn't rebuild to completed in the ES" );
            }
        }

        // 检查索引数是否已完全同步到ES
        if ( !isCountRightInES( esIndexNames, expectCount ) ) {
            return false;
        }
        // 检查固定集合的最后一条lid是否等于ES端SDBCOMMIT._id
        if ( !isLastLidInES( esIndexNames, cappedCLs ) ) {
            return false;
        }

        return true;
    }

    /**
     * 检查ES端全文索引总记录数是否正确， 若原始集合中包含多个全文索引，则总记录数为所有全文索引记录数的总和
     * 
     * @param esClient
     * @param esIndexNames
     * @param expectCount
     * @return boolean 如果ES端的全文索引记录数正确则返回true，否则返回false
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static boolean isCountRightInES( List< String > esIndexNames,
            int expectCount ) throws Exception {
        boolean isSync = false;
        int timeout = 600; // 超时 10min
        int interval = 1; // 每次检测间隔时间1s
        int doTimes = 0;
        int actCount = 0;

        while ( doTimes * interval < timeout ) {
            actCount = 0;
            // 所有索引的记录数总和
            for ( String esIndexName : esIndexNames ) {
                actCount += ( FullTextESUtils.getCountFromES( esIndexName ) );
            }

            if ( actCount == expectCount ) {
                isSync = true;
                break;
            } else {
                doTimes++;
                if ( doTimes % 60 == 0 ) {
                    System.out.println( "esIndexNames: "
                            + esIndexNames.toString() + ", doTimes: " + doTimes
                            + ", actCount: " + actCount + ", expectCount: "
                            + expectCount );
                }
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }
        }
        // 同步失败后，打印所有索引名
        if ( !isSync ) {
            System.err.println( "check " + esIndexNames.toString()
                    + " count syn to es timeout" );
        }
        return isSync;
    }

    /**
     * 检查DB端各个固定集合的最大一条LID记录是否与对应ES端全文索引的dbCOMMITID值一致， 一个全文索引对应一个固定集合
     * 每个全文索引数组元素与每个固定集合对象数组元素一一对应
     * 
     * @param esClient
     * @param esIndexNames
     * @param cappedCLs
     * @return boolean 如果ES端的SDBCOMMIT._lid的值与对应固定集合最大一条lid的值一致则返回true，否则返回false
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static boolean isLastLidInES( List< String > esIndexNames,
            List< DBCollection > cappedCLs ) throws Exception {
        boolean isSync = false;
        int timeout = 600; // 超时 10min
        int interval = 1; // 每次检测间隔时间1s
        int doTimes;

        // 获取每个数据组主节点下的固定集合最大一条lid
        List< Integer > lastLogicalIDs = new ArrayList<>();
        for ( DBCollection cappedCL : cappedCLs ) {
            lastLogicalIDs.add( new FullTextDBUtils().getLastLid( cappedCL ) );
        }

        // 检查每个全文索引的SDBCOMMITID与对应固定集合的最大一条lid是否相同
        for ( int i = 0; i < esIndexNames.size(); i++ ) {
            doTimes = 0;
            Integer commitID = -10000;
            while ( doTimes * interval < timeout ) {
                commitID = FullTextESUtils
                        .getCommitIDFromES( esIndexNames.get( i ) );
                if ( commitID.intValue() != lastLogicalIDs.get( i )
                        .intValue() ) {
                    isSync = false;
                } else {
                    isSync = true;
                }

                if ( isSync ) {
                    break;
                } else {
                    doTimes++;
                    if ( doTimes % 60 == 0 ) {
                        System.out.println( "esIndexName: "
                                + esIndexNames.get( i ).toString()
                                + ", doTimes: " + doTimes + ", commitID: "
                                + commitID + ", lastLogicalID: "
                                + lastLogicalIDs.get( i ).toString() );
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                }
            }
            // 如果最终没有完成同步则打屏
            if ( !isSync ) {
                System.err.println( "check " + esIndexNames.get( i ).toString()
                        + " lid syn to es timeout" );
                break;
            }
        }
        return isSync;
    }

    /**
     * 检查DB端单个集合（普通表或分区表）反复创建删除同一个索引的次数是否与对应ES端全文索引的idxlid值一致
     * 
     * @param esIndexName
     * @param expectIdxLid
     * @return boolean
     *         如果ES端的SDBCOMMIT._idxlid的值与db端反复创建删除同一个索引的次数相同则返回true，否则返回false
     * @Author liuxiaoxuan
     * @Date 2019-08-14
     */
    public static boolean isIdxLidSyncInES( String esIndexName,
            int expectIdxLid ) throws Exception {
        boolean isSync = false;
        int timeout = 600; // 超时 10min
        int interval = 1; // 每次检测间隔时间1s
        int doTimes = 0;

        // 检查每个全文索引的SDBCOMMITID._idxlid与预期的索引lid是否相等
        int commitIdxID = -10000;
        while ( doTimes * interval < timeout ) {
            try {
                // 获取每个全文索引对应的SDBCOMMITID._idxlid
                commitIdxID = FullTextESUtils
                        .getCommitIDXLIDFromES( esIndexName );
                if ( commitIdxID != expectIdxLid ) {
                    isSync = false;
                } else {
                    isSync = true;
                }

                if ( isSync ) {
                    break;
                } else {
                    doTimes++;
                    if ( doTimes % 60 == 0 ) {
                        System.out.println( "esIndexName: " + esIndexName
                                + ", doTimes: " + doTimes + ", commitIdxID: "
                                + commitIdxID + ", expectIdxLid: "
                                + expectIdxLid );
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                }
            } catch ( Exception e ) {
                // 若索引不存在，则休眠1s再继续获取SDBCOMMITID._idxlid
                if ( e.getMessage().equals( "no such index" ) ) {
                    doTimes++;
                    if ( doTimes % 30 == 0 ) {
                        System.out.println( "esIndexName: " + esIndexName
                                + ", doTimes: " + doTimes
                                + " is not exist while getting idxlid" );
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e2 ) {
                        e2.printStackTrace();
                    }
                } else {
                    System.out.println(
                            "isIdxLidSyncInES exception: " + e.getMessage() );
                    throw e;
                }
            }
        }
        return isSync;
    }

    /**
     * 检查DB端单个集合（普通表或分区表）反复创建删除同一个索引的次数是否与对应ES端全文索引的idxlid值一致
     * 
     * @param esIndexNames
     * @param expectIdxLids
     * @return boolean
     *         如果ES端的SDBCOMMIT._idxlid的值与db端反复创建删除同一个索引的次数相同则返回true，否则返回false
     * @Author liuxiaoxuan
     * @Date 2019-08-14
     */
    public static boolean isIdxLidSyncInES( List< String > esIndexNames,
            List< Integer > expectIdxLids ) throws Exception {
        boolean isSync = false;
        // 检查每个全文索引的SDBCOMMITID._idxlid与预期的索引lid是否相等
        for ( int i = 0; i < esIndexNames.size(); i++ ) {
            isSync = isIdxLidSyncInES( esIndexNames.get( i ),
                    expectIdxLids.get( i ) );
            // 如果最终没有完成同步则打屏
            if ( !isSync ) {
                System.err.println( "check " + esIndexNames.get( i ).toString()
                        + " _idxlid syn to es timeout" );
                break;
            }
        }
        return isSync;
    }

    /**
     * ES端的cllid与原始集合的LogicalID一致,通过原始集合的LogicalID作为预期结果来判断全文索引是否重建
     * 检查原始集合内的多个全文索引
     * 
     * @param esClient
     *            es连接
     * @param cl
     *            原始集合
     * @param indexNames
     *            多个原始集合索引名
     * @return boolean 如果原始集合LogicalID与ES端全文索引_cllid一致则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-16
     */
    // 目前集合不支持创建多个全文索引,暂时屏蔽该方法,避免调用错误
    @SuppressWarnings("unused")
    private static boolean isFulltextRebuild( DBCollection cl,
            List< String > indexNames ) throws Exception {
        // 检查每个全文索引下的_cllid值有没有变化
        for ( String indexName : indexNames ) {
            List< String > esIndexNames = FullTextDBUtils.getESIndexNames( cl,
                    indexName );
            for ( String esIndexName : esIndexNames ) {
                if ( !isLogicalIDEqualCLLid( cl, esIndexName ) ) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * ES端的cllid与原始集合的LogicalID一致,通过原始集合的LogicalID作为预期结果来判断全文索引是否重建
     * 检查原始集合内的多个全文索引
     * 
     * @param esClient
     *            es连接
     * @param cl
     *            原始集合
     * @param indexName
     *            原始集合索引名
     * @return boolean 如果原始集合LogicalID与ES端全文索引_cllid一致则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-16
     */
    private static boolean isFulltextRebuild( DBCollection cl,
            String indexName ) throws Exception {
        // 检查每个全文索引下的_cllid值有没有变化
        List< String > esIndexNames = FullTextDBUtils.getESIndexNames( cl,
                indexName );
        for ( String esIndexName : esIndexNames ) {
            if ( !isLogicalIDEqualCLLid( cl, esIndexName ) ) {
                return false;
            }
        }
        return true;
    }

    /**
     * ES端的cllid与原始集合的LogicalID一致,通过原始集合的LogicalID作为预期结果来判断全文索引是否重建
     * FullTextESUtils.getCommitCLLIDFromES ( esClient, esIndexNames )
     * 获取每个全文索引对应的SDBCOMMIT._cllid
     * 
     * @param esClient
     *            es连接
     * @param cl
     *            原始集合
     * @param esIndexName
     *            ES端全文索引名
     * @return boolean 如果原始集合LogicalID与ES端全文索引_cllid一致则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-16
     */
    private static boolean isLogicalIDEqualCLLid( DBCollection cl,
            String esIndexName ) throws Exception {
        boolean isSync = false;
        int preCLLid = -1;
        int timeout = 600; // timeout 10min
        int interval = 1; // interval 1s
        int doTimes;

        Sequoiadb db = cl.getSequoiadb();
        String[] strList = esIndexName.split( "_" );
        String groupName = strList[ strList.length - 1 ];
        Sequoiadb masterNode = db.getReplicaGroup( groupName ).getMaster()
                .connect();
        DBCursor snapCur = masterNode.getSnapshot(
                Sequoiadb.SDB_SNAP_COLLECTIONS,
                "{Name: '" + cl.getFullName() + "'}", null, null );
        if ( snapCur.hasNext() ) {
            @SuppressWarnings("unchecked")
            List< BSONObject > details = ( List< BSONObject > ) snapCur
                    .getNext().get( "Details" );
            preCLLid = ( int ) details.get( 0 ).get( "LogicalID" );
            snapCur.close();
        } else {
            snapCur.close();
            throw new Exception( cl.getFullName()
                    + " SDB_SNAP_COLLECTIONS was not found in the "
                    + masterNode.getNodeName() );
        }

        // 检查全文索引下的_cllid值有没有变化
        doTimes = 0;
        Integer curCLLID = -10000;
        while ( doTimes * interval < timeout ) {
            try {
                curCLLID = FullTextESUtils.getCommitCLLIDFromES( esIndexName );
                if ( curCLLID == preCLLid ) {
                    isSync = true;
                } else {
                    isSync = false;
                }

                if ( isSync ) {
                    break;
                } else {
                    doTimes++;
                    if ( doTimes % 60 == 0 ) {
                        System.out.println( "esIndexName: " + esIndexName
                                + ",doTimes: " + doTimes + ", previousCLLid: "
                                + preCLLid + ", currentCLLID: " + curCLLID );
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                }
            } catch ( Exception e ) {
                if ( e.getMessage().equals( "no such index" ) ) {
                    doTimes++;
                    if ( doTimes % 30 == 0 ) {
                        System.out.println( "esIndexName: " + esIndexName
                                + ", doTimes: " + doTimes
                                + " is not exist or being truncated now" );
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e2 ) {
                        e2.printStackTrace();
                    }
                } else {
                    System.out.println(
                            "isFulltextRebuild exception: " + e.getMessage() );
                    throw e;
                }
            }
        }

        return isSync;
    }

    /**
     * 检查主备节点下原始集合和固定集合的数据一致性
     * 
     * @param cl
     * @param textIndexName
     * @return boolean 如果主备节点数据一致则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-09
     */
    private static boolean isDataConsistency( DBCollection cl,
            String textIndexName ) throws Exception {

        // 判断所有节点是否已同步集合
        if ( !isCLConsistency( cl ) ) {
            return false;
        }

        // 检查索引信息
        if ( !isIndexConsistency( cl, textIndexName ) ) {
            return false;
        }

        // 检查主备节点原始集合的数据一致性
        if ( !isCLDataConsistency( cl ) ) {
            return false;
        }
        if ( !isCappedCLDataConsistency( cl, textIndexName ) ) {
            return false;
        }

        return true;
    }

    /**
     * 检查主备节点索引信息一致
     * 
     * @param cl
     * @param indexName
     * @return
     * @throws Exception
     */
    public static boolean isIndexConsistency( DBCollection cl,
            String indexName ) throws Exception {

        Sequoiadb db = cl.getSequoiadb();
        String csName = cl.getCSName();
        String clName = cl.getName();
        boolean isConsistency = false;

        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );
        for ( String groupName : groupNames ) {
            isConsistency = false;
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );
            Sequoiadb masterNode = rg.getMaster().connect();
            DBCollection masterCL = masterNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !masterCL.isIndexExist( indexName ) ) {
                for ( String nodeName : nodeNames ) {
                    if ( masterNode.getNodeName().equals( nodeName ) ) {
                        continue;
                    }
                    Sequoiadb nodeConn = rg.getNode( nodeName ).connect();
                    DBCollection nodeCL = nodeConn.getCollectionSpace( csName )
                            .getCollection( clName );
                    if ( nodeCL.isIndexExist( indexName ) ) {
                        throw new Exception( cl.getFullName()
                                + " the index info is different, masterNode: "
                                + masterNode.getNodeName()
                                + " not exists indexName: " + indexName
                                + ", but slaveNode: " + nodeName
                                + " exists indexName: " + indexName );
                    }
                }
            } else {
                BSONObject indexInfo = ( BSONObject ) ( ( BSONObject ) masterCL
                        .getIndexInfo( indexName ) );
                BSONObject indexDef = ( BSONObject ) indexInfo
                        .get( "IndexDef" );
                indexDef.removeField( "CreateTime" );
                indexDef.removeField( "RebuildTime" );
                indexInfo.put( "IndexDef", indexDef );
                for ( String nodeName : nodeNames ) {
                    if ( masterNode.getNodeName().equals( nodeName ) ) {
                        continue;
                    }
                    Sequoiadb nodeConn = rg.getNode( nodeName ).connect();
                    DBCollection nodeCL = nodeConn.getCollectionSpace( csName )
                            .getCollection( clName );
                    if ( !nodeCL.isIndexExist( indexName ) ) {
                        throw new Exception( cl.getFullName()
                                + " the index info is different, masterNode: "
                                + masterNode.getNodeName()
                                + " exists indexName: " + indexName
                                + ", but slaveNode: " + nodeName
                                + " not exists indexName: " + indexName );
                    }
                    BSONObject checkIndexInfo = nodeCL
                            .getIndexInfo( indexName );
                    BSONObject checkIndexDef = ( BSONObject ) checkIndexInfo
                            .get( "IndexDef" );
                    checkIndexDef.removeField( "CreateTime" );
                    checkIndexDef.removeField( "RebuildTime" );
                    checkIndexInfo.put( "IndexDef", checkIndexDef );
                    if ( !indexInfo.equals( checkIndexInfo ) ) {
                        throw new Exception( cl.getFullName()
                                + " the index info is different, masterNode "
                                + masterNode.getNodeName() + ": " + indexInfo
                                + ", " + nodeName + ": " + checkIndexInfo );
                    }
                }
            }

            isConsistency = true;
        }

        return isConsistency;
    }

    /**
     * 检查主备节点的普通表、分区表数据一致性： 1. 先校验主备节点原始集合记录数是否一致 2. 再检验主备节点原始集合每一条记录内容是否一致
     *
     * @param cl
     * @return boolean 如果主备节点原始集合的数据一致则返回true，否则返回false
     * @throws Exception
     * @Author yinzhen
     * @Date 2018-12-21
     */
    public static boolean isCLDataConsistency( DBCollection cl )
            throws Exception {
        boolean isConsistency = false;
        Sequoiadb db = cl.getSequoiadb();
        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );

        for ( String groupName : groupNames ) {
            isConsistency = isConsistency( db, groupName, cl.getCSName(),
                    cl.getName() );
            if ( !isConsistency ) {
                throw new Exception( cl.getCSName()
                        + " is not consistency in the " + groupName );
            }
        }
        return isConsistency;
    }

    /**
     * 检查主备节点的固定集合数据一致性 1. 先校验主备节点固定集合记录数是否一致 2. 再检验主备节点固定集合每一条记录内容是否一致
     * 
     * @param cl
     * @param textIndexName
     * @return boolean 如果主备节点固定集合的数据一致则返回true，否则返回false
     * @throws Exception
     * @Author liuxiaoxuan
     * @Date 2019-05-09
     */
    public static boolean isCappedCLDataConsistency( DBCollection cl,
            String textIndexName ) throws Exception {
        boolean isConsistency = false;
        Sequoiadb db = cl.getSequoiadb();
        String cappedName = FullTextDBUtils.getCappedName( cl, textIndexName );
        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );

        for ( String groupName : groupNames ) {
            isConsistency = isConsistency( db, groupName, cappedName,
                    cappedName );
            if ( !isConsistency ) {
                throw new Exception( cappedName
                        + " cappedcl is not consistency in the " + groupName );
            }
        }
        return isConsistency;
    }

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

        List< String > groupNames = FullTextDBUtils.getCLGroups( cl );
        for ( String groupName : groupNames ) {
            List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
            ReplicaGroup rg = db.getReplicaGroup( groupName );

            try ( Sequoiadb masterNode = rg.getMaster().connect()) {
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
                        long checkCompleteLSN = -3;
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
     * 检查主备节点主子表的原始集合和固定集合的数据一致性
     * 
     * @param cl
     * @param textIndexName
     * @return boolean 如果主备节点主子表的数据一致则返回true，否则返回false
     * @throws Exception
     * @Author yinzhen
     * @Date 2018-12-21
     */
    private static boolean isMainCLDataConsistency( DBCollection cl,
            String textIndexName ) throws Exception {
        boolean isConsistency = true;
        Sequoiadb db = cl.getSequoiadb();
        List< String > subclNames = FullTextDBUtils.getSubCLNames( db,
                cl.getFullName() );
        for ( int i = 0; i < subclNames.size(); i++ ) {
            String subcsName = subclNames.get( i ).split( "\\." )[ 0 ];
            String subclName = subclNames.get( i ).split( "\\." )[ 1 ];
            DBCollection subCL = db.getCollectionSpace( subcsName )
                    .getCollection( subclName );
            isConsistency = isDataConsistency( subCL, textIndexName );
            if ( !isConsistency ) {
                break;
            }
        }
        return isConsistency;
    }

    /**
     * 判断主备节点的集合数据是否一致
     * 
     * @param nodes
     * @param csName
     * @param clName
     * @return boolean
     * @throws Exception
     * @Author yinzhen
     * @Date 2018-12-21
     */
    public static boolean isConsistency( Sequoiadb db, String groupName,
            String csName, String clName ) {
        boolean isConsistency = false;
        int doTimes = 0;
        int timeout = 600;
        while ( true ) {
            isConsistency = isNodeRecordsConsistency( db, groupName, csName,
                    clName );
            if ( isConsistency ) {
                return isConsistency;
            } else {
                doTimes++;
                // System.out.println("csName : " + csName + " clName: " +
                // clName + " isConsistency : " + isConsistency
                // + " , doTimes: " + doTimes);
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }
            if ( doTimes >= timeout ) {
                break;
            }
        }
        return isConsistency;
    }

    /**
     * 判断主备节点的集合数据是否一致
     * 
     * @param nodes
     * @param csName
     * @param clName
     * @return boolean
     * @throws Exception
     * @Author yinzhen
     * @Date 2018-12-21
     */
    public static boolean isNodeRecordsConsistency( Sequoiadb db,
            String groupName, String csName, String clName ) {

        List< String > nodeNames = CommLib.getNodeAddress( db, groupName );
        if ( nodeNames.size() == 1 ) {
            return true;
        }

        ReplicaGroup rg = db.getReplicaGroup( groupName );
        try ( Sequoiadb firstNode = rg.getNode( nodeNames.get( 0 ) )
                .connect()) {
            DBCollection cl1 = firstNode.getCollectionSpace( csName )
                    .getCollection( clName );
            for ( int i = 1; i < nodeNames.size(); i++ ) {

                try ( Sequoiadb nextNode = rg.getNode( nodeNames.get( i ) )
                        .connect()) {
                    DBCollection cl2 = nextNode.getCollectionSpace( csName )
                            .getCollection( clName );
                    if ( cl1.getCount() != cl2.getCount() ) {
                         System.out.println(cl1.getFullName() + " from " +
                         nodeNames.get(0) + "'s count: "
                         + cl1.getCount() + ", cl from " + nodeNames.get(i) +
                         "'s count: " + cl2.getCount());
                        return false;
                    }
                    DBCursor cl1Cursor = cl1.query( null, null, "{\"_id\":1}",
                            null );
                    DBCursor cl2Cursor = cl2.query( null, null, "{\"_id\":1}",
                            null );
                    if ( !isCLRecordsConsistency( cl1Cursor, cl2Cursor ) ) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /**
     * 判断主备节点的集合数据是否一致
     * 
     * @param cl1Cursor
     * @param cl2Cursor
     * @return boolean
     * @throws Exception
     * @Author yinzhen
     * @Date 2018-12-21
     */
    public static boolean isCLRecordsConsistency( DBCursor cl1Cursor,
            DBCursor cl2Cursor ) {
        try {
            while ( cl1Cursor.hasNext() && cl2Cursor.hasNext() ) {
                BSONObject cl1Record = cl1Cursor.getNext();
                BSONObject cl2Record = cl2Cursor.getNext();
                if ( !cl1Record.equals( cl2Record ) ) {
                    System.out.println(
                            "compare record failed, collection from first node's record : "
                                    + cl1Record.toString()
                                    + "\n collection from anohter node's record : "
                                    + cl2Record.toString() );
                    return false;
                }
            }
        } finally {
            cl1Cursor.close();
            cl2Cursor.close();
        }
        return true;
    }

    /**
     * 检查全文索引是否创建,包括检查索引在es端是否被创建,固定集合空间是否被创建,数据是否同步
     * 
     * @param db
     * @param esClient
     * @param indexName
     * @return
     * @throws Exception
     */
    public static boolean isIndexCreated( DBCollection cl, String indexName,
            int expectCount ) throws Exception {

        if ( cl.isIndexExist( indexName )
                && isFullSyncToES( cl, indexName, expectCount )
                && isDataConsistency( cl, indexName )
                && isRecordEqualsByMulQueryMode( cl ) ) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 检查主表全文索引是否创建,包括检查索引在es端是否被创建,固定集合空间是否被创建,数据是否同步
     * 
     * @param db
     * @param esClient
     * @param indexName
     * @return
     * @throws Exception
     */
    public static boolean isMainCLIndexCreated( DBCollection cl,
            String indexName, int expectCount ) throws Exception {

        if ( isMainCLFullSyncToES( cl, indexName, expectCount )
                && isMainCLDataConsistency( cl, indexName )
                && isRecordEqualsByMulQueryMode( cl ) ) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 检查全文索引是否删除,包括检查索引在es端是否被删除,固定集合空间是否被删除
     * 
     * @param db
     * @param esClient
     * @param esIndexName
     * @param cappedName
     * @return boolean 删除成功返回true,否则返回false
     * @throws Exception
     */
    public static boolean isIndexDeleted( Sequoiadb db, String esIndexName,
            String cappedName ) throws Exception {

        if ( new FullTextESUtils().isIndexDeletedInES( esIndexName )
                && new FullTextDBUtils().isCSDropSuccess( db, cappedName ) ) {
            return true;
        } else {
            return false;
        }

    }

    /**
     * 检查全文索引是否删除,包括检查索引在es端是否被删除,固定集合空间是否被删除
     * 
     * @param db
     * @param esClient
     * @param esIndexName
     * @param cappedNames
     * @return boolean 删除成功返回true,否则返回false
     * @throws Exception
     */
    public static boolean isIndexDeleted( Sequoiadb db,
            List< String > esIndexNames, List< String > cappedNames )
            throws Exception {

        if ( new FullTextESUtils().isIndexDeletedInES( esIndexNames )
                && new FullTextDBUtils().isAllCSDropSuccess( db,
                        cappedNames ) ) {
            return true;
        } else {
            return false;
        }

    }

    /**
     * 检查普通查询和全文索引查询结果是否一致
     * 
     * @param cl
     * @return boolean 查询结果一致则返回true,否则返回false
     * @throws Exception
     */
    public static boolean isRecordEqualsByMulQueryMode( DBCollection cl )
            throws Exception {
        String csName = cl.getCSName();
        String clName = cl.getName();
        boolean isEquals = false;

        Sequoiadb db1 = null;
        Sequoiadb db2 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.getDefaultCoordUrl(), "", "" );
            db2 = new Sequoiadb( SdbTestBase.getDefaultCoordUrl(), "", "" );
            DBCollection cl1 = db1.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( clName );
            DBCursor cur1 = cl1.query( "", "", "{_id: 1}", "" );
            DBCursor cur2 = cl2.query(
                    "{'': {'$Text': {'query': {'match_all': {}}}}}", "",
                    "{_id: 1}", "" );
            int checkRecordTimes = 1;
            while ( cur1.hasNext() ) {
                Assert.assertEquals( cur2.getNext(), cur1.getNext(),
                        "check record times: " + checkRecordTimes );
                checkRecordTimes++;
            }

            isEquals = true;
        } finally {
            if ( db1 != null ) {
                db1.close();
            }
            if ( db2 != null ) {
                db2.close();
            }
        }
        return isEquals;
    }

}
