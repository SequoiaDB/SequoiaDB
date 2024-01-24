package com.sequoiadb.testcommon;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicInteger;

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextUtils;

public class SdbTestBase {
    protected static String coordUrl;
    protected static String hostName;
    protected static String serviceName;
    public static String dsHostName;
    public static String dsServiceName;
    protected static String csName;
    protected static int reservedPortBegin;
    protected static int reservedPortEnd;
    protected static String reservedDir;
    protected static String workDir;
    private static String confToolScript;
    public static String backupPath;
    public static String rootPwd;
    public static String remoteUser;
    public static String remotePwd;
    private static String enableTransaction;
    private static Sequoiadb sequoiadb = null;
    public static String esHostName;
    public static String esServiceName;
    public static String cappedCSName;
    public static String expandGroupName;
    public static int expandNodeNum;
    public static String reservedCL = "java_dummy";

    private static final String TRANSACTIONON = "transactionon";
    private static final String TRANSISOLATION = "transisolation";
    private static final String TRANSLOCKWAIT = "translockwait";
    private static final String INDEXSCANSTEP = "indexscanstep";
    private static final String TRANSTIMEOUT = "transactiontimeout";
    private static final String TRANSAUTOCOMMIT = "transautocommit";
    private static final String TRANSAUTOROLLBACK = "transautorollback";
    private static final String TRANSUSERBS = "transuserbs";
    private static final String NODENAME = "NodeName";
    public static final String RU = "ru";
    public static final String RC = "rc";
    public static final String RCWAITLOCK = "rcwaitlock";
    public static final String RS = "rs";
    public static final String RCAUTO = "rcauto";
    public static final String RCUSERBS = "rcuserbs";
    public static final String LOCKESCALATION = "lockEscalation";
    public static final String TRANSREPLSIZE = "transreplsize";
    public static final String TRANSALLOWLOCKESCALATION = "transallowlockescalation";
    public static final String TRANSMAXLOCKNUM = "transmaxlocknum";
    public static final String RECYCLEBIN = "recycleBin";
    public static final String RECYCLEBINDEFAULTATTR = "recycleBinDefaultAttr";
    public static final String RECYCLEBINUSERATTR = "recycleBinUserAttr";
    public static final String ENABLE = "Enable";
    public static final String EXPIRETIME = "ExpireTime";
    public static final String MAXITEMNUM = "MaxItemNum";
    public static final String MAXVERSIONNUM = "MaxVersionNum";
    public static final String AUTODROP = "AutoDrop";
    public static final String LOCATION = "location";
    public static ArrayList< String > expandGroupNames = new ArrayList<>();
    public static ArrayList< BasicBSONObject > expandNodeInfos = null;
    public static final String RBAC = "rbac";
    public static final String rootUserName = "rootUserName";
    public static final String rootUserPassword = "rootUserPassword";

    private static ConfigOptions options = new ConfigOptions();
    public static String testGroup = null;
    private static final int newIndexScanStep = 100;
    private static final int transReplsize = 1;
    public static final int timeOutLen = 120;
    private static final Map< String, BSONObject > group2Conf = new HashMap< String, BSONObject >();
    private static final Map< String, AtomicInteger > group2Count = new HashMap< String, AtomicInteger >();
    private static final Map< String, BSONObject > node2Conf = new HashMap< String, BSONObject >();
    private static final Map< String, BSONObject > recycleBinAttr = new HashMap< String, BSONObject >();
    private static boolean istransactionOn = true;
    private static BasicBSONObject confObj = new BasicBSONObject();
    public static List< String > coordUrls = new ArrayList<>();
    private static boolean privilegecheck = false;

    static {
        group2Conf.put( RU, new BasicBSONObject() );
        group2Conf.get( RU ).put( TRANSISOLATION, 0 );
        group2Conf.get( RU ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RU ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RU ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RU ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RU ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RU ).put( TRANSUSERBS, true );
        group2Conf.get( RU ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RC, new BasicBSONObject() );
        group2Conf.get( RC ).put( TRANSISOLATION, 1 );
        group2Conf.get( RC ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RC ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RC ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RC ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RC ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RC ).put( TRANSUSERBS, true );
        group2Conf.get( RC ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RCWAITLOCK, new BasicBSONObject() );
        group2Conf.get( RCWAITLOCK ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCWAITLOCK ).put( TRANSLOCKWAIT, true );
        group2Conf.get( RCWAITLOCK ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCWAITLOCK ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCWAITLOCK ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RCWAITLOCK ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RCWAITLOCK ).put( TRANSUSERBS, true );
        group2Conf.get( RCWAITLOCK ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RS, new BasicBSONObject() );
        group2Conf.get( RS ).put( TRANSISOLATION, 2 );
        group2Conf.get( RS ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RS ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RS ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RS ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RS ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RS ).put( TRANSUSERBS, true );
        group2Conf.get( RS ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RCAUTO, new BasicBSONObject() );
        group2Conf.get( RCAUTO ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCAUTO ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RCAUTO ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCAUTO ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCAUTO ).put( TRANSAUTOCOMMIT, true );
        group2Conf.get( RCAUTO ).put( TRANSAUTOROLLBACK, false );
        group2Conf.get( RCAUTO ).put( TRANSUSERBS, true );
        group2Conf.get( RCAUTO ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( RCUSERBS, new BasicBSONObject() );
        group2Conf.get( RCUSERBS ).put( TRANSISOLATION, 1 );
        group2Conf.get( RCUSERBS ).put( TRANSLOCKWAIT, false );
        group2Conf.get( RCUSERBS ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( RCUSERBS ).put( TRANSTIMEOUT, timeOutLen );
        group2Conf.get( RCUSERBS ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( RCUSERBS ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( RCUSERBS ).put( TRANSUSERBS, false );
        group2Conf.get( RCUSERBS ).put( TRANSREPLSIZE, transReplsize );

        group2Conf.put( LOCKESCALATION, new BasicBSONObject() );
        group2Conf.get( LOCKESCALATION ).put( TRANSISOLATION, 2 );
        group2Conf.get( LOCKESCALATION ).put( TRANSLOCKWAIT, false );
        group2Conf.get( LOCKESCALATION ).put( INDEXSCANSTEP, newIndexScanStep );
        group2Conf.get( LOCKESCALATION ).put( TRANSTIMEOUT, 2 );
        group2Conf.get( LOCKESCALATION ).put( TRANSAUTOCOMMIT, false );
        group2Conf.get( LOCKESCALATION ).put( TRANSAUTOROLLBACK, true );
        group2Conf.get( LOCKESCALATION ).put( TRANSUSERBS, true );
        group2Conf.get( LOCKESCALATION ).put( TRANSREPLSIZE, transReplsize );
        group2Conf.get( LOCKESCALATION ).put( TRANSALLOWLOCKESCALATION, true );
        group2Conf.get( LOCKESCALATION ).put( TRANSMAXLOCKNUM, 10 );

        // 添加回收站默认属性
        recycleBinAttr.put( RECYCLEBINDEFAULTATTR, new BasicBSONObject() );
        recycleBinAttr.get( RECYCLEBINDEFAULTATTR ).put( ENABLE, true );
        recycleBinAttr.get( RECYCLEBINDEFAULTATTR ).put( EXPIRETIME, 4320 );
        recycleBinAttr.get( RECYCLEBINDEFAULTATTR ).put( MAXITEMNUM, 100 );
        recycleBinAttr.get( RECYCLEBINDEFAULTATTR ).put( MAXVERSIONNUM, 2 );
        recycleBinAttr.get( RECYCLEBINDEFAULTATTR ).put( AUTODROP, false );

        recycleBinAttr.put( RECYCLEBINUSERATTR, new BasicBSONObject() );

        for ( String key : group2Conf.keySet() ) {
            group2Count.put( key, new AtomicInteger( 0 ) );
            for ( String conf : group2Conf.get( key ).keySet() ) {
                if ( !confObj.containsField( conf ) ) {
                    confObj.put( conf, "" );
                }
            }
        }
    }

    public static synchronized void setRunGroup( List< String > testGroups ) {
        if ( testGroups.size() != 1 ) {
            return;
        }
        if ( !testGroups.get( 0 ).equals( SdbTestBase.testGroup ) ) {
            SdbTestBase.testGroup = testGroups.get( 0 );
        }
    }

    private static void getAllNodeConf( BasicBSONObject selector ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            selector.put( NODENAME, "" );
            selector.put( TRANSACTIONON, "" );

            DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CONFIGS, null,
                    selector, null );
            while ( cursor.hasNext() ) {
                BasicBSONObject doc = ( BasicBSONObject ) cursor.getNext();
                String key = doc.getString( NODENAME );
                doc.remove( NODENAME );
                node2Conf.put( key, doc );

                if ( doc.getString( TRANSACTIONON ).equals( "FALSE" ) ) {
                    istransactionOn = false;
                }
                doc.remove( TRANSACTIONON );
            }
            cursor.close();
        }
    }

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "ROOTPASSWD", "REMOTEUSER",
            "REMOTEPASSWD", "BACKUPTMPNODELOGPATH", "CONFTOOL",
            "ENABLETRANSACTION", "ESHOSTNAME", "ESSVCNAME", "FULLTEXTPREFIX",
            "DSHOSTNAME", "DSSVCNAME" })
    @BeforeSuite(alwaysRun = true)
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR,
            @Optional("sequoiadb") String ROOTPASSWD,
            @Optional("sdbadmin") String REMOTEUSER,
            @Optional("Admin@1024") String REMOTEPASSWD,
            @Optional("${BACKUPTMPNODELOGPATH}") String BACKUPTMPNODELOGPATH,
            @Optional("") String CONFTOOL,
            @Optional("false") String ENABLETRANSACTION,
            @Optional("localhost") String ESHOSTNAME,
            @Optional("9200") String ESSVCNAME,
            @Optional("") String FULLTEXTPREFIX,
            @Optional("localhost") String DSHOSTNAME,
            @Optional("11810") String DSSVCNAME ) {
        System.out.println( "initSuite....." );
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        esHostName = ESHOSTNAME;
        esServiceName = ESSVCNAME;
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
        confToolScript = CONFTOOL;
        backupPath = BACKUPTMPNODELOGPATH;
        enableTransaction = ENABLETRANSACTION;
        FullTextUtils.setFulltextPrefix( FULLTEXTPREFIX );
        dsHostName = DSHOSTNAME;
        dsServiceName = DSSVCNAME;

        try {
            if ( enableTransaction.equals( "true" ) ) {
                getAllNodeConf( confObj );
                if ( !istransactionOn ) {
                    modifyNodeConfAndRestart( true, "before" );
                }
            }

            options.setSocketKeepAlive( true );
            sequoiadb = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );
            if ( sequoiadb.isCollectionSpaceExist( csName ) ) {
                sequoiadb.dropCollectionSpace( csName,
                        new BasicBSONObject( "SkipRecycleBin", true ) );
            }
            CollectionSpace cs = sequoiadb.createCollectionSpace( csName,
                    ( BSONObject ) JSON.parse( "{LobPageSize: 8192}" ) );
            cs.createCollection( reservedCL, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{_id:1},AutoSplit:true}" ) );
            // add capped cs;
            if ( sequoiadb.isCollectionSpaceExist( cappedCSName ) ) {
                sequoiadb.dropCollectionSpace( cappedCSName,
                        new BasicBSONObject( "SkipRecycleBin", true ) );
            }
            sequoiadb.createCollectionSpace( cappedCSName,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );
            coordUrls = CommLib.getAllCoordUrls( sequoiadb );

            File workDirFile = new File( workDir );
            if ( !workDirFile.exists() ) {
                workDirFile.mkdir();
            }

        } catch ( BaseException e ) {
            e.printStackTrace();
            throw new RuntimeException( "initSuite failed" );
        } finally {
            if ( sequoiadb != null ) {
                sequoiadb.close();
            }
        }
    }

    private static BSONObject buildNodeConf( boolean transactionon ) {
        BasicBSONObject configs = new BasicBSONObject();
        configs.append( TRANSACTIONON, transactionon );
        return configs;
    }

    @SuppressWarnings("unused")
    private static BSONObject buildNodeConf( Properties prop ) {
        BasicBSONObject configs = new BasicBSONObject();
        for ( Object key : prop.keySet() ) {
            String value = prop.getProperty( ( String ) key );
            int val;
            try {
                val = Integer.parseInt( value );
                configs.append( ( String ) key, val );
            } catch ( NumberFormatException e ) {
                configs.append( ( String ) key, Boolean.parseBoolean( value ) );
            }
        }
        return configs;
    }

    private static void modifyNodeConfAndRestart( boolean transactionon,
            String mode ) {
        BSONObject defaultConf = new BasicBSONObject();
        BSONObject dataConf = buildNodeConf( transactionon );
        BSONObject stdalnConf = buildNodeConf( transactionon );
        try {
            createConfFile( defaultConf, defaultConf, dataConf, defaultConf,
                    dataConf, defaultConf, stdalnConf, defaultConf );
        } catch ( IOException e1 ) {
            e1.printStackTrace();
            throw new RuntimeException( "initGroups failed!!!" );
        }

        String[] cmd;
        try {
            cmd = getConfCmd( mode, confToolScript );
            if ( !execCmd( cmd ) ) {
                throw new RuntimeException(
                        "exec script failed, initGroups failed!!!" );
            }
        } catch ( IOException | InterruptedException e ) {
            e.printStackTrace();
            throw new RuntimeException( "initGroups failed!!!" );
        }
    }

    private static void modifyNodeConf( BSONObject cfg, BSONObject object ) {
        if ( object == null ) {
            object = new BasicBSONObject().append( "Global", true );
        }
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            sdb.updateConfig( cfg, object );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
    }

    private static void modifyRecycleBinAttr( BSONObject attr ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            sdb.getRecycleBin().alter( attr );
        }
    }

    private static void getRecycleBinAttr() {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                options )) {
            recycleBinAttr.get( RECYCLEBINUSERATTR )
                    .putAll( sdb.getRecycleBin().getDetail() );
        }
    }

    @Parameters({ "EXPANDNODENUM" })
    @BeforeTest(groups = { RU, RC, RCWAITLOCK, RS, RCAUTO, RCUSERBS,
            LOCKESCALATION, RECYCLEBIN, LOCATION, RBAC })
    public static synchronized void initTestGroups(
            @Optional("0") int EXPANDNODENUM ) throws Exception {
        if ( testGroup == null ) {
            return;
        } else if ( testGroup.equals( RECYCLEBIN ) ) {
            // 修改回收站属性为默认属性
            getRecycleBinAttr();
            modifyRecycleBinAttr( recycleBinAttr.get( RECYCLEBINDEFAULTATTR ) );
        }
        // 对需要扩容的测试用例创建一个复制组，并创建节点
        if ( testGroup.equals( LOCATION ) ) {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                    options )) {
                expandNodeNum = EXPANDNODENUM;
                if ( sdb.isReplicaGroupExist( expandGroupName ) ) {
                    sdb.getReplicaGroup( expandGroupName ).start();
                } else {
                    sdb.createReplicaGroup( expandGroupName );
                }
                expandGroupNames.add( expandGroupName );
                expandNodeInfos = CommLib.createNode( sdb, expandGroupName,
                        expandNodeNum );
                // 扩容完成后校验LSN一致
                CommLib.waitGroupSelectPrimaryNode( sdb, expandGroupName, 60 );
                if ( !CommLib.isLSNConsistency( sdb, expandGroupName ) ) {
                    Assert.fail( "LSN is not consistency" );
                }
            }
        }
        System.out.println( "init " + testGroup + " Groups..........." );
        modifyNodeConf( group2Conf.get( testGroup ), null );

        if ( testGroup.equals( RBAC ) ) {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                    options )) {
                privilegecheck = CommLib.getPrivilegecheck( sdb );
            }
            if ( !privilegecheck ) {
                CommLib.setPrivilegecheck( !privilegecheck );
            }
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                    options )) {
                BSONObject options = ( BSONObject ) JSON
                        .parse( "{Roles:['_root']}" );
                sdb.createUser( rootUserName, rootUserPassword, options );
            }
        }
    }

    @Parameters({ "EXPANDNODENUM" })
    @AfterTest(groups = { RC, RU, RCWAITLOCK, RS, RCAUTO, RCUSERBS,
            LOCKESCALATION, RECYCLEBIN, LOCATION, RBAC }, alwaysRun = true)
    public static synchronized void finiTestGroups(
            @Optional("0") int EXPANDNODENUM ) throws Exception {
        if ( testGroup == null ) {
            return;
        } else if ( testGroup.equals( RBAC ) ) {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl,
                    rootUserName, rootUserPassword, options )) {
                sdb.removeUser( rootUserName, rootUserPassword );
            }
            if ( !privilegecheck ) {
                CommLib.setPrivilegecheck( privilegecheck );
            }
        } else if ( testGroup.equals( RECYCLEBIN ) ) {
            // 执行完用例后将回收站配置改为执行用例前配置
            modifyRecycleBinAttr( recycleBinAttr.get( RECYCLEBINUSERATTR ) );
        }
        // 移除增加的复制组
        if ( testGroup.equals( LOCATION ) ) {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "",
                    options )) {
                for ( String expandGroupName : expandGroupNames ) {
                    CommLib.isLSNConsistency( sdb, expandGroupName );
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
        System.out.println( "fini " + testGroup + " Groups..........." );
        for ( String key : node2Conf.keySet() ) {
            BasicBSONObject opt = new BasicBSONObject();
            opt.put( NODENAME, key );
            modifyNodeConf( node2Conf.get( key ), opt );
        }
        testGroup = null;
    }

    @AfterSuite(alwaysRun = true)
    public static void finiSuite() {
        try {
            if ( enableTransaction.equals( "true" ) && !istransactionOn ) {
                modifyNodeConfAndRestart( false, "after" );
            }
            sequoiadb = new Sequoiadb( SdbTestBase.coordUrl, "", "", options );

            // if (sequoiadb.isCollectionSpaceExist(csName)) {
            // sequoiadb.dropCollectionSpace(csName);
            // }

            // drop capped cs
            // if (sequoiadb.isCollectionSpaceExist(cappedCSName)) {
            // sequoiadb.dropCollectionSpace(cappedCSName);
            // }
            // sdb.close() ;

        } catch ( BaseException e ) {
            e.printStackTrace();
        } finally {
            if ( sequoiadb != null ) {
                sequoiadb.close();
            }

            SdbThreadBase.shutdown();
        }
    }

    public static String getDefaultCoordUrl() {
        return coordUrl;
    }

    public static String getWorkDir() {
        return workDir;
    }

    private static boolean execCmd( String[] cmd )
            throws IOException, InterruptedException {
        System.out.println( "cmd:" + Arrays.toString( cmd ) );
        Process process = Runtime.getRuntime().exec( cmd );

        BufferedReader input = new BufferedReader(
                new InputStreamReader( process.getInputStream() ) );
        String line = "";
        while ( ( line = input.readLine() ) != null ) {
            System.out.println( line );
        }

        int exitValue = process.waitFor();
        if ( 0 != exitValue ) {
            System.out.println(
                    "fail to change node configure beforetest, return code="
                            + exitValue );
            return false;
        } else {
            return true;
        }
    }

    private static String[] getConfCmd( String mode, String confToolScript )
            throws IOException {
        String[] cmd = new String[ 5 ];
        String confFullName = "";
        if ( mode.equals( "before" ) ) {
            confFullName = getCurrentClass().getResource( "" ) + "/node.conf";
            confFullName = confFullName.substring( 5 );
        } else {
            confFullName = System.getProperty( "user.dir" ) + "/node.conf.ini";
        }

        Properties prop = new Properties();
        InputStream in = new FileInputStream(
                new File( "/etc/default/sequoiadb" ) );
        prop.load( in );
        String installPath = prop.getProperty( "INSTALL_DIR" );
        String sdbFullName = installPath + "/bin/sdb";
        System.out.println( sdbFullName );
        cmd[ 0 ] = sdbFullName;
        cmd[ 1 ] = "-f";
        cmd[ 2 ] = confFullName + "," + confToolScript;
        cmd[ 3 ] = "-e";
        cmd[ 4 ] = "var hostname='" + hostName + "';" + "var svcname="
                + serviceName + ";" + "var mode='" + mode + "'";

        return cmd;
    }

    private static void createConfFile( BSONObject cataConf,
            BSONObject cataDynaConf, BSONObject coordConf,
            BSONObject coordDynaConf, BSONObject dataConf,
            BSONObject dataDynaConf, BSONObject stdalnConf,
            BSONObject stdalnDynaConf ) throws IOException {
        String confPath = getCurrentClass().getResource( "" ).getPath()
                + "/node.conf";
        FileWriter confFile = new FileWriter( confPath );
        addStaticConf( confFile, cataConf, coordConf, dataConf, stdalnConf );
        addDynConf( confFile, cataDynaConf, coordDynaConf, dataDynaConf,
                stdalnDynaConf );
        confFile.flush();
        confFile.close();
        System.out.println( "create file: " + confPath );
    }

    @SuppressWarnings("rawtypes")
    private static final Class getCurrentClass() {
        return new Object() {
            public Class getClassForStatic() {
                return this.getClass();
            }
        }.getClassForStatic();
    }

    private static void addStaticConf( FileWriter confFile, BSONObject cataConf,
            BSONObject coordConf, BSONObject dataConf, BSONObject stdalnConf )
            throws IOException {
        confFile.write( "catalogConf = " + cataConf + ";\n" );
        confFile.write( "coordConf = " + coordConf + ";\n" );
        confFile.write( "dataConf = " + dataConf + ";\n" );
        confFile.write( "standaloneConf = " + stdalnConf + ";\n" );
    }

    private static void addDynConf( FileWriter confFile,
            BSONObject cataDynaConf, BSONObject coordDynaConf,
            BSONObject dataDynaConf, BSONObject stdalnDynaConf )
            throws IOException {
        confFile.write( "catalogDynaConf = " + cataDynaConf + ";\n" );
        confFile.write( "coordDynaConf = " + coordDynaConf + ";\n" );
        confFile.write( "dataDynaConf = " + dataDynaConf + ";\n" );
        confFile.write( "standaloneConf = " + stdalnDynaConf + ";\n" );
    }
}
