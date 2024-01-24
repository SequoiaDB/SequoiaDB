package com.sequoiadb.testcommon;

import java.io.File;

import org.testng.Assert;
import org.testng.annotations.AfterSuite;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Parameters;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbTestBase {
    protected static String coordUrl;
    protected static String hostName;
    protected static String serviceName;
    protected static String csName;
    protected static int reservedPortBegin;
    protected static int reservedPortEnd;
    protected static String reservedDir;
    protected static String workDir;

    @SuppressWarnings("deprecation")
    @Parameters({ "HOSTNAME", "SVCNAME", "CHANGEDPREFIX", "RSRVPORTBEGIN",
            "RSRVPORTEND", "RSRVNODEDIR", "WORKDIR" })
    @BeforeSuite
    public static void initSuite( String HOSTNAME, String SVCNAME,
            String COMMCSNAME, int RSRVPORTBEGIN, int RSRVPORTEND,
            String RSRVNODEDIR, String WORKDIR ) {
        hostName = HOSTNAME;
        serviceName = SVCNAME;
        csName = COMMCSNAME;
        reservedPortBegin = RSRVPORTBEGIN;
        reservedPortEnd = RSRVPORTEND;
        reservedDir = RSRVNODEDIR;
        workDir = WORKDIR;
        coordUrl = HOSTNAME + ":" + SVCNAME;

        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            boolean ret = createCommonCS( db );
            Assert.assertTrue( ret );
            File workDirFile = new File( workDir );
            if ( !workDirFile.exists() ) {
                workDirFile.mkdir();
            }
        } catch ( BaseException e ) {
            Assert.fail( "connect " + coordUrl + ": " + e.getErrorCode() );
        } finally {
            if ( db != null ) {
                db.disconnect();
            }
        }
    }

    @SuppressWarnings({ "resource", "deprecation" })
    @AfterSuite(alwaysRun = true)
    public static void finiSuite() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            throw e;
        } finally {
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
