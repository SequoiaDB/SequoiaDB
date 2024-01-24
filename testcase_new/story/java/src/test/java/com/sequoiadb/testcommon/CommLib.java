package com.sequoiadb.testcommon;

import java.util.*;

import com.sequoiadb.base.*;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.ITestResult;
import org.testng.Reporter;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class CommLib {
    /**
     * Judge the mode
     * 
     * @param sdb
     * @return true/false, true is standalone, false is cluster
     */
    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) { // -159:The operation is for coord
                                              // node only
                // System.out.printf("The mode is standalone.");
                return true;
            }
        }
        return false;
    }

    /**
     * Judge the group number
     * 
     * @param sdb
     * @return true/false, true is only on group, false is multiple group
     */
    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroupNames( sdb ).size() < 2 ) {
            System.out.printf( "Only one group." );
            return true;
        }
        return false;
    }

    /**
     * get dataGroupNames
     * 
     * @param sdb
     * @return dataGroupNames
     */
    public static ArrayList< String > getDataGroupNames( Sequoiadb sdb ) {
        ArrayList< String > dataGroupNames = new ArrayList<>();
        try {
            dataGroupNames = sdb.getReplicaGroupNames();
            dataGroupNames.remove( "SYSCatalogGroup" );
            dataGroupNames.remove( "SYSCoord" );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to get dataGroupsName. ErrorMsg:\n"
                    + e.getMessage() );
        }
        return dataGroupNames;
    }

    /**
     * get node address
     * 
     * @param sdb
     * @param rgName
     * @return nodeAddrs, eg.[host1:11840, host2:11850]
     */
    public static List< String > getNodeAddress( Sequoiadb sdb,
            String rgName ) {
        List< String > nodeAddrs = new ArrayList<>();
        try {
            ReplicaGroup tmpArray = sdb.getReplicaGroup( rgName );
            BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
            BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );

            for ( int i = 0; i < groups.size(); ++i ) {
                BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
                String hostName = group.getString( "HostName" );
                BasicBSONList service = ( BasicBSONList ) group
                        .get( "Service" );
                BasicBSONObject srcInfo = ( BasicBSONObject ) service.get( 0 );
                String svcName = srcInfo.getString( "Name" );
                nodeAddrs.add( hostName + ":" + svcName );
            }
            // System.out.println(rgName + " address: " + nodeAddrs.toString());
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to get groupAdrr. ErrorMsg:\n" + e.getMessage() );
        }
        return nodeAddrs;
    }

    /**
     * get cs info by catalog master
     * 
     * @param sdb
     * @return csInfoOfCata
     */
    public static ArrayList< BSONObject > getCSInfoOfCatalog( Sequoiadb sdb ) {
        ArrayList< BSONObject > csInfoOfCata = new ArrayList<>();
        Sequoiadb cataDB = null;
        try {
            String nodeName = sdb.getReplicaGroup( "SYSCATALOG" ).getMaster()
                    .getNodeName();
            cataDB = new Sequoiadb( nodeName, "", "" );
            DBCollection dmDB = cataDB.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONSPACES" );

            BSONObject sel = new BasicBSONObject();
            BSONObject subSel = new BasicBSONObject();
            subSel.put( "$include", 1 );
            sel.put( "Name", subSel );
            DBCursor cursor = dmDB.query( null, sel, null, null );
            while ( cursor.hasNext() ) {
                BSONObject tmpInfo = cursor.getNext();
                csInfoOfCata.add( tmpInfo );
            }
        } catch ( BaseException e ) {
            Assert.fail( "Failed to get cs info of catalog. ErrorMsg:\n"
                    + e.getMessage() );
        }
        return csInfoOfCata;
    }

    /**
     * check domain results of catalog, compare each node in catalog
     * 
     * @param sdb
     */
    public void checkDomainOfCatalog( Sequoiadb sdb, String domainName ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + domainName );
            matcher.put( "Name", subObj );

            // compare node's data within the group
            CommLib CommLib = new CommLib();
            String rgName = "SYSCatalogGroup";
            String csName = "SYSCAT";
            String clName = "SYSDOMAINS";
            CommLib.compareNodeData( sdb, rgName, csName, clName, matcher );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to check the results of SYSCatalogGroup. ErrorMsg:\n"
                            + e.getMessage() );
        }
    }

    /**
     * check cs results of catalog, compare each node in catalog
     * 
     * @param sdb
     */
    public void checkCSOfCatalog( Sequoiadb sdb, String csName ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName );
            matcher.put( "Name", subObj );

            // compare node's data within the group
            CommLib CommLib = new CommLib();
            String rgName = "SYSCatalogGroup";
            String sysCsName = "SYSCAT";
            String sysClName = "SYSCOLLECTIONSPACES";
            CommLib.compareNodeData( sdb, rgName, sysCsName, sysClName,
                    matcher );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to check the results of SYSCatalogGroup. ErrorMsg:\n"
                            + e.getMessage() );
        }
    }

    /**
     * check cl results of catalog, compare each node in catalog
     * 
     * @param sdb
     * @return true/false, true is success, false is failed
     */
    public void checkCLOfCatalog( Sequoiadb sdb, String csName,
            String clName ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName + "." + clName );
            matcher.put( "Name", subObj );

            // compare node's data within the group
            CommLib CommLib = new CommLib();
            String rgName = "SYSCatalogGroup";
            String sysCSName = "SYSCAT";
            String sysCLName = "SYSCOLLECTIONS";
            CommLib.compareNodeData( sdb, rgName, sysCSName, sysCLName,
                    matcher );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to check the results of SYSCatalogGroup. ErrorMsg:\n"
                            + e.getMessage() );
        }
    }

    /**
     * check cl results of dataRG, compare each node in dataRG
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public void checkCLOfDataRG( Sequoiadb sdb, String csName, String clName ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName + "." + clName );
            matcher.put( "Name", subObj );

            // get all dataGroupNames
            ArrayList< String > dataGroupNames = CommLib
                    .getDataGroupNames( sdb );
            for ( int i = 0; i < dataGroupNames.size(); i++ ) {
                List< String > nodeAddrs = CommLib.getNodeAddress( sdb,
                        dataGroupNames.get( i ) );
                // direct node and compare node's data
                int failCnt = 0;
                int maxCnt = 20;
                boolean checkSucc = false;
                do {
                    ArrayList< String > allNodeData = new ArrayList<>();

                    for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                        Sequoiadb dataDB = new Sequoiadb( nodeAddrs.get( j ),
                                "", "" );
                        DBCursor cursor = dataDB.listCollections();

                        // get the data for each node
                        ArrayList< BSONObject > oneNodeData = new ArrayList<>();
                        while ( cursor.hasNext() ) {
                            BSONObject clList = cursor.getNext();
                            if ( clList.get( "Name" ).toString()
                                    .indexOf( clName ) >= 0 ) {
                                oneNodeData.add( clList );
                            }
                        }
                        cursor.close();
                        dataDB.close();
                        // all node data within the group
                        allNodeData.add( j, oneNodeData.toString() );

                        // compare data between nodes
                        if ( j > 0 ) {
                            if ( allNodeData.get( j )
                                    .equals( allNodeData.get( j - 1 ) ) ) {
                                checkSucc = true;
                                break;
                            } else if ( ++failCnt < maxCnt ) {
                                try {
                                    Thread.sleep( 10 );
                                } catch ( InterruptedException e ) {
                                    e.printStackTrace();
                                }
                                break;
                            }
                            Assert.assertEquals( allNodeData.get( j ),
                                    allNodeData.get( j - 1 ),
                                    "The group is SYSCatalogGroup, "
                                            + nodeAddrs.get( j ) + " and "
                                            + nodeAddrs.get( j - 1 )
                                            + " is not consistent." );
                        }
                        dataDB.closeAllCursors();
                    }
                } while ( !checkSucc && failCnt < maxCnt );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( "Failed to check the results of dataRG. ErrorMsg:\n"
                    + e.getMessage() );
        }
    }

    /**
     * check result for cl
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public void checkCLResult( Sequoiadb sdb, String csName, String clName ) {
        try {
            CommLib.this.checkCLOfCatalog( sdb, csName, clName );
            CommLib.this.checkCLOfDataRG( sdb, csName, clName );

            boolean rc = CommLib.this.compareDataAndCata( sdb, csName, clName );
            Assert.assertTrue( rc );

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * check index between all dataRG node
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public void checkIndex( Sequoiadb sdb, String csName, String clName ) {
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName + "." + clName );
            matcher.put( "Name", subObj );

            DBCursor cursor = sdb.getSnapshot( 8, matcher, null, null );
            while ( cursor.hasNext() ) {
                // get cl info
                BasicBSONObject clInfo = ( BasicBSONObject ) cursor.getNext();
                BasicBSONList cataInfo = ( BasicBSONList ) clInfo
                        .get( "CataInfo" );
                String name = clInfo.getString( "Name" );
                for ( int i = 0; i < cataInfo.size(); ++i ) {
                    BasicBSONObject groupInfo = ( BasicBSONObject ) cataInfo
                            .get( i );
                    String groupName = groupInfo.getString( "GroupName" );
                    // get node address within the group
                    List< String > nodeAddrs = CommLib.getNodeAddress( sdb,
                            groupName );
                    // direct node and compare node's data
                    int failCnt = 0;
                    int maxCnt = 8;
                    boolean checkSucc = false;
                    do {
                        ArrayList< String > allNodeData = new ArrayList<>();
                        for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                            Sequoiadb dataDB = new Sequoiadb(
                                    nodeAddrs.get( j ), "", "" );
                            String tmpCSName = name.split( "\\." )[ 0 ];
                            String tmpCLName = name.split( "\\." )[ 1 ];
                            DBCursor cur = dataDB
                                    .getCollectionSpace( tmpCSName )
                                    .getCollection( tmpCLName ).getIndexes();
                            // get the data for each node
                            ArrayList< BSONObject > oneNodeData = new ArrayList<>();
                            while ( cur.hasNext() ) {
                                BSONObject idxList = cur.getNext();
                                oneNodeData.add( idxList );
                            }
                            cur.close();
                            dataDB.close();
                            // all node data within the group
                            allNodeData.add( j, oneNodeData.toString() );

                            // compare data between nodes
                            if ( j > 0 ) {
                                if ( !allNodeData.get( j )
                                        .equals( allNodeData.get( j - 1 ) ) ) {
                                    checkSucc = true;
                                    break;
                                } else if ( ++failCnt < maxCnt ) {
                                    try {
                                        Thread.sleep( 10 );
                                    } catch ( InterruptedException e ) {
                                        e.printStackTrace();
                                    }
                                    break;
                                }
                                Assert.assertEquals( allNodeData.get( j ),
                                        allNodeData.get( j - 1 ),
                                        "The group is " + groupName + ", "
                                                + nodeAddrs.get( j ) + " and "
                                                + nodeAddrs.get( j - 1 )
                                                + " is not consistent." );
                            }
                            dataDB.closeAllCursors();
                        }
                    } while ( !checkSucc && failCnt < maxCnt );
                }
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 && e.getErrorCode() != -248 ) { // -248:Dropping
                                                                         // the
                                                                         // collection
                                                                         // space
                                                                         // is
                                                                         // in
                                                                         // progress
                Assert.fail(
                        "Failed to check the results of dataRG. ErrorMsg:\n"
                                + e.getMessage() );
            }
        }
    }

    /**
     * compare cl info between dataMaster and cataMaster
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public boolean compareDataAndCata( Sequoiadb sdb, String csName,
            String clName ) {
        try {
            // get all dataGroupNames
            ArrayList< String > dataGroupNames = CommLib
                    .getDataGroupNames( sdb );
            for ( int i = 0; i < dataGroupNames.size(); i++ ) {
                // direct connect data master node, listCollections
                String dataMAddr = sdb
                        .getReplicaGroup( dataGroupNames.get( i ) ).getMaster()
                        .getNodeName();
                Sequoiadb dataDB = new Sequoiadb( dataMAddr, "", "" );
                DBCursor cursor = dataDB.listCollections();
                while ( cursor.hasNext() ) {
                    String tmpCLName = ( String ) cursor.getNext()
                            .get( "Name" );
                    if ( tmpCLName.indexOf( csName + "." + clName ) >= 0 ) {
                        // direct connect cata master node, find the collection
                        String cataAddr = sdb
                                .getReplicaGroup( "SYSCatalogGroup" )
                                .getMaster().getNodeName();
                        Sequoiadb cataDB = new Sequoiadb( cataAddr, "", "" );
                        BSONObject matcher = new BasicBSONObject();
                        matcher.put( "Name", csName + "." + tmpCLName );
                        DBCursor cur = cataDB.getCollectionSpace( "SYSCAT" )
                                .getCollection( "SYSCOLLECTIONS" )
                                .query( matcher, null, null, null );
                        while ( cur.hasNext() ) {
                            String name = ( String ) cur.getNext()
                                    .get( "Name" );
                            if ( name.isEmpty() ) {
                                cur.close();
                                cataDB.close();
                                return false;
                            }
                        }
                        cur.close();
                        cataDB.close();
                    }
                }
                dataDB.close();
            }
        } catch ( BaseException e ) {
            Assert.fail( "Failed to check cl for dataRG. ErrorMsg:\n"
                    + e.getMessage() );
        }

        try {
            // direct connect cata master node, find collections
            String cataAddr = sdb.getReplicaGroup( "SYSCatalogGroup" )
                    .getMaster().getNodeName();
            Sequoiadb cataDB = new Sequoiadb( cataAddr, "", "" );
            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName + "." + clName );
            matcher.put( "Name", subObj );
            DBCursor cursor = cataDB.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" )
                    .query( matcher, null, null, null );
            while ( cursor.hasNext() ) {
                // get clName and groupNames from catalog
                BasicBSONObject clInfo = ( BasicBSONObject ) cursor.getNext();
                String tmpFullCLName = ( String ) clInfo.get( "Name" );
                String tmpCLName = tmpFullCLName.split( csName + "." )[ 1 ];
                BasicBSONList cataInfo = ( BasicBSONList ) clInfo
                        .get( "CataInfo" );
                for ( int i = 0; i < cataInfo.size(); ++i ) {
                    BasicBSONObject groupInfo = ( BasicBSONObject ) cataInfo
                            .get( i );
                    String dataGroupNames = groupInfo.getString( "GroupName" );
                    // direct dataNode, get the collection
                    if ( dataGroupNames != null ) {
                        String dataMAddr = sdb.getReplicaGroup( dataGroupNames )
                                .getMaster().getNodeName();
                        Sequoiadb dataDB = new Sequoiadb( dataMAddr, "", "" );
                        dataDB.getCollectionSpace( csName )
                                .getCollection( tmpCLName );
                        dataDB.close();
                    }
                }
            }
            cursor.close();
            cataDB.close();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -23 ) { // -23:Collection does not exist
                Assert.fail( "Failed to check cl for dataRG. ErrorMsg:\n"
                        + e.getMessage() );
            }
        }

        return true;
    }

    /**
     * compare node's data within the group
     * 
     * @param matcher,
     *            matching condition for query
     */
    public void compareNodeData( Sequoiadb sdb, String rgName, String csName,
            String clName, BSONObject matcher ) {
        Sequoiadb dataDB = null;
        try {
            // get node address within the group
            List< String > nodeAdrrs = CommLib.getNodeAddress( sdb, rgName );

            // direct node and compare node's data
            int failCnt = 0;
            int maxCnt = 15;
            boolean checkSucc = false;
            do {
                ArrayList< String > allNodeData = new ArrayList<>();

                for ( int i = 0; i < nodeAdrrs.size(); ++i ) {
                    dataDB = new Sequoiadb( nodeAdrrs.get( i ), "", "" );
                    DBCollection clDB = dataDB.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCursor cursor = clDB.query( matcher, null, null, null );

                    // get the data for each node
                    ArrayList< BSONObject > oneNodeData = new ArrayList<>();
                    while ( cursor.hasNext() ) {
                        BSONObject csInfo = cursor.getNext();
                        oneNodeData.add( csInfo );
                    }
                    cursor.close();
                    dataDB.close();
                    // all node data within the group
                    allNodeData.add( i, oneNodeData.toString() );

                    // compare data between nodes
                    if ( i > 0 ) {
                        if ( allNodeData.get( i )
                                .equals( allNodeData.get( i - 1 ) ) ) {
                            checkSucc = true;
                            break;
                        } else if ( ++failCnt < maxCnt ) {
                            try {
                                Thread.sleep( 10 );
                            } catch ( InterruptedException e ) {
                                e.printStackTrace();
                            }
                            break;
                        }
                        Assert.assertEquals( allNodeData.get( i ),
                                allNodeData.get( i - 1 ),
                                "The group is SYSCatalogGroup, "
                                        + nodeAdrrs.get( i ) + " and "
                                        + nodeAdrrs.get( i - 1 )
                                        + " is not consistent." );
                    }
                }
            } while ( !checkSucc && failCnt < maxCnt );
        } catch ( BaseException e ) {
            Assert.fail(
                    "Failed to compare data by direct link node. ErroMsg:\n"
                            + e.getMessage() );
        }
    }

    /**
     * clear domain, remove all domains that match domainName
     * 
     * @param sdb
     * @param domainName
     */
    public void clearDomain( Sequoiadb sdb, String domainName ) {
        String dmName = null;
        try {
            BSONObject matcher = new BasicBSONObject();
            BSONObject subMc1 = new BasicBSONObject();
            BSONObject selector = new BasicBSONObject();
            BSONObject subSc1 = new BasicBSONObject();
            // e.g: {Name:{$regex:"^domain10161.",$options:'i'}}
            subMc1.put( "$regex", "^" + domainName );
            subMc1.put( "$options", "i" );
            matcher.put( "Name", subMc1 );
            // e.g: {Name:{$include:1}}
            subSc1.put( "$include", 1 );
            selector.put( "Name", subSc1 );

            DBCursor tmpList = sdb.listDomains( matcher, selector, null, null );
            while ( tmpList.hasNext() ) {
                dmName = ( String ) tmpList.getNext().get( "Name" );
                DBCursor csInDomain = sdb.getDomain( dmName ).listCSInDomain();
                while ( csInDomain.hasNext() ) {
                    String csName = ( String ) csInDomain.getNext()
                            .get( "Name" );
                    sdb.dropCollectionSpace( csName );
                }
                csInDomain.close();
                sdb.dropDomain( dmName );
            }
            tmpList.close();

        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -256 ) { // -256:Domain is not empty
                Assert.fail( "Failed to drop domain in the beginning. "
                        + "ErrorMsg:\n" + e.getMessage() );
            }
        }
    }

    /**
     * clear CS, remove all CS that match csName
     * 
     * @param sdb
     * @param csName
     */
    public static void clearCS( Sequoiadb sdb, String csName ) {
        try {
            DBCursor cursor = sdb.listCollectionSpaces();
            while ( cursor.hasNext() ) {
                String rtCSName = ( String ) cursor.getNext().get( "Name" );
                int num = rtCSName.indexOf( csName );
                if ( num >= 0 ) {
                    sdb.dropCollectionSpace( rtCSName );
                }
            }
            cursor.close();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -34 ) { // -34:Collection space does not
                                             // exist
                Assert.fail( "Failed to drop CS in the beginning. "
                        + "ErrorMsg:\n" + e.getMessage() );
            }
        }
    }

    /**
     * clear CL, remove all CL that match the clName
     * 
     * @param sdb
     * @param clName
     */
    public static void clearCL( Sequoiadb sdb, String csName, String clName ) {
        try {
            DBCursor cursor = sdb.listCollections();
            while ( cursor.hasNext() ) {
                String clFullName = ( String ) cursor.getNext().get( "Name" );
                int num = clFullName.indexOf( clName );
                if ( num >= 0 ) {
                    String name = clFullName.split( csName + "." )[ 1 ];
                    sdb.getCollectionSpace( csName ).dropCollection( name );
                }
            }
            cursor.close();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -23 && e.getErrorCode() != -34 ) {
                // -23:Collection does not exist
                // -34:Collection space does not exist
                Assert.fail( "Failed to drop CL in the beginning. "
                        + "ErrorMsg:\n" + e.getMessage() );
            }
        }
    }

    /**
     * clear group, remove all groups that match rgName
     * 
     * @param sdb
     * @param rgName
     */
    public void clearGroup( Sequoiadb sdb, String rgName ) {
        try {
            ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
            for ( int i = 0; i < groupNames.size(); i++ ) {
                String tmpName = groupNames.get( i );
                if ( tmpName.indexOf( rgName ) >= 0 ) {
                    sdb.removeReplicaGroup( tmpName );
                }
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -154 ) { // -154:Group does not exist
                Assert.fail( e.getMessage() );

            }
        }
    }

    /**
     * createNode
     * 
     * @param sdb
     */
    public void createNode( Sequoiadb sdb, String rgName, int portStart,
            int portStop, String path ) {
        try {
            // create dataRG
            ReplicaGroup rg = sdb.getReplicaGroup( rgName );

            // get hostname
            String hostName = sdb.getReplicaGroup( "SYSCatalogGroup" )
                    .getMaster().getHostName();
            // create node
            BSONObject rgConf = new BasicBSONObject();
            rgConf.put( "logfilesz", 64 );
            int svnName = portStart;
            boolean checkSucc = false;
            do {
                String nodePath = null;
                nodePath = path + "data/" + String.valueOf( svnName );
                try {
                    rg.createNode( hostName, svnName, nodePath, rgConf );
                    checkSucc = true;
                    break;
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() == -157 // -157:Invalid node
                                                  // configuration(Port is
                                                  // occupied)
                            && e.getErrorCode() != -145 ) { // -145:Node already
                                                            // exists
                        svnName = svnName + 10;
                    }
                }
            } while ( !checkSucc && svnName < portStop );
            // rg.start();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -153 // -153:Group already exist;
                    && e.getErrorCode() != -156 ) { // -156: Failed to start the
                                                    // node
                Assert.fail( e.getMessage() );
            }
        }
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
        List< String > groupNames = new ArrayList<>();
        Sequoiadb db = cl.getSequoiadb();
        if ( CommLib.isStandAlone( db ) ) {
            return groupNames;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", cl.getFullName() );
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                groupNames.add( obj.getString( "GroupName" ) );
            }
        }

        // groupNames元素去重
        HashSet< String > uniqueSet = new HashSet<>( groupNames );
        groupNames.clear();
        groupNames.addAll( uniqueSet );

        // groupNames数组元素排序
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

    /**
     * 获取集群的全局事务及mvcc配置
     * 
     * @param db:db连接;role:节点角色，包括coord、catalog、data
     * @return BSONObject 配置项
     * @Author 赵育
     * @Date 2020-03-31
     */
    public static BSONObject getTransConfig( Sequoiadb db, String role ) {
        BSONObject transConfig = db.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS,
                "{role:'" + role + "'}", "{globtranson:'',mvccon:''}", "" )
                .getNext();
        return transConfig;
    }

    /**
     * 获取集群所有coord地址
     * 
     * @param db
     * @return List
     */
    public static List< String > getAllCoordUrls( Sequoiadb db ) {
        List< String > coordUrls = new ArrayList<>();
        DBCursor snapshot = db.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH,
                "{Role: 'coord'}", "{'NodeName': 1}", null );
        while ( snapshot.hasNext() ) {
            coordUrls.add( ( String ) snapshot.getNext().get( "NodeName" ) );
        }
        snapshot.close();
        return coordUrls;
    }

    /**
     * 获取集群中的一个随机coord
     * 
     * @return Sequoiadb
     */
    public static Sequoiadb getRandomSequoiadb() {
        if ( SdbTestBase.coordUrls.isEmpty() ) {
            try ( Sequoiadb sequoiadb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                SdbTestBase.coordUrls = CommLib.getAllCoordUrls( sequoiadb );
            }
        }
        List< String > coordUrls = SdbTestBase.coordUrls;
        String coord = coordUrls
                .get( new Random().nextInt( coordUrls.size() ) );

        // 获取用例名
        ITestResult testClassName = Reporter.getCurrentTestResult();
        System.out.println(
                testClassName.getTestClass().getName() + " coord:" + coord );
        return new Sequoiadb( coord, "", "" );
    }

    /**
     * @description: 获取group下的所有节点，以[{"hostName":hostName,"svcName":svcName,"nodeID":nodeID}]形式返回
     * @param db
     *            db连接
     * @param groupName
     *            需要获取的group名
     * @return
     */
    public static List< BasicBSONObject > getGroupNodes( Sequoiadb db,
            String groupName ) {

        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        try {
            ReplicaGroup tmpArray = db.getReplicaGroup( groupName );
            BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
            BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );

            for ( int i = 0; i < groups.size(); ++i ) {
                BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
                String hostName = group.getString( "HostName" );
                BasicBSONList service = ( BasicBSONList ) group
                        .get( "Service" );
                BasicBSONObject srcInfo = ( BasicBSONObject ) service.get( 0 );
                String svcName = srcInfo.getString( "Name" );
                String nodeID = group.getString( "NodeID" );
                nodeAddrs.add( new BasicBSONObject( "hostName", hostName )
                        .append( "svcName", svcName )
                        .append( "nodeID", nodeID ) );
            }
        } catch ( BaseException e ) {
            throw e;
        }
        return nodeAddrs;
    }

    /**
     * @description: 获取CL所在的所有节点
     * @param db
     *            db连接
     * @param csName
     *            需要获取的CS名
     * @param clName
     *            需要获取的CL名
     * @return
     */
    public static List< BasicBSONObject > getCLNodes( Sequoiadb db,
            String csName, String clName ) {
        List< String > groupName = new ArrayList<>();
        List< BasicBSONObject > nodeAddrs = new ArrayList<>();
        List< BasicBSONObject > nodeInfo = new ArrayList<>();
        DBCollection dbcl = db.getCollectionSpace( csName )
                .getCollection( clName );
        groupName = CommLib.getCLGroups( dbcl );
        for ( int i = 0; i < groupName.size(); i++ ) {
            nodeInfo = getGroupNodes( db, groupName.get( i ) );
            for ( int j = 0; j < nodeInfo.size(); j++ ) {
                nodeAddrs.add( nodeInfo.get( j ) );
            }
        }
        return nodeAddrs;
    }

    /**
     * @description: 循环获取CL,超过60s未获取到报超时
     * @param db
     *            需要获取CL的db连接
     * @param csName
     *            对应的CS名
     * @param clName
     *            需要获取的CL名
     * @return
     */
    public static DBCollection getCL( Sequoiadb db, String csName,
            String clName ) {
        int doTime = 0;
        int timeOut = 60;
        DBCollection dbcl = null;
        while ( doTime < timeOut ) {
            try {
                dbcl = db.getCollectionSpace( csName ).getCollection( clName );
                break;
            } catch ( BaseException e ) {
                if ( e.getErrorType() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorType()
                        && e.getErrorType() != SDBError.SDB_DMS_CS_NOTEXIST
                                .getErrorType() ) {
                    throw e;
                }
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
            doTime++;
        }
        if ( doTime >= timeOut ) {
            Assert.fail( "get collection time out" );
        }
        return dbcl;
    }

    /**
     * @description: 获取group中的节点数量
     * @param db
     *            需要获取group的db连接
     * @param groupName
     *            对应的group名
     * @return
     */
    public static int getNodeNum( Sequoiadb db, String groupName ) {
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        BSONObject object = rg.getDetail();
        ArrayList rgInfo = ( ArrayList ) object.get( "Group" );
        return rgInfo.size();
    }

    /**
     * @description: 获取所有节点privilegecheck的值求与
     * @param sdb
     *            db连接
     * @return
     */
    public static boolean getPrivilegecheck( Sequoiadb sdb ) {
        boolean privilegecheck = true;
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                new BasicBSONObject( "privilegecheck", 1 ), null );
        while ( cursor.hasNext() ) {
            BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext();
            boolean nodePrivilegecheck = Boolean
                    .parseBoolean( ( String ) obj.get( "privilegecheck" ) );
            privilegecheck = privilegecheck && nodePrivilegecheck;
        }
        cursor.close();
        return privilegecheck;
    }

    /**
     * @description: 集群设置privilegecheck，会重启节点，只能修改SdbTestBase.coordUrl不支持传入sdb
     * @param privilegecheck
     *            需要设置的privilegecheck值
     * @return
     */
    public static void setPrivilegecheck( boolean privilegecheck )
            throws Exception {
        Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        try {
            sdb.updateConfig(
                    new BasicBSONObject( "privilegecheck", privilegecheck ) );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_RTN_CONF_NOT_TAKE_EFFECT
                    .getErrorCode()
                    && e.getErrorCode() != SDBError.SDB_COORD_NOT_ALL_DONE
                            .getErrorCode() ) {
                e.printStackTrace();
            }
        }

        List< String > coordUrls = CommLib.getAllCoordUrls( sdb );
        ArrayList< String > groupNames = sdb.getReplicaGroupNames();
        groupNames.remove( "SYSCoord" );
        groupNames.remove( "SYSCatalogGroup" );

        if ( coordUrls.size() == 1 ) {
            System.out.println( "only one coord" );
            ArrayList< String > hostNames = CommLib.getHostNames( sdb );
            for ( String hostName : hostNames ) {
                Ssh ssh = new Ssh( hostName, "root", SdbTestBase.rootPwd );
                try {
                    ssh.exec( "cat /etc/default/sequoiadb |grep INSTALL_DIR" );
                    String str = ssh.getStdout();
                    if ( str.length() <= 0 ) {
                        throw new Exception(
                                "exec command:cat /etc/default/sequoiadb |grep INSTALL_DIR can not find sequoiadb install dir" );
                    }
                    String installPath = str.substring( str.indexOf( "=" ) + 1,
                            str.length() - 1 );
                    String cmdStopNode = installPath + "/bin/sdbstop -t db";
                    ssh.exec( cmdStopNode );
                    String cmdStartNode = installPath + "/bin/sdbstart";
                    ssh.exec( cmdStartNode );
                } finally {
                    ssh.disconnect();
                }
            }
        } else {
            System.out.println( "more than one coord" );

            // 先重启catalog，然后等待catalog选出主节点
            ReplicaGroup catalogRG = sdb.getReplicaGroup( "SYSCatalogGroup" );
            catalogRG.stop();
            catalogRG.start();
            waitGroupSelectMasterNode( sdb, "SYSCatalogGroup", 300 );
            CommLib.isLSNConsistency( sdb, "SYSCatalogGroup" );

            for ( String groupName : groupNames ) {
                ReplicaGroup replicaGroup = sdb.getReplicaGroup( groupName );
                replicaGroup.stop();
                replicaGroup.start();
            }

            // 连接第一个coord重启后面coord
            Sequoiadb sdb1 = new Sequoiadb( coordUrls.get( 0 ), "", "" );
            ReplicaGroup coordRG = sdb1.getReplicaGroup( "SYSCoord" );
            for ( int i = 1; i < coordUrls.size(); i++ ) {
                coordRG.getNode( coordUrls.get( i ) ).stop();
                coordRG.getNode( coordUrls.get( i ) ).start();
            }
            sdb1.close();

            // 连接第二个coord重启第一个coord
            Sequoiadb sdb2 = new Sequoiadb( coordUrls.get( 1 ), "", "" );
            coordRG = sdb2.getReplicaGroup( "SYSCoord" );
            coordRG.getNode( coordUrls.get( 0 ) ).stop();
            coordRG.getNode( coordUrls.get( 0 ) ).start();
            sdb2.close();
        }

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        waitGroupSelectMasterNode( sdb, groupNames, 300 );
        for ( String groupName : groupNames ) {
            CommLib.isLSNConsistency( sdb, groupName );
        }

        sdb.close();
    }

    /**
     * @description: 获取集群所在所有机器的主机名
     * @param db
     *            需要获取集群的db连接
     */
    public static ArrayList< String > getHostNames( Sequoiadb db ) {
        ArrayList< String > hostNames = new ArrayList();
        String hostName = "";
        BasicBSONObject matcher = new BasicBSONObject( "RawData", true );
        BasicBSONObject selector = new BasicBSONObject( "HostName", 1 );
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE, matcher,
                selector, null );
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            hostName = ( String ) obj.get( "HostName" );
            // 已存在的主机名不重复加入
            if ( !hostNames.contains( hostName ) ) {
                hostNames.add( hostName );
            }
        }
        cursor.close();
        return hostNames;
    }

    /**
     * @description: 在指定group中创建一定数量的节点
     * @param db
     *            需要获取group的db连接
     * @param groupName
     *            对应的group名
     * @param expNodeNum
     *            期望最终的节点数量
     * @return
     */
    public static ArrayList< BasicBSONObject > createNode( Sequoiadb db,
            String groupName, int expNodeNum ) {
        ArrayList< BasicBSONObject > nodeInfos = new ArrayList();
        int actNodeNum = getNodeNum( db, groupName );
        // 期望节点数量小于实际节点数量时直接报错
        if ( expNodeNum < actNodeNum ) {
            Assert.fail(
                    "expected number of nodes is less than actual number of nodes, act:"
                            + actNodeNum + ", exp:" + expNodeNum );
        } else if ( expNodeNum == actNodeNum ) {
            // 期望节点数量等于实际节点数量时直接返回
            return nodeInfos;
        }
        ReplicaGroup rg = db.getReplicaGroup( groupName );

        ArrayList< String > hostNames = getHostNames( db );
        Random random = new Random();
        BasicBSONObject configure = new BasicBSONObject( "diaglevel", 5 );
        // 创建节点，并保存创建节点的主机名和端口号
        for ( int i = 0; i < expNodeNum - actNodeNum; i++ ) {
            int randomIndex = random.nextInt( hostNames.size() );
            int port = SdbTestBase.reservedPortBegin + i * 10;
            String dbPath = SdbTestBase.reservedDir + port + "/";
            String hostName = hostNames.get( randomIndex );
            rg.createNode( hostName, port, dbPath, configure );
            System.out.println( hostName + port + dbPath + configure );

            BasicBSONObject nodeInfo = new BasicBSONObject();
            nodeInfo.put( "hostName", hostName );
            nodeInfo.put( "port", port );
            nodeInfos.add( nodeInfo );
        }
        rg.start();
        return nodeInfos;
    }

    /**
     * @description: 移除group中的指定节点，并确保移除后的主节点位置
     * @param db
     *            需要获取group的db连接
     * @param groupName
     *            对应的group名
     * @param expMasterNodeID
     *            期望的主节点ID
     * @param nodeInfos
     *            期望移除的节点
     * @return
     */
    public static void removeNode( Sequoiadb db, String groupName,
            Integer expMasterNodeID, ArrayList< BasicBSONObject > nodeInfos ) {
        System.out.println( "nodeInfos -- " + nodeInfos.toString() );
        if ( nodeInfos.size() == 0 ) {
            return;
        }
        ReplicaGroup rg = db.getReplicaGroup( groupName );

        Integer actMasterNodeID = null;
        int doTime = 0;
        int timeOut = 180;
        actMasterNodeID = rg.getMaster().getNodeId();

        System.out.println( "doTime > timeOut -- " + ( doTime > timeOut ) );
        System.out.println(
                "Objects.equals( actMasterNodeID, expMasterNodeID ) -- "
                        + actMasterNodeID.equals( expMasterNodeID ) );

        // 保证主节点和测试前一致
        while ( doTime > timeOut
                || !actMasterNodeID.equals( expMasterNodeID ) ) {
            rg.reelect( new BasicBSONObject( "NodeID", expMasterNodeID ) );
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                throw new RuntimeException( e );
            }
            doTime++;
            actMasterNodeID = rg.getMaster().getNodeId();
        }

        if ( doTime >= timeOut ) {
            Assert.fail(
                    "failed to select the master within the expected time" );
        }

        for ( BasicBSONObject nodeInfo : nodeInfos ) {
            String hostname = nodeInfo.getString( "hostName" );
            int port = ( int ) nodeInfo.get( "port" );
            rg.removeNode( hostname, port, null );
        }
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
    public static boolean isLSNConsistency( Sequoiadb db, String groupName ) {
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

    /**
     * @description: 等待group中选出主节点
     * @param db
     *            需要获取group的db连接
     * @param groupName
     *            对应的group名
     * @param timeOut
     *            等待超时时间
     * @return
     */
    public static void waitGroupSelectPrimaryNode( Sequoiadb db,
            String groupName, int timeOut ) {
        int doTime = 0;
        while ( doTime < timeOut ) {
            ReplicaGroup rg = db.getReplicaGroup( groupName );
            try {
                rg.getMaster();
                break;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                        .getErrorCode() ) {
                    throw e;
                }
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                throw new RuntimeException( e );
            }
            doTime++;
        }

        if ( doTime >= timeOut ) {
            Assert.fail(
                    "there is no primary node in group, group : " + groupName );
        }
    }

    /**
     * @description: 清理复制组下所有集合空间
     * @param db
     *            需要获取group的db连接
     * @param groupName
     *            对应的group名
     * @return
     */
    public static void cleanUpCSInGroup( Sequoiadb db, String groupName ) {
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                new BasicBSONObject( "GroupName", groupName ), null, null );
        while ( cursor.hasNext() ) {
            String csName = ( String ) cursor.getNext().get( "Name" );
            db.dropCollectionSpace( csName );
        }
        cursor.close();
    }

    /**
     * @description: 备份节点诊断日志，日志已nodeName/diaglog的形式存放在backupPath路径下
     * @param db
     *            指定一个可用的db连接
     * @param matcher
     *            匹配需要备份的节点
     * @param user
     *            远程连接用户名
     * @param passwd
     *            远程连接用户对应密码
     * @param backupPath
     *            备份日志存放路径
     * @return
     */
    public static void copyNodeLogs( Sequoiadb db, BasicBSONObject matcher,
            String user, String passwd, String backupPath ) throws Exception {
        Ssh ssh = null;
        // 清理备份目录
        ArrayList< String > hostNames = getHostNames( db );
        for ( String hostName : hostNames ) {
            try {
                ssh = new Ssh( hostName, user, passwd );
                String cleanPath = "rm -rf " + backupPath;
                ssh.exec( cleanPath );
            } finally {
                if ( ssh != null ) {
                    ssh.disconnect();
                }
            }
        }

        // 备份日志
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, matcher,
                null, null );
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            try {
                String nodeName = ( String ) obj.get( "NodeName" );
                String[] parts = nodeName.split( ":" );
                System.out.println( "parts[ 0 ] -- " + parts[ 0 ] );
                ssh = new Ssh( parts[ 0 ], user, passwd );
                String backupPathFull = backupPath + "/" + nodeName;
                // 创建备份目录
                String createFolderCmd = "mkdir -p " + backupPathFull;
                System.out.println( "createFolderCmd -- " + createFolderCmd );
                ssh.exec( createFolderCmd );
                // 备份日志
                String diagpath = ( String ) obj.get( "diagpath" );
                String copyCmd = "cp -r " + diagpath + " " + backupPathFull;
                System.out.println( "copyCmd -- " + copyCmd );
                System.out.println( "copyCmd -- " + copyCmd );
                ssh.exec( copyCmd );
            } finally {
                if ( ssh != null ) {
                    ssh.disconnect();
                }
            }
        }
        cursor.close();
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
                obj.put( "a", value );
                obj.put( "teststr", "teststr" + value );
                batchRecords.add( obj );
            }
            dbcl.bulkInsert( batchRecords );
            insertRecord.addAll( batchRecords );
            batchRecords.clear();
        }
        return insertRecord;
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, BasicBSONObject orderBy ) {
        DBCursor cursor = dbcl.query( null, null, orderBy, null );

        int count = 0;
        while ( cursor.hasNext() ) {

            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            if ( !expRecord.equals( record ) ) {
                Assert.fail( "record: " + record.toString() + "\nexp: "
                        + expRecord.toString() );
            }
            Assert.assertEquals( record, expRecord );
        }
        if ( count != expRecords.size() ) {
            Assert.fail(
                    "actNum: " + count + "\nexpNum: " + expRecords.size() );
        }
    }

    public static ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum ) {
        return insertData( dbcl, recordNum, 5 );
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

    /**
     * @description: 等待group中选出PrimaryNode
     * @param db
     *            db连接
     * @param groupNames
     *            需要获取的groups名
     * @param timeOut
     *            等待超时时间
     */
    public static void waitGroupSelectMasterNode( Sequoiadb db,
            ArrayList< String > groupNames, int timeOut ) {
        int doTime = 0;
        for ( String groupName : groupNames ) {
            while ( doTime < timeOut ) {
                try {
                    ReplicaGroup replicaGroup = db.getReplicaGroup( groupName );
                    replicaGroup.getMaster();
                    break;
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_RTN_NO_PRIMARY_FOUND
                            .getErrorCode()
                            && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                    .getErrorCode() ) {
                        throw e;
                    }
                }

                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }

                doTime++;
            }
        }

        if ( doTime >= timeOut ) {
            Assert.fail( "there is no primary node in group " );
        }
    }

    public static void waitGroupSelectMasterNode( Sequoiadb db,
            String groupName, int timeOut ) {
        ArrayList< String > groupNames = new ArrayList<>();
        groupNames.add( groupName );
        waitGroupSelectMasterNode( db, groupNames, timeOut );
    }
}
