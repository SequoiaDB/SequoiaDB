package com.sequoiadb.commlib;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Optional;
import org.testng.annotations.Parameters;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fulltext.FullTextUtils;

public class SdbTestBase {
    public static String coordUrl;
    public static String srcCoordUrl;
    public static String hostName;
    public static String serviceName;
    public static String dsHostName;
    public static String dsServiceName;
    public static String csName;
    public static String cappedCSName;
    public static int reservedPortBegin;
    public static int reservedPortEnd;
    public static String reservedDir;
    public static String workDir;
    public static String backupPath;
    public static String rootPwd;
    public static String remoteUser;
    public static String remotePwd;
    public static String scriptDir;
    public static String esHostName;
    public static String esServiceName;
    public static String sdbseadapterDir;
    public static String expandGroupName;
    public static int expandNodeNum;
    private static boolean srcdbExist = false;

    private static final String TRANSISOLATION = "transisolation";
    private static final String TRANSLOCKWAIT = "translockwait";
    private static final String TRANSAUTOCOMMIT = "transautocommit";
    private static final String TRANSAUTOROLLBACK = "transautorollback";
    private static final String TRANSUSERBS = "transuserbs";
    private static final String TRANSREPLSIZE = "transreplsize";
    private static final String RCAUTO = "rcauto";
    private static final String RC = "rc";
    private static final String NODENAME = "NodeName";
    public static final String LOCATION = "location";
    public static ArrayList< String > expandGroupNames = new ArrayList<>();
    public static ArrayList< BasicBSONObject > expandNodeInfos = null;

    private static final Map< String, BSONObject > group2Conf = new HashMap<>();
    private static final Map< String, BSONObject > node2Conf = new HashMap<>();
    private static final Map< String, AtomicInteger > groupName2Count = new HashMap<>();
    private static BasicBSONObject confObj = new BasicBSONObject();
    public static String testGroupOfCurrent;
    private static final int transReplsize = 2;

    public static void setTestGroup( List< String > testGroup ) {
        if ( testGroup == null || testGroup.isEmpty() )
            return;
        SdbTestBase.testGroupOfCurrent = testGroup.get( 0 );
    }

    private static void getAllNodeConf( BasicBSONObject selector ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            selector.put( NODENAME, "" );

            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cursor.getNext();
                String key = doc.getString( NODENAME );
                doc.remove( NODENAME );
                node2Conf.put( key, doc );
            }
            cursor.close();
        }
    }

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "ROOTPASSWD", "REMOTEUSER",
            "REMOTEPASSWD", "SCRIPTDIR", "BACKUPTMPNODELOGPATH", "ESHOSTNAME",
            "ESSVCNAME", "FULLTEXTPREFIX", "SDBSEADAPTERDIR", "DSHOSTNAME",
            "DSSVCNAME" })
    @BeforeSuite(alwaysRun = true)
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR, String ROOTPASSWD,
            String REMOTEUSER, String REMOTEPASSWD, String SCRIPTDIR,
            @Optional("${BACKUPTMPNODELOGPATH}") String BACKUPTMPNODELOGPATH,
            @Optional("localhost") String ESHOSTNAME,
            @Optional("9200") String ESSVCNAME,
            @Optional("") String FULLTEXTPREFIX,
            @Optional("/opt/sequoiadb/conf/sdbseadapter") String SDBSEADAPTERDIR,
            @Optional("${DSHOSTNAME}") String DSHOSTNAME,
            @Optional("11810") String DSSVCNAME ) {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        cappedCSName = COMMCSNAME + "_capped";
        expandGroupName = COMMCSNAME + "_group";
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        coordUrl = HOSTNAME + ":" + SVCNAME;
        rootPwd = ROOTPASSWD;
        remoteUser = REMOTEUSER;
        remotePwd = REMOTEPASSWD;
        scriptDir = SCRIPTDIR;
        backupPath = BACKUPTMPNODELOGPATH;
        esHostName = ESHOSTNAME;
        esServiceName = ESSVCNAME;
        FullTextUtils.setFulltextPrefix( FULLTEXTPREFIX );
        sdbseadapterDir = SDBSEADAPTERDIR;
        dsHostName = DSHOSTNAME;
        dsServiceName = DSSVCNAME;
        srcCoordUrl = DSHOSTNAME + ":" + DSSVCNAME;

        getAllNodeConf( confObj );
        try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" )) {
            boolean ret = createCommonCS( db );
            Assert.assertTrue( ret );
            createWorkDir();
            createReserveDir();
        } catch ( BaseException e ) {
            Assert.fail( "connect " + coordUrl + ": " + e.getErrorCode() );
        }

        if ( !"${DSHOSTNAME}".equals( DSHOSTNAME ) ) {
            try ( Sequoiadb srcdb = new Sequoiadb( srcCoordUrl, "", "" )) {
                srcdbExist = true;
                boolean ret = createCommonCS( srcdb );
                Assert.assertTrue( ret );
                createWorkDir( srcCoordUrl );
                createReserveDir( srcCoordUrl );
            } catch ( BaseException e ) {
                Assert.fail(
                        "connect " + srcCoordUrl + ": " + e.getErrorCode() );
            }
        }

    }

    static {
        group2Conf.put( RCAUTO, new BasicBSONObject() );
        group2Conf.get( RCAUTO ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCAUTO ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RCAUTO ).put( TRANSAUTOCOMMIT, true );
        group2Conf.get( RCAUTO ).put( TRANSAUTOROLLBACK, false );
        group2Conf.get( RCAUTO ).put( TRANSUSERBS, true );
        group2Conf.get( RCAUTO ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RC, new BasicBSONObject() );
        group2Conf.get( RC ).put( TRANSISOLATION, 1 );
        group2Conf.get( RC ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RC ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RC ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RC ).put( TRANSUSERBS, true );
        group2Conf.get( RC ).put( TRANSREPLSIZE, transReplsize );

        for ( String key : group2Conf.keySet() ) {
            groupName2Count.put( key, new AtomicInteger( 0 ) );
            for ( String conf : group2Conf.get( key ).keySet() ) {
                if ( !confObj.containsField( conf ) ) {
                    confObj.put( conf, "" );
                }
            }
        }
        groupName2Count.put( LOCATION, new AtomicInteger( 0 ) );
    }

    private static void modifyNodeConf( BSONObject cfg, BSONObject object ) {
        if ( object == null ) {
            object = new BasicBSONObject().append( "Global", true );
        }

        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            sdb.updateConfig( cfg, object );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
    }

    @Parameters({ "EXPANDNODENUM" })
    @BeforeTest(groups = { RC, RCAUTO, LOCATION })
    public static synchronized void initTestGroups(
            @Optional("0") int EXPANDNODENUM ) {
        if ( !groupName2Count.containsKey( testGroupOfCurrent ) ) {
            return;
        }

        if ( groupName2Count.get( testGroupOfCurrent ).getAndIncrement() > 0 ) {
            return;
        }
        // 对需要扩容的测试用例选择一个复制组进行扩容
        if ( testGroupOfCurrent.equals( LOCATION ) ) {
            int timeout = 300;
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                expandNodeNum = EXPANDNODENUM;
                if ( sdb.isReplicaGroupExist( expandGroupName ) ) {
                    try {
                        sdb.getReplicaGroup( expandGroupName ).start();
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() != SDBError.SDB_CLS_EMPTY_GROUP
                                .getErrorCode() ) {
                            throw e;
                        }
                    }
                } else {
                    sdb.createReplicaGroup( expandGroupName );
                }
                System.err.println( "expandNodeNum -- " + expandNodeNum );
                expandGroupNames.add( expandGroupName );
                expandNodeInfos = CommLib.createNode( sdb, expandGroupName,
                        expandNodeNum );
                System.out.println( "expandNodeInfos -- " + expandNodeInfos );
                // 扩容完成后校验LSN一致
                CommLib.waitGroupSelectPrimaryNode( sdb, expandGroupName, 60 );
                if ( !CommLib.isLSNConsistency( sdb,
                        SdbTestBase.expandGroupName ) ) {
                    Assert.fail( "LSN is not consistency" );
                }
            }
        }

        System.out
                .println( "init " + testGroupOfCurrent + " Groups..........." );
        modifyNodeConf( group2Conf.get( testGroupOfCurrent ), null );
    }

    @Parameters({ "EXPANDNODENUM" })
    @AfterTest(groups = { RC, RCAUTO, LOCATION }, alwaysRun = true)
    public static synchronized void finiTestGroups(
            @Optional("0") int EXPANDNODENUM ) {
        if ( !groupName2Count.containsKey( testGroupOfCurrent ) ) {
            return;
        }

        if ( groupName2Count.get( testGroupOfCurrent ).decrementAndGet() < 0 ) {
            return;
        }

        // 移除扩容的节点
        if ( testGroupOfCurrent.equals( LOCATION ) ) {
            int timeout = 300;
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                for ( String expandGroupName : expandGroupNames ) {
                    if ( !CommLib.isLSNConsistency( sdb,
                            SdbTestBase.expandGroupName ) ) {
                        Assert.fail( "LSN is not consistency" );
                    }
                    System.out.println( "backupPath -- " + backupPath );
                    System.out.println( "backupPath.equals -- "
                            + ( !backupPath.equals( "" ) ) );
                    if ( !"${BACKUPTMPNODELOGPATH}".equals( backupPath ) ) {
                        String backupPathFull = backupPath + "/" + LOCATION
                                + EXPANDNODENUM;
                        System.out.println(
                                "backupPathFull -- " + backupPathFull );
                        BasicBSONObject matcher = new BasicBSONObject(
                                "GroupName", expandGroupName );
                        String user = "root";
                        try {
                            CommLib.copyNodeLogs( sdb, matcher, user,
                                    SdbTestBase.rootPwd, backupPathFull );
                        } catch ( Exception e ) {
                            e.printStackTrace();
                            throw new RuntimeException( e );
                        }
                    }
                    CommLib.cleanUpCSInGroup( sdb, expandGroupName );
                    sdb.removeReplicaGroup( expandGroupName );
                }
            }
        }

        System.out
                .println( "fini " + testGroupOfCurrent + " Groups..........." );
        for ( String key : node2Conf.keySet() ) {
            BasicBSONObject opt = new BasicBSONObject();
            opt.put( NODENAME, key );
            modifyNodeConf( node2Conf.get( key ), opt );
        }
    }

    private static void createReserveDir() {
        createReserveDir( coordUrl );
    }

    private static void createReserveDir( String coordUrl ) {
        try {
            GroupMgr mgr = GroupMgr.getInstance( coordUrl );
            List< String > hosts = mgr.getAllHosts( coordUrl );
            for ( String host : hosts ) {
                Ssh ssh = new Ssh( host, "root", SdbTestBase.rootPwd );
                try {
                    ssh.exec( "mkdir -p " + SdbTestBase.reservedDir );
                    ssh.exec( "chown sdbadmin:sdbadmin_group "
                            + SdbTestBase.reservedDir );
                } finally {
                    ssh.disconnect();
                }
            }
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() );
            e.printStackTrace();
        }
    }

    private static void createWorkDir() {
        createWorkDir( coordUrl );
    }

    private static void createWorkDir( String coordUrl ) {
        try {
            GroupMgr mgr = GroupMgr.getInstance( coordUrl );
            List< String > hosts = mgr.getAllHosts( coordUrl );
            for ( String host : hosts ) {
                Ssh ssh = new Ssh( host, "root", SdbTestBase.rootPwd );
                try {
                    ssh.exec( "mkdir -p " + SdbTestBase.workDir );
                } finally {
                    ssh.disconnect();
                }
            }
        } catch ( ReliabilityException e ) {
            Assert.fail( e.getMessage() );
            e.printStackTrace();
        }
    }

    @AfterSuite(enabled = false)
    public static void finiSuite() {
        System.out.println( "finisuit" );
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
            if ( db.isCollectionSpaceExist( cappedCSName ) ) {
                db.dropCollectionSpace( cappedCSName );
            }
            if ( srcdbExist ) {
                Sequoiadb srcdb = new Sequoiadb( srcCoordUrl, "", "" );
                if ( srcdb.isCollectionSpaceExist( csName ) ) {
                    srcdb.dropCollectionSpace( csName );
                }
                if ( srcdb.isCollectionSpaceExist( cappedCSName ) ) {
                    srcdb.dropCollectionSpace( cappedCSName );
                }
                srcdb.close();
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private static boolean createCommonCS( Sequoiadb sdb ) {
        boolean isCreateSuccess = true;
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
            sdb.createCollectionSpace( csName );

            // 创建公共的固定集合空间
            if ( sdb.isCollectionSpaceExist( cappedCSName ) ) {
                sdb.dropCollectionSpace( cappedCSName );
            }
            sdb.createCollectionSpace( cappedCSName,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );
        } catch ( BaseException e ) {
            System.out.printf( "create CollectionSpace %s failed, errMsg:%s\n",
                    csName + " or " + cappedCSName, e.getMessage() );
            isCreateSuccess = false;
        }
        return isCreateSuccess;
    }

}
