package com.sequoiadb.location;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.*;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import java.util.ArrayList;
import java.util.List;

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
            System.out.println( "location -- " + location );
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
                System.out.println( "location -- " + location );
                String actLocationName = ( String ) location.get( "Location" );
                if ( actLocationName.equals( locationName ) ) {
                    existPrimaryNode = location.containsField( "PrimaryNode" );
                }
            }

            if ( existPrimaryNode ) {
                break;
            }
        }

        if ( doTime >= timeOut ) {
            Assert.fail( "there is no primary node in location, rg.getDetail : "
                    + doc );
        }

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
        System.out.println( "expect at least one node to sync, count : " + count
                + ",nodeAddrs : " + nodeAddrs );
        if ( !count.contains( recordNum ) ) {
            Assert.fail( "expect at least one node to sync, count : " + count
                    + ",nodeAddrs : " + nodeAddrs );
        }
    }
}
