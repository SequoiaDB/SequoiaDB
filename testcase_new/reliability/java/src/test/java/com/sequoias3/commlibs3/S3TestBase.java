package com.sequoias3.commlibs3;

import java.io.File;
import java.util.List;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Optional;
import org.testng.annotations.Parameters;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;

public class S3TestBase {
    public static String coordUrl;
    public static String hostName;
    public static String serviceName;
    public static String csName;
    public static int reservedPortBegin;
    public static int reservedPortEnd;
    public static String reservedDir;
    public static String workDir;
    public static String rootPwd;
    public static String remoteUser;
    public static String remotePwd;
    public static String scriptDir;

    public static String s3ClientUrl;
    public static String s3HostName;
    public static String s3Port;
    public static String bucketName;
    public static String enableVerBucketName;
    public static String s3UserName;
    public static String s3AccessKeyId;
    public static String installPath;
    public static String propertiesFileName = "";
    public static String replaceFileName = "";
    private static String clusterFileName = "";
    private static String clusterInfo = "";
    private static StorageInterface storage = new SdbStorage();
    private final static String S3_CS_PREFIX = "S3_SYS";

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "S3HOSTNAME", "S3PORT",
            "S3USERNAME", "S3ACCESSKEYID", "ROOTPASSWD", "REMOTEUSER",
            "REMOTEPASSWD", "SCRIPTDIR" })
    @BeforeSuite(alwaysRun = true)
    public void initSuite( String HOSTNAME, String SVCNAME, String COMMCSNAME,
            int RSRVPORTBEGIN, int RSRVPORTEND, String RSRVNODEDIR,
            String WORKDIR, String S3HOSTNAME, @Optional("8002") String S3PORT,
            String S3USERNAME, String S3ACCESSKEYID, String ROOTPASSWD,
            String REMOTEUSER, String REMOTEPASSWD, String SCRIPTDIR )
            throws Exception {

        SdbTestBase.hostName = hostName = HOSTNAME;
        SdbTestBase.serviceName = serviceName = SVCNAME;
        SdbTestBase.csName = csName = COMMCSNAME;
        SdbTestBase.reservedPortBegin = reservedPortBegin = RSRVPORTBEGIN;
        SdbTestBase.reservedPortEnd = reservedPortEnd = RSRVPORTEND;
        SdbTestBase.reservedDir = reservedDir = RSRVNODEDIR;
        SdbTestBase.workDir = workDir = WORKDIR;
        SdbTestBase.coordUrl = coordUrl = HOSTNAME + ":" + SVCNAME;
        SdbTestBase.rootPwd = rootPwd = ROOTPASSWD;
        SdbTestBase.remoteUser = remoteUser = REMOTEUSER;
        SdbTestBase.remotePwd = remotePwd = REMOTEPASSWD;
        SdbTestBase.scriptDir = scriptDir = SCRIPTDIR;

        s3HostName = S3HOSTNAME;
        s3Port = S3PORT;
        s3UserName = S3USERNAME;
        s3AccessKeyId = S3ACCESSKEYID;
        s3ClientUrl = "http://" + S3HOSTNAME + ":" + S3PORT;
        bucketName = "commbucket";
        enableVerBucketName = "commbucketwithversion";

        getInstallPath();
        storage.envPrePare( coordUrl );
        changeConfAndStartS3();
        // clean file
        File workDirFile = new File( workDir );
        if ( !workDirFile.exists() ) {
            workDirFile.mkdir();
        }
        createCommonCS();
        cleanS3EnvAndPrepare();
    }

    @AfterSuite(alwaysRun = true)
    public void finiSuite( ITestContext context ) throws Exception {
        try {
            execCmd( Command.S3_RESTORECONF );
            clusterFileName = installPath + "/tools/sequoias3/log/cluster.log";
            clusterInfo = storage.getClusterInfo( coordUrl );
            execCmd( Command.S3_SAVECLUSTERINFO );
            storage.envRestore( coordUrl );
            if ( context.getFailedTests().size() == 0 ) {
                dropS3CS();
            }
        } finally {
            execCmd( Command.S3_STOP );
        }
    }

    public static String getDefaultCoordUrl() {
        return coordUrl;
    }

    public static String getWorkDir() {
        return workDir;
    }

    public static String getDefaultS3ClientUrl() {
        return s3ClientUrl;
    }

    public static void changeConfAndStartS3() throws Exception {
        // 更新properties
        try {
            execCmd( Command.S3_SETCONFBEFORE );
            // change log level
            execCmd( Command.S3_CHANGEDIALEVEL );
        } catch ( Exception e ) {
            e.printStackTrace();
            Assert.fail( "update application.properties file failed" );
        }
        System.out.println( "finish update application.properties" );
        execCmd( Command.S3_CHECKPORTALIVE );
        String output = Command.S3_CHECKPORTALIVE.getOutput();
        // 检查如已存在s3进程，则重启s3服务，不存在的话就直接启动s3
        if ( output.contains( "sequoias3" ) ) {
            System.out.println( "restart s3..." );
            execCmd( Command.S3_STOP );
            execCmd( Command.S3_START );
        } else {
            execCmd( Command.S3_START );
        }
    }

    public static void getInstallPath() throws Exception {
        Command.S3_GETINSTALLPATH.execCmd( s3HostName, remoteUser, remotePwd,
                Command.S3_GETINSTALLPATH.cmd );
        installPath = Command.S3_GETINSTALLPATH.getOutput();
    }

    public static void createCommonCS() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            if ( !db.isCollectionSpaceExist( csName ) ) {
                db.createCollectionSpace( csName );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    public static void dropS3CS() {
        Sequoiadb db = null;
        DBCursor cursor = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            cursor = db.listCollectionSpaces();
            while ( cursor.hasNext() ) {
                String csName = ( String ) cursor.getNext().get( "Name" );
                if ( csName.startsWith( S3_CS_PREFIX ) ) {
                    db.dropCollectionSpace( csName );
                }
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( db != null ) {
                db.close();
            }
        }
    }

    public void cleanS3EnvAndPrepare() {
        AmazonS3 s3Client = null;
        try {
            // clean up existing buckets
            s3Client = CommLibS3.buildS3Client();
            List< Bucket > buckets = s3Client.listBuckets();
            for ( int i = 0; i < buckets.size(); i++ ) {
                String bucketName = buckets.get( i ).getName();
                String bucketVerStatus = s3Client
                        .getBucketVersioningConfiguration( bucketName )
                        .getStatus();
                if ( bucketVerStatus == "null" ) {
                    CommLibS3.deleteAllObjects( s3Client, bucketName );
                } else {
                    CommLibS3.deleteAllObjectVersions( s3Client, bucketName );
                }
                s3Client.deleteBucket( bucketName );
            }
            // create bucket
            s3Client.createBucket( new CreateBucketRequest( bucketName ) );
            // create bucket by enable versioning
            s3Client.createBucket(
                    new CreateBucketRequest( enableVerBucketName ) );
            CommLibS3.setBucketVersioning( s3Client, enableVerBucketName,
                    "Enabled" );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private static void execCmd( Command cmd ) throws Exception {
        String command = "";
        switch ( cmd ) {
        case S3_CHECKPORTALIVE:
        case S3_START:
        case S3_STOP:
            cmd.exec( s3HostName, remoteUser, remotePwd, installPath );
            break;
        case S3_SETCONFBEFORE:
            propertiesFileName = installPath
                    + "/tools/sequoias3/config/application.properties";
            replaceFileName = installPath
                    + "/tools/sequoias3/config/ori_application.properties";
            String coordUrls = storage.getUrls( coordUrl );
            cmd.exec( s3HostName, remoteUser, remotePwd, propertiesFileName,
                    replaceFileName, coordUrls, propertiesFileName );
            break;
        case S3_CHANGEDIALEVEL:
            String logBackFileName = installPath
                    + "/tools/sequoias3/config/logback.xml";
            cmd.exec( s3HostName, remoteUser, remotePwd, logBackFileName );
            break;
        case S3_RESTORECONF:
            cmd.exec( s3HostName, remoteUser, remotePwd, propertiesFileName,
                    replaceFileName, propertiesFileName );
            break;
        case S3_SAVECLUSTERINFO:
            cmd.exec( s3HostName, remoteUser, remotePwd, clusterInfo,
                    clusterFileName );
            break;
        default:
            break;
        }

        Ssh ssh = null;
        try {
            ssh = new Ssh( s3HostName, remoteUser, remotePwd );
            ssh.exec( command );
            if ( ssh.getExitStatus() != 0 ) {
                throw new Exception( "exec command : " + command
                        + " failed, stout= " + ssh.getStdout() );
            }
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
    }

    enum Command {
        S3_CHECKPORTALIVE("%s/tools/sequoias3/sequoias3.sh status"), S3_START(
                "source /etc/profile;%s/tools/sequoias3/sequoias3.sh start > /tmp/s3start.log"), S3_STOP(
                        "%s/tools/sequoias3/sequoias3.sh stop -a"), S3_SETCONFBEFORE(
                                "mv %s %s;echo 'sdbs3.sequoiadb.url=sequoiadb://%s' > %s"), S3_CHANGEDIALEVEL(
                                        "sed -i 's/INFO/DEBUG/g' %s"), S3_RESTORECONF(
                                                "rm -f %s;mv %s %s"), S3_CHANGECONF_BEFORETEST(
                                                        "echo '%s' >> %s"), S3_CHANGECONF_AFTERTEST(
                                                                "sed -i 's/%s/#%s/g' %s"), S3_SAVECLUSTERINFO(
                                                                        "echo %s > %s"), S3_GETINSTALLPATH(
                                                                                "cat /etc/default/sequoiadb | grep 'INSTALL_DIR' | awk -F '=' '{printf(\"%s\",$2)}'");

        private String cmd;
        private String output;

        private Command( String cmd ) {
            this.cmd = cmd;
        }

        public void execCmd( String remoteHost, String user, String password,
                String command ) throws Exception {
            Ssh ssh = null;
            try {
                ssh = new Ssh( remoteHost, user, password );
                ssh.exec( command );
                if ( ssh.getExitStatus() != 0 ) {
                    throw new Exception( "exec command : " + command
                            + " failed, stout= " + ssh.getStdout() );
                }
                this.output = ssh.getStdout();
            } finally {
                if ( ssh != null ) {
                    ssh.disconnect();
                }
            }
        }

        public void exec( String remoteHost, String user, String password,
                String... args ) throws Exception {
            String command = String.format( this.cmd, args );
            execCmd( remoteHost, user, password, command );
        }

        public String getOutput() {
            return this.output;
        }
    }
}