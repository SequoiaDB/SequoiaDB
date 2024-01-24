package com.sequoiadb.auth;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.Ssh;

public class Util extends SdbTestBase {
    public static boolean isCluster( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            int errno = e.getErrorCode();
            if ( new BaseException( SDBError.SDB_RTN_COORD_ONLY )
                    .getErrorCode() == errno ) {
                System.out.println(
                        "This test is for cluster environment only." );
                return false;
            }
        }
        return true;
    }

    public static void createPasswdFile( String username, String password,
            String passwordFile ) throws Exception {
        createPasswdFile( username, password, passwordFile, null );
    }

    public static void createPasswdFile( String username, String password,
            String passwordFile, String token ) throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( SdbTestBase.hostName, SdbTestBase.username,
                    SdbTestBase.password );
            String toolsPath = Util.getSdbInstallDir() + "/bin/";
            String passwdFilePath = toolsPath + passwordFile;
            String cmd = toolsPath + "sdbpasswd  -a \"" + username + "\" -p \""
                    + password + "\" -f \"" + passwdFilePath + "\"";
            if ( token != null ) {
                cmd += " -t \"" + token + "\"";
            }
            ssh.exec( cmd );
        } finally {
            if ( null != ssh ) {
                ssh.disconnect();
            }
        }
    }

    public static void downLoadFileToLocal( String localPath,
            String remotePath ) throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( SdbTestBase.hostName, SdbTestBase.username,
                    SdbTestBase.password );
            ssh.scpFrom( localPath, remotePath );
        } finally {
            if ( null != ssh ) {
                ssh.disconnect();
            }
        }
    }

    public static String getSdbInstallDir() throws Exception {
        Ssh ssh = new Ssh( SdbTestBase.hostName, SdbTestBase.username,
                SdbTestBase.password );
        String dir = null;
        try {
            ssh.exec( "cat /etc/default/sequoiadb |grep INSTALL_DIR" );
            String str = ssh.getStdout();
            if ( str.length() <= 0 ) {
                throw new Exception(
                        "exec command:cat /etc/default/sequoiadb |grep INSTALL_DIR can not find sequoiadb install dir" );
            }
            dir = str.substring( str.indexOf( "=" ) + 1, str.length() - 1 );
        } finally {
            ssh.disconnect();
        }
        return dir;
    }

    public static void removePasswdFile( String passwordFilePath )
            throws Exception {
        Ssh ssh = new Ssh( SdbTestBase.hostName, SdbTestBase.username,
                SdbTestBase.password );
        try {
            ssh.exec( " rm -rf  " + passwordFilePath );
        } finally {
            ssh.disconnect();
        }
    }
}
