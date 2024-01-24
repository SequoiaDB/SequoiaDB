package com.sequoiadb.metadataconsistency.data;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class MetaDataUtils extends SdbTestBase {

    /**
     * get dataGroupNames
     * 
     * @param sdb
     * @return dataGroupNames
     */
    public static ArrayList< String > getDataGroupNames( Sequoiadb sdb ) {
        ArrayList< String > dataGroupNames = new ArrayList< String >();
        dataGroupNames = sdb.getReplicaGroupNames();
        dataGroupNames.remove( "SYSCatalogGroup" );
        dataGroupNames.remove( "SYSCoord" );
        dataGroupNames.remove( "SYSSpare" );
        return dataGroupNames;
    }

    /**
     * Judge the number of nodes in the SYSCatalogGroup
     * 
     * @param sdb
     * @return true/false
     */
    public static boolean oneCataNode( Sequoiadb sdb ) {
        int nodesNum = nodesNum( sdb, "SYSCatalogGroup" );
        if ( nodesNum < 2 ) {
            System.out.printf( "Only one node in the SYSCatalogGroup." );
            return true;
        }
        return false;
    }

    /**
     * Judge the number of nodes int the dataGroup
     * 
     * @param sdb
     * @return true/false
     */
    public static boolean oneDataNode( Sequoiadb sdb ) {
        ArrayList< String > dataGroupNames = getDataGroupNames( sdb );
        for ( String rgName : dataGroupNames ) {
            int nodesNum = nodesNum( sdb, rgName );
            if ( nodesNum < 2 ) {
                System.out.printf( "Only one node in the " + rgName + "." );
                return true;
            }
        }
        return false;
    }

    private static int nodesNum( Sequoiadb sdb, String rgName ) {
        ReplicaGroup tmpArray = sdb.getReplicaGroup( rgName );
        BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
        BasicBSONList nodes = ( BasicBSONList ) doc.get( "Group" );
        return nodes.size();
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
        List< String > nodeAddrs = new ArrayList< String >();
        ReplicaGroup tmpArray = sdb.getReplicaGroup( rgName );
        BasicBSONObject doc = ( BasicBSONObject ) tmpArray.getDetail();
        BasicBSONList groups = ( BasicBSONList ) doc.get( "Group" );

        for ( int i = 0; i < groups.size(); ++i ) {
            BasicBSONObject group = ( BasicBSONObject ) groups.get( i );
            String hostName = group.getString( "HostName" );
            BasicBSONList service = ( BasicBSONList ) group.get( "Service" );
            BasicBSONObject srcInfo = ( BasicBSONObject ) service.get( 0 );
            String svcName = srcInfo.getString( "Name" );
            nodeAddrs.add( hostName + ":" + svcName );
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
        ArrayList< BSONObject > csInfoOfCata = new ArrayList< BSONObject >();
        Sequoiadb cataDB = null;
        try {
            cataDB = sdb.getReplicaGroup( "SYSCATALOG" ).getMaster().connect();
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
        } finally {
            cataDB.close();
        }
        return csInfoOfCata;
    }

    /**
     * check domain results of catalog, compare each node in catalog
     * 
     * @param sdb
     * @return true/false, true is success, false is failed
     */
    public static void checkDomainOfCatalog( String domainName ) {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + domainName );
            matcher.put( "Name", subObj );

            // compare node's data within the group
            String rgName = "SYSCatalogGroup";
            String csName = "SYSCAT";
            String clName = "SYSDOMAINS";
            compareNodeData( db, rgName, csName, clName, matcher );
        } finally {
            db.close();
        }
    }

    /**
     * check cs results of catalog, compare each node in catalog
     * 
     * @param sdb
     * @return true/false, true is success, false is failed
     */
    public static void checkCSOfCatalog( String csName ) {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName );
            matcher.put( "Name", subObj );

            // compare node's data within the group
            String rgName = "SYSCatalogGroup";
            String sysCsName = "SYSCAT";
            String sysClName = "SYSCOLLECTIONSPACES";
            compareNodeData( db, rgName, sysCsName, sysClName, matcher );
        } finally {
            db.close();
        }
    }

    /**
     * check cl results of catalog, compare each node in catalog
     * 
     * @param sdb
     * @return true/false, true is success, false is failed
     */
    public static void checkCLOfCatalog( Sequoiadb sdb, String csName,
            String clName ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "$regex", "^" + csName + "." + clName );
        matcher.put( "Name", subObj );

        // compare node's data within the group
        String rgName = "SYSCatalogGroup";
        String sysCSName = "SYSCAT";
        String sysCLName = "SYSCOLLECTIONS";
        compareNodeData( sdb, rgName, sysCSName, sysCLName, matcher );
    }

    /**
     * check RG info of catalog, compare each node in catalog
     * 
     * @param sdb
     */
    public static void checkRGOfCatalog( String groupName ) {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + groupName );
            matcher.put( "GroupName", subObj );

            // compare node's data within the group
            String rgName = "SYSCatalogGroup";
            String sysCsName = "SYSCAT";
            String sysClName = "SYSNODES";
            compareNodeData( db, rgName, sysCsName, sysClName, matcher );
        } finally {
            db.close();
        }
    }

    /**
     * check cl results of dataRG, compare each node in dataRG
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public static void checkCLOfDataRG( Sequoiadb sdb, String csName,
            String clName ) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject subObj = new BasicBSONObject();
        subObj.put( "$regex", "^" + csName + "." + clName );
        matcher.put( "Name", subObj );

        // get all dataGroupNames
        ArrayList< String > dataGroupNames = getDataGroupNames( sdb );
        for ( int i = 0; i < dataGroupNames.size(); i++ ) {
            List< String > nodeAddrs = getNodeAddress( sdb,
                    dataGroupNames.get( i ) );

            if ( nodeAddrs.size() < 2 ) { // other testCase create empty
                                          // group
                                          // or only one node that may be
                                          // cause to fail
                System.out.println( "group = " + dataGroupNames.get( i )
                        + ", nodeNum = " + nodeAddrs.size() );
                break;
                // throw new Exception("group = " + dataGroupNames.get(i) +
                // ", nodeNum = " + nodeAddrs.size());
            }

            // direct node and compare node's data
            int failCnt = 0;
            int maxCnt = 600;
            boolean checkSucc = false;
            do {
                ArrayList< String > allNodeData = new ArrayList< String >();

                for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                    Sequoiadb dataDB = new Sequoiadb( nodeAddrs.get( j ), "",
                            "" );
                    DBCursor cursor = dataDB.listCollections();

                    // get the data for each node
                    ArrayList< BSONObject > oneNodeData = new ArrayList< BSONObject >();
                    while ( cursor.hasNext() ) {
                        BSONObject clList = ( BSONObject ) cursor.getNext();
                        String name = clList.get( "Name" ).toString() ;
                        if ( name.indexOf("SYSRECYCLE") == -1 &&name.indexOf( clName ) >= 0 ){ 
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
                                Thread.sleep( 200 );
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

                }
            } while ( !checkSucc && failCnt < maxCnt );
        }
    }

    /**
     * check result for cl
     * 
     * @param sdb
     * @param csName
     * @param clName
     */
    public static void checkCLResult( String csName, String clName ) {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            checkCLOfCatalog( db, csName, clName );
            checkCLOfDataRG( db, csName, clName );

            boolean rc = compareDataAndCata( db, csName, clName );
            Assert.assertTrue( rc );

        } finally {
            db.close();
        }
    }

    /**
     * check index between all dataRG node
     * 
     * @param sdb
     * @param csName
     * @param clName
     * @throws InterruptedException
     */
    public static void checkIndex( String csName, String clName )
            throws InterruptedException {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            BSONObject matcher = new BasicBSONObject();
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "$regex", "^" + csName + "." + clName );
            matcher.put( "Name", subObj );

            DBCursor cursor = db.getSnapshot( 8, matcher, null, null );
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
                    List< String > nodeAddrs = getNodeAddress( db, groupName );
                    if ( nodeAddrs.size() <= 1 ) {
                        // testing for consistency requires more than two nodes
                        Assert.fail(
                                "the number of nodes in the group is less than 2!" );
                    }
                    // direct node and compare node's data
                    int failCnt = 0;
                    int maxCnt = 600;
                    boolean checkSucc = false;
                    do {
                        ArrayList< String > allNodeData = new ArrayList< String >();
                        for ( int j = 0; j < nodeAddrs.size(); j++ ) {
                            Sequoiadb dataDB = new Sequoiadb(
                                    nodeAddrs.get( j ), "", "" );
                            String tmpCSName = name.split( "\\." )[ 0 ];
                            String tmpCLName = name.split( "\\." )[ 1 ];
                            DBCollection clDB = dataDB
                                    .getCollectionSpace( tmpCSName )
                                    .getCollection( tmpCLName );
                            DBCursor cur = null;
                            if ( clDB != null ) {
                                cur = clDB.getIndexes();
                            }
                            // get the data for each node
                            ArrayList< String > oneNodeData = new ArrayList< String >();
                            while ( cur.hasNext() ) {
                                BSONObject idxList = ( BSONObject ) cur
                                        .getNext();
                                BSONObject IndexDef = ( BSONObject ) idxList
                                        .get( "IndexDef" );
                                String idxName = ( String ) IndexDef
                                        .get( "name" );
                                if ( !idxName.equals( "$id" ) ) {
                                    oneNodeData.add( idxName );
                                }
                            }
                            cur.close();
                            dataDB.close();
                            // all node data within the group
                            allNodeData.add( j, oneNodeData.toString() );
                            // compare data between nodes
                            if ( j == ( nodeAddrs.size() - 1 ) ) {
                                for ( int k = 0; k < j; k++ ) {
                                    // System.out.println("k=" + k + ",
                                    // oneNodeData = " + allNodeData.get(k));
                                    // System.out.println("k=" + (k+1) + ",
                                    // oneNodeData = " + allNodeData.get(k+1));
                                    if ( allNodeData.get( k ).equals(
                                            allNodeData.get( k + 1 ) ) ) {
                                        if ( k == ( j - 1 ) ) {
                                            checkSucc = true;
                                            break;
                                        }
                                    } else if ( ++failCnt < maxCnt ) {
                                        Thread.sleep( 200 );
                                        break;
                                    } else {
                                        Assert.fail( "clName = " + name
                                                + ", allNodeData = "
                                                + allNodeData.toString()
                                                + ", Failed to compare data between nodes, party node's data is not consistent." );
                                    }
                                }
                            }
                        }
                    } while ( !checkSucc && failCnt < maxCnt );
                }
            }
        } catch ( BaseException e ) {
            int eCode = e.getErrorCode();
            if ( eCode != -34 && eCode != -23 && eCode != -248 ) {
                // -248:Dropping the collection space is in progress
                throw e;
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
    public static boolean compareDataAndCata( Sequoiadb sdb, String csName,
            String clName ) {
        // get all dataGroupNames
        ArrayList< String > dataGroupNames = getDataGroupNames( sdb );
        for ( int i = 0; i < dataGroupNames.size(); i++ ) {
            // direct connect data master node, listCollections
            Sequoiadb dataDB = sdb.getReplicaGroup( dataGroupNames.get( i ) )
                    .getMaster().connect();
            DBCursor cursor = dataDB.listCollections();
            while ( cursor.hasNext() ) {
                String tmpCLName = ( String ) cursor.getNext().get( "Name" );
                if ( tmpCLName.indexOf( csName + "." + clName ) >= 0 ) {
                    // direct connect cata master node, find the collection
                    Sequoiadb cataDB = sdb.getReplicaGroup( "SYSCatalogGroup" )
                            .getMaster().connect();
                    BSONObject matcher = new BasicBSONObject();
                    matcher.put( "Name", tmpCLName );
                    DBCursor cur = cataDB.getCollectionSpace( "SYSCAT" )
                            .getCollection( "SYSCOLLECTIONS" )
                            .query( matcher, null, null, null );
                    while ( cur.hasNext() ) {
                        String name = ( String ) cur.getNext().get( "Name" );
                        if ( name.isEmpty() ) {
                            return false;
                        }
                    }
                    cur.close();
                    cataDB.close();
                }
            }
            cursor.close();
            dataDB.close();
        }

        try {
            // direct connect cata master node, find collections
            Sequoiadb cataDB = sdb.getReplicaGroup( "SYSCatalogGroup" )
                    .getMaster().connect();
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
                    String dataGroupNames1 = groupInfo.getString( "GroupName" );
                    // direct dataNode, get the collection
                    if ( dataGroupNames1 != null ) {
                        Sequoiadb dataDB = sdb
                                .getReplicaGroup( dataGroupNames1 ).getMaster()
                                .connect();
                        dataDB.getCollectionSpace( csName )
                                .getCollection( tmpCLName );

                        dataDB.close();
                    }
                }
            }

            cursor.close();
            cataDB.close();

        } catch ( BaseException e ) {
            int eCode = e.getErrorCode();
            if ( eCode == -23 ) {
                throw e;
            }
        }

        return true;
    }

    /**
     * compare node's data within the group
     * 
     * @param .......
     * @param matcher,
     *            matching condition for query
     */
    public static void compareNodeData( Sequoiadb sdb, String rgName,
            String csName, String clName, BSONObject matcher ) {
        Sequoiadb dataDB = null;
        // get node address within the group
        List< String > nodeAdrrs = getNodeAddress( sdb, rgName );
        if ( nodeAdrrs.size() < 2 ) {
            Assert.fail( "at least catalogNodes are required" );
        }
        // direct node and compare node's data
        int failCnt = 0;
        int maxCnt = 600;
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
                            Thread.sleep( 200 );
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
    }

    /**
     * clear domain, remove all domains that match domainName
     * 
     * @param sdb
     * @param domainName
     */
    public static void clearDomain( Sequoiadb sdb, String domainName ) {
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
            int eCode = e.getErrorCode();
            if ( eCode != -256 ) { // -256:Domain is not empty
                throw e;
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
            int eCode = e.getErrorCode();
            if ( eCode != -34 ) {
                throw e;
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
            int eCode = e.getErrorCode();
            if ( eCode != -23 && eCode != -34 ) {
                throw e;
            }
        }
    }

    /**
     * clear group, remove all groups that match rgName
     * 
     * @param sdb
     * @param rgName
     */
    public static void clearGroup( Sequoiadb sdb, String rgName ) {
        try {
            ArrayList< String > groupNames = getDataGroupNames( sdb );
            for ( int i = 0; i < groupNames.size(); i++ ) {
                String tmpName = groupNames.get( i );
                if ( tmpName.indexOf( rgName ) >= 0 ) {
                    sdb.removeReplicaGroup( tmpName );
                }
            }
        } catch ( BaseException e ) {
            int eCode = e.getErrorCode();
            if ( eCode != -154 ) { // -154:Group does not exist
                throw e;
            }
        }
    }

    /**
     * createNode
     * 
     * @param sdb
     */
    public static void createNode( Sequoiadb sdb, String rgName, int portStart,
            int portStop, String path ) {
        // get hostname
        // String hostName =
        // sdb.getReplicaGroup("SYSCatalogGroup").getMaster().getHostName();
        Random random = new Random();
        ReplicaGroup catalogGroup = sdb.getReplicaGroup( "SYSCatalogGroup" );
        List< String > hostNames = new ArrayList< String >();
        BasicBSONObject info = ( BasicBSONObject ) catalogGroup.getDetail();
        BasicBSONList group = ( BasicBSONList ) info.get( "Group" );
        for ( int i = 0; i < group.size(); ++i ) {
            BasicBSONObject tmp = ( BasicBSONObject ) group.get( i );
            String tmpHostName = tmp.getString( "HostName" );
            hostNames.add( tmpHostName );
        }

        // create node
        ReplicaGroup rg = sdb.getReplicaGroup( rgName );
        BSONObject rgConf = new BasicBSONObject();
        rgConf.put( "logfilesz", 64 );
        int svnName = portStart;
        boolean checkSucc = false;
        do {
            String nodePath = null;
            String hostName = null;
            nodePath = path + "data/" + String.valueOf( svnName );
            hostName = hostNames.get( random.nextInt( hostNames.size() ) );
            try {
                if ( rg != null ) {
                    rg.createNode( hostName, svnName, nodePath, rgConf );
                    checkSucc = true;
                    break;
                } else if ( rg == null ) {
                    System.out.println( "DataRG is not exist." );
                    break;
                }
            } catch ( BaseException e ) {
                int eCode = e.getErrorCode();
                if ( eCode == -157 // -157:Invalid node configuration(Port
                                   // is
                                   // occupied)
                        || eCode == -145 ) { // -145:Node already exists
                    svnName = svnName + 10;
                } else if ( eCode == -154 ) {
                    System.out.println( "DataRG is not exist." );
                    break;
                } else {
                    throw e;
                }
            }
        } while ( !checkSucc && svnName < portStop );
    }

    public static void insertData( Sequoiadb sdb, String csName,
            String clName ) {
        try {
            DBCollection clDB = sdb.getCollectionSpace( csName )
                    .getCollection( clName );

            ArrayList< BSONObject > records = new ArrayList< BSONObject >();
            for ( int i = 0; i < 100; i++ ) {
                BSONObject record = new BasicBSONObject();
                record.put( "a", i );
                records.add( record );
            }
            if ( clDB != null ) {
                clDB.insert( records );
            }
        } catch ( BaseException e ) {
            if ( -23 != e.getErrorCode() ) {
                throw e;
            }
        }
    }

    public static void sleep( int times ) {
        try {
            Thread.sleep( times );
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }
}
