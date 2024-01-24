package com.sequoiadb.testcommon;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

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
        ArrayList< String > dataGroupNames = new ArrayList< String >();
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
    public List< String > getNodeAddress( Sequoiadb sdb, String rgName ) {
        List< String > nodeAddrs = new ArrayList< String >();
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
    public ArrayList< BSONObject > getCSInfoOfCatalog( Sequoiadb sdb ) {
        ArrayList< BSONObject > csInfoOfCata = new ArrayList< BSONObject >();
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
                BSONObject tmpInfo = ( BSONObject ) cursor.getNext();
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
     * @return true/false, true is success, false is failed
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
     * @return true/false, true is success, false is failed
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
            CommLib CommLib = new CommLib();
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
                    ArrayList< String > allNodeData = new ArrayList< String >();

                    for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                        Sequoiadb dataDB = new Sequoiadb( nodeAddrs.get( j ),
                                "", "" );
                        DBCursor cursor = dataDB.listCollections();

                        // get the data for each node
                        ArrayList< BSONObject > oneNodeData = new ArrayList< BSONObject >();
                        while ( cursor.hasNext() ) {
                            BSONObject clList = ( BSONObject ) cursor.getNext();
                            if ( clList.get( "Name" ).toString()
                                    .indexOf( clName ) >= 0 ) {
                                oneNodeData.add( clList );
                            }
                        }
                        cursor.close();
                        dataDB.disconnect();
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
                    CommLib CommLib = new CommLib();
                    List< String > nodeAddrs = CommLib.getNodeAddress( sdb,
                            groupName );
                    // direct node and compare node's data
                    int failCnt = 0;
                    int maxCnt = 8;
                    boolean checkSucc = false;
                    do {
                        ArrayList< String > allNodeData = new ArrayList< String >();
                        for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                            Sequoiadb dataDB = new Sequoiadb(
                                    nodeAddrs.get( j ), "", "" );
                            String tmpCSName = name.split( "\\." )[ 0 ];
                            String tmpCLName = name.split( "\\." )[ 1 ];
                            DBCursor cur = dataDB
                                    .getCollectionSpace( tmpCSName )
                                    .getCollection( tmpCLName ).getIndexes();
                            // get the data for each node
                            ArrayList< BSONObject > oneNodeData = new ArrayList< BSONObject >();
                            while ( cur.hasNext() ) {
                                BSONObject idxList = ( BSONObject ) cur
                                        .getNext();
                                oneNodeData.add( idxList );
                            }
                            cur.close();
                            dataDB.disconnect();
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
            CommLib CommLib = new CommLib();
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
                                return false;
                            }
                        }
                    }
                }
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
                    }
                }
            }
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
            CommLib CommLib = new CommLib();
            List< String > nodeAdrrs = CommLib.getNodeAddress( sdb, rgName );

            // direct node and compare node's data
            int failCnt = 0;
            int maxCnt = 15;
            boolean checkSucc = false;
            do {
                ArrayList< String > allNodeData = new ArrayList< String >();

                for ( int i = 0; i < nodeAdrrs.size(); ++i ) {
                    dataDB = new Sequoiadb( nodeAdrrs.get( i ), "", "" );
                    DBCollection clDB = dataDB.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCursor cursor = clDB.query( matcher, null, null, null );

                    // get the data for each node
                    ArrayList< BSONObject > oneNodeData = new ArrayList< BSONObject >();
                    while ( cursor.hasNext() ) {
                        BSONObject csInfo = ( BSONObject ) cursor.getNext();
                        oneNodeData.add( csInfo );
                    }
                    cursor.close();
                    dataDB.disconnect();
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
    public void clearCS( Sequoiadb sdb, String csName ) {
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
    public void clearCL( Sequoiadb sdb, String csName, String clName ) {
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
            ArrayList< String > groupNames = CommLib.this
                    .getDataGroupNames( sdb );
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

    public static void checkRecordsResult(DBCursor queryCursor, List< BSONObject > expRecords ){
        ArrayList< BSONObject > actRecords = new ArrayList< BSONObject >();
        while ( queryCursor.hasNext() ) {
            BSONObject queryRecord = queryCursor.getNext();
           // queryRecord.removeField("_id");
            actRecords.add( queryRecord );
        }
        queryCursor.close();
        Assert.assertEquals( actRecords, expRecords);
    }

}
