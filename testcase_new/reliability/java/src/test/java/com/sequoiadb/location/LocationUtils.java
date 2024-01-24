package com.sequoiadb.location;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class LocationUtils {

    /**
     * @description 使用两地三中心方式设置Location，需要保证group副本数量为7
     * @param db
     * @param groupName
     *            设置的复制组
     * @param primaryLocation
     *            主中心Location
     * @param sameCityLocation
     *            同城备中心Location
     * @param offsiteLocation
     *            异地备中心Location
     * @return 以["Location":[{"hostName":hostName,"svcName":svcName}]]的形式返回
     */
    public static ArrayList< BasicBSONObject > setTwoCityAndThreeLocation(
            Sequoiadb db, String groupName, String primaryLocation,
            String sameCityLocation, String offsiteLocation ) {
        ArrayList< BasicBSONObject > locationNodeAddrs = new ArrayList<>();
        ArrayList< BasicBSONObject > primaryLocationAddrs = new ArrayList<>();
        ArrayList< BasicBSONObject > sameCityLocationAddrs = new ArrayList<>();
        ArrayList< BasicBSONObject > offsiteLocationAddrs = new ArrayList<>();

        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getGroupNodes( db, groupName );
        if ( nodeAddrs.size() != 7 ) {
            Assert.fail( "the numbwe of nodes is not 7, " + nodeAddrs );
        }

        ReplicaGroup rg = db.getReplicaGroup( groupName );
        // 给主节点设置Location
        Node masterNode = rg.getMaster();
        masterNode.setLocation( primaryLocation );
        String masterNodeName = masterNode.getNodeName();
        primaryLocationAddrs
                .add( new BasicBSONObject( "nodeName", masterNodeName ) );

        ArrayList< BasicBSONObject > slaveNodeAddrs = new ArrayList<>();
        slaveNodeAddrs = getGroupSlaveNodes( db, groupName );
        if ( slaveNodeAddrs.size() != 6 ) {
            Assert.fail(
                    "the numbwe of slave nodes is not 6, " + slaveNodeAddrs );
        }
        int slaveNodeNum = 0;
        for ( BasicBSONObject slaveNodeAddr : slaveNodeAddrs ) {
            slaveNodeNum++;
            String hostName = ( String ) slaveNodeAddr.get( "hostName" );
            String svcName = ( String ) slaveNodeAddr.get( "svcName" );
            Node slaveNode = rg.getNode( hostName + ":" + svcName );
            if ( slaveNodeNum < 3 ) {
                // 主中心备节点设置Location
                slaveNode.setLocation( primaryLocation );
                primaryLocationAddrs
                        .add( new BasicBSONObject( "hostName", hostName )
                                .append( "svcName", svcName ) );
            } else if ( slaveNodeNum < 5 ) {
                // 同城备中心设置Location
                slaveNode.setLocation( sameCityLocation );
                sameCityLocationAddrs
                        .add( new BasicBSONObject( "hostName", hostName )
                                .append( "svcName", svcName ) );
            } else {
                // 异地备中心设置Location
                slaveNode.setLocation( offsiteLocation );
                offsiteLocationAddrs
                        .add( new BasicBSONObject( "hostName", hostName )
                                .append( "svcName", svcName ) );
            }
        }

        locationNodeAddrs.add(
                new BasicBSONObject( primaryLocation, primaryLocationAddrs ) );
        locationNodeAddrs.add( new BasicBSONObject( sameCityLocation,
                sameCityLocationAddrs ) );
        locationNodeAddrs.add(
                new BasicBSONObject( offsiteLocation, offsiteLocationAddrs ) );

        int timeOut = 30;
        waitLocationSelectPrimaryNode( db, groupName, primaryLocation,
                timeOut );
        waitLocationSelectPrimaryNode( db, groupName, sameCityLocation,
                timeOut );
        waitLocationSelectPrimaryNode( db, groupName, offsiteLocation,
                timeOut );

        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPS,
                new BasicBSONObject( "GroupName", groupName ), null, null );
        while ( cursor.hasNext() ) {
            System.out
                    .println( "group info -- " + cursor.getNext().toString() );
        }
        cursor.close();

        return locationNodeAddrs;
    }

    /**
     * @description 同城双中心，需要保证group副本数量为7
     * @param db
     * @param groupName
     *            设置的复制组
     * @param primaryLocation
     *            主中心Location
     * @param sameCityLocation
     *            同城备中心Location
     * @return 以["Location":[{"hostName":hostName,"svcName":svcName}]]的形式返回
     */
    public static ArrayList< BasicBSONObject > setTwoLocationInSameCity(
            Sequoiadb db, String groupName, String primaryLocation,
            String sameCityLocation ) {
        ArrayList< BasicBSONObject > locationNodeAddrs = new ArrayList<>();
        ArrayList< BasicBSONObject > primaryLocationAddrs = new ArrayList<>();
        ArrayList< BasicBSONObject > sameCityLocationAddrs = new ArrayList<>();

        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getGroupNodes( db, groupName );
        if ( nodeAddrs.size() != 7 ) {
            Assert.fail( "the numbwe of nodes is not 7, " + nodeAddrs );
        }

        ReplicaGroup rg = db.getReplicaGroup( groupName );
        // 给主节点设置Location
        Node masterNode = rg.getMaster();
        masterNode.setLocation( primaryLocation );
        String masterNodeName = masterNode.getNodeName();
        primaryLocationAddrs
                .add( new BasicBSONObject( "nodeName", masterNodeName ) );

        ArrayList< BasicBSONObject > slaveNodeAddrs = new ArrayList<>();
        slaveNodeAddrs = getGroupSlaveNodes( db, groupName );
        if ( slaveNodeAddrs.size() != 6 ) {
            Assert.fail(
                    "the numbwe of slave nodes is not 6, " + slaveNodeAddrs );
        }
        int slaveNodeNum = 0;
        for ( BasicBSONObject slaveNodeAddr : slaveNodeAddrs ) {
            slaveNodeNum++;
            String hostName = ( String ) slaveNodeAddr.get( "hostName" );
            String svcName = ( String ) slaveNodeAddr.get( "svcName" );
            Node slaveNode = rg.getNode( hostName + ":" + svcName );
            if ( slaveNodeNum < 4 ) {
                // 主中心备节点设置Location
                slaveNode.setLocation( primaryLocation );
                primaryLocationAddrs
                        .add( new BasicBSONObject( "hostName", hostName )
                                .append( "svcName", svcName ) );
            } else if ( slaveNodeNum < 7 ) {
                // 同城备中心设置Location
                slaveNode.setLocation( sameCityLocation );
                sameCityLocationAddrs
                        .add( new BasicBSONObject( "hostName", hostName )
                                .append( "svcName", svcName ) );
            }
        }

        locationNodeAddrs.add(
                new BasicBSONObject( primaryLocation, primaryLocationAddrs ) );
        locationNodeAddrs.add( new BasicBSONObject( sameCityLocation,
                sameCityLocationAddrs ) );

        int timeOut = 30;
        waitLocationSelectPrimaryNode( db, groupName, primaryLocation,
                timeOut );
        waitLocationSelectPrimaryNode( db, groupName, sameCityLocation,
                timeOut );

        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPS,
                new BasicBSONObject( "GroupName", groupName ), null, null );
        while ( cursor.hasNext() ) {
            System.out
                    .println( "group info -- " + cursor.getNext().toString() );
        }
        cursor.close();

        return locationNodeAddrs;
    }

    /**
     * @description 清理复制组中所有节点上的Location
     * @param db
     * @param groupName
     *            指定的复制组
     */
    public static void cleanLocation( Sequoiadb db, String groupName ) {
        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        nodeAddrs = CommLib.getGroupNodes( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        for ( BasicBSONObject nodeAddr : nodeAddrs ) {
            String nodeName = nodeAddr.get( "hostName" ) + ":"
                    + nodeAddr.get( "svcName" );
            Node slaveNode = rg.getNode( nodeName );
            slaveNode.setLocation( "" );
        }
    }

    /**
     * @description: 获取group中指定Location下的所有节点，以[{"hostName":hostName,"svcName":svcName,"nodeID":nodeID}]形式返回
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @param LocationName
     *            需要获取的LocationName名
     * @return
     */
    public static ArrayList< BasicBSONObject > getGroupLocationNodes(
            Sequoiadb db, String groupName, String LocationName ) {
        ArrayList< BasicBSONObject > nodeAddrs = new ArrayList<>();
        ReplicaGroup tmpArray = db.getReplicaGroup( groupName );
        BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
        BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );
        for ( int i = 0; i < groups.size(); ++i ) {
            BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
            if ( group.containsField( "Location" ) ) {
                String actLocationName = ( String ) group.get( "Location" );
                if ( LocationName.equals( actLocationName ) ) {
                    int nodeID = ( int ) group.get( "NodeID" );
                    String hostName = group.getString( "HostName" );
                    BasicBSONList service = ( BasicBSONList ) group
                            .get( "Service" );
                    BasicBSONObject srcInfo = ( BasicBSONObject ) service
                            .get( 0 );
                    String svcName = srcInfo.getString( "Name" );
                    nodeAddrs.add( new BasicBSONObject( "hostName", hostName )
                            .append( "svcName", svcName )
                            .append( "nodeID", nodeID ) );
                }
            }
        }
        return nodeAddrs;
    }

    /**
     * @description: 获取group下的所有备节点，以[{"hostName":hostName,"svcName":svcName,"nodeID":nodeID}]形式返回
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @return
     */
    public static ArrayList< BasicBSONObject > getGroupSlaveNodes( Sequoiadb db,
            String groupName ) {
        ArrayList< BasicBSONObject > nodeAddrs = new ArrayList<>();
        ReplicaGroup tmpArray = db.getReplicaGroup( groupName );
        BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
        BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );
        int masterNodeID = ( int ) doc.get( "PrimaryNode" );
        for ( int i = 0; i < groups.size(); ++i ) {
            BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
            int nodeID = ( int ) group.get( "NodeID" );
            if ( nodeID != masterNodeID ) {
                String hostName = group.getString( "HostName" );
                BasicBSONList service = ( BasicBSONList ) group
                        .get( "Service" );
                BasicBSONObject srcInfo = ( BasicBSONObject ) service.get( 0 );
                String svcName = srcInfo.getString( "Name" );
                nodeAddrs.add( new BasicBSONObject( "hostName", hostName )
                        .append( "svcName", svcName )
                        .append( "nodeID", nodeID ) );
            }
        }
        return nodeAddrs;
    }

    /**
     * @description: 获取group中对应Location下的所有备节点，以[{"hostName":hostName,"svcName":svcName,"nodeID":nodeID}]形式返回
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @param locationName
     *            指定的Location名
     * @return
     */
    public static ArrayList< BasicBSONObject > getGroupLocationSlaveNodes(
            Sequoiadb db, String groupName, String locationName ) {
        waitLocationSelectPrimaryNode( db, groupName, locationName, 30 );

        ArrayList< BasicBSONObject > nodeAddrs = new ArrayList<>();
        ReplicaGroup tmpArray = db.getReplicaGroup( groupName );
        BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();

        if ( !doc.containsField( "Locations" ) ) {
            Assert.fail( "location is not set in group" );
        }
        ArrayList< BasicBSONObject > locations = ( ArrayList< BasicBSONObject > ) doc
                .get( "Locations" );

        int locationPrimaryNode = 0;
        for ( BasicBSONObject location : locations ) {
            String actLocationName = ( String ) location.get( "Location" );
            if ( actLocationName.equals( locationName ) ) {
                locationPrimaryNode = ( int ) location.get( "PrimaryNode" );
            }
        }

        if ( locationPrimaryNode == 0 ) {
            Assert.fail( "group dose not include " + locationName );
        }

        BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );
        for ( int i = 0; i < groups.size(); ++i ) {
            BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
            if ( group.containsField( "Location" ) ) {
                String actLocationName = ( String ) group.get( "Location" );
                if ( actLocationName.equals( locationName ) ) {
                    int nodeID = ( int ) group.get( "NodeID" );
                    if ( nodeID != locationPrimaryNode ) {
                        String hostName = group.getString( "HostName" );
                        BasicBSONList service = ( BasicBSONList ) group
                                .get( "Service" );
                        BasicBSONObject srcInfo = ( BasicBSONObject ) service
                                .get( 0 );
                        String svcName = srcInfo.getString( "Name" );
                        nodeAddrs.add(
                                new BasicBSONObject( "hostName", hostName )
                                        .append( "svcName", svcName )
                                        .append( "nodeID", nodeID ) );
                    }
                }
            }
        }
        return nodeAddrs;
    }

    /**
     * @description: 等待group中对应Location下选出PrimaryNode
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @param locationName
     *            指定的Location名
     * @param timeOut
     *            等待超时时间
     * @return
     */
    public static void waitLocationSelectPrimaryNode( Sequoiadb db,
            String groupName, String locationName, int timeOut ) {
        BasicBSONObject doc = new BasicBSONObject();
        boolean existPrimaryNode = false;
        int doTime = 0;
        while ( doTime < timeOut ) {

            ReplicaGroup tmpArray = db.getReplicaGroup( groupName );
            doc = ( BasicBSONObject ) tmpArray.getDetail();
            if ( !doc.containsField( "Locations" ) ) {
                Assert.fail( "location is not set in group" );
            }
            ArrayList< BasicBSONObject > locations = ( ArrayList< BasicBSONObject > ) doc
                    .get( "Locations" );

            for ( BasicBSONObject location : locations ) {
                String actLocationName = ( String ) location.get( "Location" );
                if ( actLocationName.equals( locationName ) ) {
                    existPrimaryNode = location.containsField( "PrimaryNode" );
                }
            }

            if ( existPrimaryNode ) {
                break;
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                throw new RuntimeException( e );
            }
            doTime++;
        }

        if ( doTime >= timeOut ) {
            Assert.fail( "there is no primary node in location, rg.getDetail : "
                    + doc );
        }
    }

    /**
     * @description: 检测PrimaryNode是否在指定的Location中
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @param locationNodes
     *            指定的Location中的节点
     * @param timeOut
     *            等待超时时间
     * @return
     */
    public static void checkPrimaryNodeInLocation( Sequoiadb db,
            String groupName, ArrayList< BasicBSONObject > locationNodes,
            int timeOut ) {
        Node primaryNode = null;
        boolean primaryNodeExist = false;
        int doTime = 0;
        while ( doTime < timeOut ) {
            try {
                primaryNode = db.getReplicaGroup( groupName ).getMaster();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                        .getErrorCode() ) {
                    throw e;
                } else {
                    doTime++;
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e1 ) {
                        throw new RuntimeException( e1 );
                    }
                    continue;
                }
            }
            int nodeID = primaryNode.getNodeId();

            for ( BasicBSONObject location : locationNodes ) {
                if ( location.getInt( "nodeID" ) == nodeID ) {
                    primaryNodeExist = true;
                }
            }

            if ( primaryNodeExist ) {
                break;
            } else {
                Assert.fail( "primary node is not in location, rg.getDetail : "
                        + db.getReplicaGroup( groupName ).getDetail() );
            }
        }

        if ( doTime >= timeOut ) {
            Assert.fail( "failed to get master node in location " );
        }
    }

    /**
     * @description: 检测PrimaryNode是否在指定的Location中
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @param locationNodes
     *            指定的Location中的节点
     * @param timeOut
     *            等待超时时间
     * @return
     */
    public static boolean isPrimaryNodeInLocation( Sequoiadb db,
            String groupName, ArrayList< BasicBSONObject > locationNodes,
            int timeOut ) {
        Node primaryNode = null;
        boolean primaryNodeExist = false;
        int doTime = 0;
        while ( doTime < timeOut ) {
            try {
                primaryNode = db.getReplicaGroup( groupName ).getMaster();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                        .getErrorCode() ) {
                    throw e;
                } else {
                    doTime++;
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e1 ) {
                        throw new RuntimeException( e1 );
                    }
                    continue;
                }
            }
            int nodeID = primaryNode.getNodeId();

            for ( BasicBSONObject location : locationNodes ) {
                if ( location.getInt( "nodeID" ) == nodeID ) {
                    primaryNodeExist = true;
                }
            }
            break;
        }
        return primaryNodeExist;
    }

    /**
     * @description: 校验指定节点中至少存在一个节点已经同步数据
     * @param csName
     *            集合空间名称
     * @param clName
     *            集合名称
     * @param recordNum
     *            数据量
     * @param nodeAddrs
     *            需要校验的节点，[{"hostName":hostName,"svcName":svcName}]，至少需要包含hostName和svcName
     */
    public static void checkRecordSync( String csName, String clName,
            int recordNum, ArrayList< BasicBSONObject > nodeAddrs ) {
        DBCollection dbcl = null;
        ArrayList< Integer > count = new ArrayList<>();
        for ( BasicBSONObject nodeAddr : nodeAddrs ) {
            String nodeName = nodeAddr.get( "hostName" ) + ":"
                    + nodeAddr.get( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeName, "", "" )) {
                dbcl = data.getCollectionSpace( csName )
                        .getCollection( clName );
                count.add( ( int ) dbcl.getCount() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
                count.add( 0 );
            }
        }

        if ( !count.contains( recordNum ) ) {
            Assert.fail( "expect at least one node to sync, count : " + count
                    + ",nodeAddrs : " + nodeAddrs );
        }
    }

    /**
     * @description: 计算指定节点中有多少个节点已经同步数据
     * @param csName
     *            集合空间名称
     * @param clName
     *            集合名称
     * @param recordNum
     *            数据量
     * @param nodeAddrs
     *            需要校验的节点，[{"hostName":hostName,"svcName":svcName}]，至少需要包含hostName和svcName
     */
    public static int countRecordSyncNum( String csName, String clName,
            int recordNum, ArrayList< BasicBSONObject > nodeAddrs ) {
        int count = 0;
        DBCollection dbcl = null;
        for ( BasicBSONObject nodeAddr : nodeAddrs ) {
            String nodeName = nodeAddr.get( "hostName" ) + ":"
                    + nodeAddr.get( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeName, "", "" )) {
                dbcl = data.getCollectionSpace( csName )
                        .getCollection( clName );
                if ( ( int ) dbcl.getCount() == recordNum ) {
                    count++;
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
        return count;
    }

    public static String getDBPath( Sequoiadb sdb, String nodeName ) {
        String dbPath = "";
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS,
                new BasicBSONObject( "NodeName", nodeName ), null, null );
        while ( cursor.hasNext() ) {
            dbPath = ( String ) cursor.getNext().get( "dbpath" );
        }
        return dbPath;
    }

    /**
     * @description: 检测复制组处于critical模式
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static void checkGroupInCriticalMode( Sequoiadb db,
            String groupName ) {
        ReplicaGroup group = db.getReplicaGroup( groupName );
        BasicBSONObject groupInfo = ( BasicBSONObject ) group.getDetail();
        String mode = groupInfo.getString( "GroupMode" );
        if ( mode == null || !mode.equals( "critical" ) ) {
            Assert.fail(
                    "group " + groupName + " is not in critical mode, detail:"
                            + groupInfo.toString() );
        }
    }

    /**
     * @description: 检测复制组处于maintenance模式
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static void checkGroupInMaintenanceMode( Sequoiadb db,
                                                 String groupName ) {
        ReplicaGroup group = db.getReplicaGroup( groupName );
        BasicBSONObject groupInfo = ( BasicBSONObject ) group.getDetail();
        String mode = groupInfo.getString( "GroupMode" );
        if ( mode == null || !mode.equals( "maintenance" ) ) {
            Assert.fail(
                    "group " + groupName + " is not in maintenance mode, detail:"
                            + groupInfo.toString() );
        }
    }

    /**
     * @description: 检测复制组退出critical模式
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static void checkGroupStopCriticalMode( Sequoiadb db,
            String groupName ) {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "GroupName", groupName );
        query.put( "GroupMode", "critical" );
        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPMODES, query,
                null, null );
        if ( cursor.hasNext() ) {
            Assert.fail( "group " + groupName
                    + " is still in critical mode, SDB_LIST_GROUPMODES : "
                    + cursor.getNext().toString() );
        }
        cursor.close();
    }

    /**
     * @description: 检测复制组nodeID处于critical模式
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static void checkGroupCriticalModeStatus( Sequoiadb db,
            String groupName, int nodeID ) {
        BasicBSONObject option = new BasicBSONObject( "GroupName", groupName );
        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPMODES, option,
                null, null );

        while ( cursor.hasNext() ) {
            BasicBSONObject ret = ( BasicBSONObject ) cursor.getNext();
            BasicBSONList propers = ( BasicBSONList ) ret.get( "Properties" );
            BasicBSONObject proper = ( BasicBSONObject ) propers.get( 0 );
            String groupmode = ret.getString( "GroupMode" );
            int nodeid = proper.getInt( "NodeID" );
            if ( !groupmode.equals( "critical" ) ) {
                Assert.fail(
                        "group " + groupName + " is not in critical mode" );
            }
            if ( nodeid != nodeID ) {
                Assert.fail(
                        "groupID " + nodeid + " is not as exceped :" + nodeID );
            }
        }
    }

    /**
     * @description: 获取复制组处于critical模式的properties信息
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static BasicBSONObject getGroupCriticalInfo( Sequoiadb db,
            String groupName ) {
        BasicBSONObject propertyInfo = null;

        BasicBSONObject option = new BasicBSONObject( "GroupName", groupName );
        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPMODES, option,
                null, null );

        while ( cursor.hasNext() ) {
            BasicBSONObject ret = ( BasicBSONObject ) cursor.getNext();
            BasicBSONList propers = ( BasicBSONList ) ret.get( "Properties" );
            propertyInfo = ( BasicBSONObject ) propers.get( 0 );
            String groupmode = ret.getString( "GroupMode" );
            if ( !groupmode.equals( "critical" ) ) {
                Assert.fail(
                        "group " + groupName + " is not in critical mode" );
            }
        }
        return propertyInfo;
    }

    /**
     * @description: 等待节点启动
     * @param sdb
     *            db连接
     * @param nodeName
     *            节点名
     * @param timeout
     *            超时时间
     */
    public static void waitNodeStart( Sequoiadb sdb, String nodeName,
            int timeout ) {
        int doTime = 0;
        while ( doTime < timeout ) {
            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                    new BasicBSONObject( "NodeName", nodeName )
                            .append( "ShowError", "only" ),
                    null, null );
            if ( cursor.hasNext() ) {
                cursor.close();
                doTime++;
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    throw new RuntimeException( e );
                }
            } else {
                cursor.close();
                break;
            }
        }

        if ( doTime >= timeout ) {
            Assert.fail( "waitNodeStart timeout" );
        }
    }

    /**
     * @description: 异常停止节点
     * @param sdb
     *            db连接
     * @param groupName
     *            复制组名
     * @param Nodes
     *            节点列表
     */
    public static void stopNodeAbnormal( Sequoiadb sdb, String groupName,
            ArrayList< BasicBSONObject > Nodes ) throws ReliabilityException {
        ReplicaGroup group = sdb.getReplicaGroup( groupName );

        // 异常停止节点，然后stop节点模拟故障无法启动
        TaskMgr mgr = new TaskMgr();
        for ( BasicBSONObject curNode : Nodes ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    curNode.getString( "hostName" ),
                    curNode.getString( "svcName" ), 0 );
            mgr.addTask( faultTask );
        }
        mgr.execute();

        for ( BasicBSONObject curNode : Nodes ) {
            String nodeName = curNode.getString( "hostName" ) + ":"
                    + curNode.getString( "svcName" );
            Node node = group.getNode( nodeName );
            node.stop();
        }
    }

    /**
     * @description: 从beginTime开始等待waitTime分钟
     * @param beginTime
     *            开始时间
     * @param waitTime
     *            等待时间，单位为分钟
     */
    public static void validateWaitTime( Date beginTime, int waitTime ) {
        // 获取当前时间
        Date currentTime = new Date();
        // 检查 beginTime 是否大于当前时间
        if ( beginTime.compareTo( currentTime ) > 0 ) {
            Assert.fail( "开始时间大于当前时间" );
        }

        // 计算等待时间的结束时间，将分钟转换为毫秒
        Date endTime = new Date( beginTime.getTime() + waitTime * 60000L );

        // 等待时间循环检查
        while ( true ) {
            currentTime = new Date(); // 更新当前时间

            // 检查当前时间是否超过等待时间
            if ( currentTime.compareTo( endTime ) >= 0 ) {
                return;
            }

            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                throw new RuntimeException( e );
            }
        }
    }

    /**
     * @description: 检测复制组退出critical模式
     * @param db
     *            db连接
     * @param groupName
     *            复制组名
     */
    public static void checkGroupStartCriticalMode( Sequoiadb db,
            String groupName ) {
        BasicBSONObject query = new BasicBSONObject();
        query.put( "GroupName", groupName );
        query.put( "GroupMode", "critical" );
        DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_GROUPMODES, query,
                null, null );
        if ( !cursor.hasNext() ) {
            Assert.fail( "group " + groupName
                    + " is not still in critical mode, SDB_LIST_GROUPMODES : "
                    + cursor.getNext().toString() );
        }
        cursor.close();
    }

    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    public static class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
        }
    }

    public static void ReadLob( DBCollection dbcl,
            LinkedBlockingQueue< SaveOidAndMd5 > id2md5 )
            throws InterruptedException {
        SaveOidAndMd5 oidAndMd5 = id2md5.take();
        ObjectId oid = oidAndMd5.getOid();

        try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
            byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
            rLob.read( rbuff );
            String curMd5 = getMd5( rbuff );
            String prevMd5 = oidAndMd5.getMd5();
            Assert.assertEquals( curMd5, prevMd5 );
        }
        id2md5.offer( oidAndMd5 );
    }

    /**
     * get the buff MD5 value
     *
     * @param inbuff
     *            the object of need to get the MD5
     * @return the MD5 value
     */
    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

    public static LinkedBlockingQueue< SaveOidAndMd5 > writeLobAndGetMd5(
            DBCollection dbcl, int lobtimes ) {
        Random random = new Random();
        LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue<>();
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = getRandomBytes( writeLobSize );
            ObjectId oid = createAndWriteLob( dbcl, wlobBuff );

            // save oid and md5
            String prevMd5 = getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
        return id2md5;
    }

    public static ObjectId createAndWriteLob( DBCollection dbcl, byte[] data ) {
        return createAndWriteLob( dbcl, null, data );
    }

    static ObjectId createAndWriteLob( DBCollection dbcl, ObjectId id,
            byte[] data ) {
        DBLob lob = dbcl.createLob( id );
        lob.write( data );
        lob.close();
        return lob.getID();
    }

    /**
     * get node address
     *
     * @param sdb
     * @param rgName
     * @return nodeAddrs, eg.[host1:11840, host2:11850]
     */
    public static List< String > getNormalNodeAddress( Sequoiadb sdb,
            String rgName ) {
        List< String > nodeAddrs = new ArrayList<>();

        BasicBSONObject matcher = new BasicBSONObject();
        matcher.put( "GroupName", rgName );
        // matcher.put( "ShowError", "ignore" );
        BasicBSONObject selector = new BasicBSONObject();
        selector.put( "NodeName", 1 );
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH, matcher,
                selector, null );
        while ( cursor.hasNext() ) {
            BasicBSONObject node = ( BasicBSONObject ) cursor.getNext();
            String nodeName = ( String ) node.get( "NodeName" );
            if ( nodeName != null ) {
                nodeAddrs.add( nodeName );
            }
        }

        return nodeAddrs;
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
    public static boolean isLSNConsistencyNormalNode( Sequoiadb db,
            String groupName ) {
        boolean isConsistency = false;
        List< String > nodeNames = getNormalNodeAddress( db, groupName );
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        Node masterNode = rg.getMaster();
        String masterNodeName = masterNode.getHostName() + ":"
                + masterNode.getPort();
        try ( Sequoiadb masterSdb = new Sequoiadb( masterNodeName, "", "" )) {
            long completeLSN = -2;
            DBCursor cursor = masterSdb.getSnapshot( Sequoiadb.SDB_SNAP_SYSTEM,
                    null, "{CompleteLSN: ''}", null );
            if ( cursor.hasNext() ) {
                BasicBSONObject snapshot = ( BasicBSONObject ) cursor.getNext();
                if ( snapshot.containsField( "CompleteLSN" ) ) {
                    completeLSN = ( long ) snapshot.get( "CompleteLSN" );
                }
            } else {
                Assert.fail( masterSdb.getNodeName()
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
}
