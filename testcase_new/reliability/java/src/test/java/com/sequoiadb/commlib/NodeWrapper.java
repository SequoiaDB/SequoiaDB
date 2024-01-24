/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:NodeWrapper.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.commlib;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;

public class NodeWrapper {
    public enum NodeStatus {
        STOP_SUCCESS, STOP_FAILURE, START_SUCCESS, START_FAILURE
    };

    private NodeStatus status;
    private Node node;
    private BasicBSONObject nodeInfo;

    @Override
    public String toString() {
        return "NodeWrapper{" + "status=" + status + ", node=" + node
                + ", nodeInfo=" + nodeInfo + '}';
    }

    public NodeWrapper( Node node, BasicBSONObject nodeInfo ) {
        this.node = node;
        this.nodeInfo = nodeInfo;
    }

    private BasicBSONObject getDataBaseSnapshot( boolean printRes )
            throws ReliabilityException {
        Sequoiadb sdb = null;
        BasicBSONObject retObj = null;
        try {
            // System.out.println( "csq hostname : " + node.getHostName());
            // System.out.println( "csq port : " + node.getPort());
            // Sequoiadb csqdb = new
            // Sequoiadb(node.getHostName(),node.getPort(),"","");
            // System.out.println( "csq isValid : " + csqdb.isValid());
            // Sequoiadb coord = new Sequoiadb("192.168.28.107",11810,"","");
            // System.out.println( "csq before connect snapshot databse : ");
            // DBCursor csqcur = coord.getSnapshot(Sequoiadb.SDB_SNAP_DATABASE,
            // new BasicBSONObject(), null, null);
            // while(csqcur.hasNext()){
            // System.out.println(csqcur.getNext().toString());
            // }
            // csqcur.close();
            sdb = node.connect();
            // System.out.println( "csq after connect snapshot databse : ");
            // csqcur = coord.getSnapshot(Sequoiadb.SDB_SNAP_DATABASE, new
            // BasicBSONObject(), null, null);
            // while(csqcur.hasNext()){
            // System.out.println(csqcur.getNext().toString());
            // }
            // csqcur.close();
            BSONObject nullObj = null;
            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                    nullObj, nullObj, nullObj );
            while ( cursor.hasNext() ) {
                retObj = ( BasicBSONObject ) cursor.getNext();
            }
            cursor.close();
        } catch ( BaseException e ) {
            if ( printRes ) {
                System.out.println( node.getNodeName() + " getSnapshot( "
                        + Sequoiadb.SDB_SNAP_DATABASE + ") failed "
                        + e.getErrorCode() );
            }
            throw new ReliabilityException( e );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
        return retObj;
    }

    public boolean start() throws ReliabilityException {
        try {
            node.start();
            status = NodeStatus.START_SUCCESS;

        } catch ( BaseException e ) {
            System.out.println( "start " + node.getNodeName() + " failed "
                    + e.getErrorCode() );
            status = NodeStatus.START_FAILURE;
            throw new ReliabilityException( e );
        }
        return true;
    }

    public boolean stop() throws ReliabilityException {
        try {
            node.stop();
            status = NodeStatus.STOP_SUCCESS;

        } catch ( BaseException e ) {
            // -140 停止节点超时，但节点最终仍然停止成功，捕获此错误码规避该错误。
            if ( e.getErrorCode() != -140 ) {
                System.out.println( "stop " + node.getNodeName() + " failed "
                        + e.getErrorCode() );
                status = NodeStatus.STOP_FAILURE;
                e.printStackTrace();
                throw new ReliabilityException( e );
            }
        }
        return true;
    }

    public boolean checkStop() {
        if ( status == NodeStatus.STOP_FAILURE ) {
            return false;
        } else {
            return true;
        }
    }

    public boolean checkStart() {
        if ( status == NodeStatus.START_FAILURE ) {
            return false;
        } else {
            return true;
        }
    }

    public boolean isNodeActive() {
        try {
            Sequoiadb db = node.connect();
            BSONObject nullObj = null;
            DBCursor cursor = db.getList( Sequoiadb.SDB_LIST_CONTEXTS_CURRENT,
                    nullObj, nullObj, nullObj );
            while ( cursor.hasNext() ) {
                cursor.getNext();
            }
            cursor.close();
        } catch ( BaseException e ) {
            return false;
        }
        return true;
    }

    public String hostName() {
        final String HostName = "HostName";
        if ( nodeInfo.containsField( HostName ) ) {
            return nodeInfo.getString( HostName );
        } else {
            return "";
        }
    }

    public int nodeID() {
        final String NodeID = "NodeID";
        if ( nodeInfo.containsField( NodeID ) ) {
            return nodeInfo.getInt( NodeID );
        } else {
            return -1;
        }
    }

    public String svcName() {
        return ( ( BasicBSONObject ) ( ( BasicBSONList ) nodeInfo
                .get( "Service" ) ).get( 0 ) ).getString( "Name" );
    }

    public String dbPath() {
        return nodeInfo.getString( "dbpath" );
    }

    public boolean isMaster() throws ReliabilityException {
        return isMaster( true );
    }

    public boolean isMaster( boolean printException )
            throws ReliabilityException {
        BasicBSONObject obj = getDataBaseSnapshot( printException );
        if ( obj != null ) {
            return obj.getBoolean( "IsPrimary" );
        }

        return false;
    }

    public NodeCheckResult checkBusiness( boolean printRes ) {
        NodeCheckResult checkResult = new NodeCheckResult();
        checkResult.hostName = hostName();
        checkResult.nodeID = nodeID();
        checkResult.svcName = svcName();

        int svcPort = Integer.parseInt( checkResult.svcName );
        if ( svcPort >= SdbTestBase.reservedPortBegin
                && svcPort <= SdbTestBase.reservedPortEnd ) {
            checkResult.isInDeploy = false;
        }

        try {
            BasicBSONObject obj = getDataBaseSnapshot( printRes );
            // System.out.println(node.toString() + "snapshot(6)" +
            // obj.toString()) ;
            checkResult.serviceStatus = obj.getBoolean( "ServiceStatus" );
            checkResult.connect = true;
            if ( obj.containsField( "CurrentLSN" ) ) {
                checkResult.LSNVer = ( ( BasicBSONObject ) obj
                        .get( "CurrentLSN" ) ).getInt( "Version" );
            }

            if ( obj.containsField( "CompleteLSN" ) ) {
                checkResult.LSN = obj.getLong( "CompleteLSN" );
            }

            if ( obj.containsField( "IsPrimary" ) ) {
                checkResult.isPrimary = obj.getBoolean( "IsPrimary" );
            }
            if ( obj.containsField( "Disk" ) ) {
                checkResult.freeSpace = ( ( BasicBSONObject ) obj
                        .get( "Disk" ) ).getLong( "FreeSpace" );
            }
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            checkResult.connect = false;
        }

        return checkResult;
    }

    public void backupDiaglog( String testCaseName )
            throws ReliabilityException {

        Ssh remote = new Ssh( hostName(), SdbTestBase.remoteUser,
                SdbTestBase.remotePwd );
        remote.exec( String.format( "cp -r %s/diaglog %s/backup_%s", dbPath(),
                SdbTestBase.workDir, testCaseName ) );
        if ( 0 != remote.getExitStatus() ) {
            throw new ReliabilityException( "stdout:" + remote.getStdout()
                    + "\nstderr:" + remote.getStderr() );
        }
    }

    public Sequoiadb connect() {
        Sequoiadb db = new Sequoiadb( hostName() + ":" + svcName(), "", "" );
        return db;
    }
}
