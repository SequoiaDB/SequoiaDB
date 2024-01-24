package com.sequoiadb.testcommon;

import java.io.IOException;
import java.io.File;
import java.util.TimeZone;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Optional;
import org.testng.annotations.Parameters;

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
    protected static String username;
    protected static String password;
    public static TimeZone defaultZone = TimeZone.getDefault();

    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR", "REMOTEUSER",
            "REMOTEPASSWD", "DSHOSTNAME", "DSSVCNAME" })
    @BeforeSuite
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR, String REMOTEUSER,
            String REMOTEPASSWD, @Optional("localhost") String DSHOSTNAME,
            @Optional("11810") String DSSVCNAME ) {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        username = REMOTEUSER;
        password = REMOTEPASSWD;
        coordUrl = HOSTNAME + ":" + SVCNAME;
        dsHostName = DSHOSTNAME;
        dsServiceName = DSSVCNAME;

        TimeZone.setDefault( TimeZone.getTimeZone( "Asia/Shanghai" ) );
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            boolean ret = createCommonCS( db );
            Assert.assertTrue( ret );
            File workDirFile = new File( workDir );
            if ( !workDirFile.exists() ) {
                workDirFile.mkdir();
            }
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @AfterSuite
    public static void finiSuite() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
        } finally {
            TimeZone.setDefault( defaultZone );
            if ( db != null ) {
                db.disconnect();
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
        } catch ( BaseException e ) {
            System.out.printf( "create CollectionSpace %s failed, errMsg:%s\n",
                    csName, e.getMessage() );
            isCreateSuccess = false;
        }
        return isCreateSuccess;
    }

}
