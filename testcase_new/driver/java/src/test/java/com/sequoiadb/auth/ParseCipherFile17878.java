package com.sequoiadb.auth;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.util.SdbDecrypt;
import com.sequoiadb.util.SdbDecryptUserInfo;

/**
 * @Description: seqDB-17878:带token,单个cluster，密码文件包含每个cluster单个和多个用户
 *               本用例只涉及单个集群单个用户，只进行简单的功能验证
 * @author fanyu
 * @Date:2019年02月22日
 * @version:1.0
 */

public class ParseCipherFile17878 extends SdbTestBase {
    private Sequoiadb sdb;
    private String username = "user17878";
    private String password = "17878";
    private String token = "123456";
    private String passwdFileName = "/password17878";
    private String passwordFilePath;

    @BeforeClass(alwaysRun = true)
    private void setUp() throws Exception {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        try {
            sdb.removeUser( username, password );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -300 ) {
                throw e;
            }
        }
        sdb.createUser( username, password );
        // get sdb tools path
        Util.createPasswdFile( username, password, passwdFileName, token );
        String toolsPath = Util.getSdbInstallDir() + "/bin/";
        Util.downLoadFileToLocal( SdbTestBase.workDir,
                toolsPath + passwdFileName );
        passwordFilePath = SdbTestBase.workDir + passwdFileName;
    }

    @Test
    private void testParseCipherFile() throws Exception {
        SdbDecrypt sdbDecrypt = new SdbDecrypt();
        SdbDecryptUserInfo info = sdbDecrypt.parseCipherFile( username, token,
                new File( passwordFilePath ) );
        // check
        Assert.assertEquals( info.getUserName(), username, info.toString() );
        Assert.assertEquals( info.getPasswd(), password );
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, info.getUserName(),
                info.getPasswd() );
        db.close();
    }

    @Test
    private void testDecrypPasswd() throws Exception {
        // get encryptPasswd form passwordFile
        BufferedReader br = null;
        String encryptPasswd;
        try {
            br = new BufferedReader( new FileReader( passwordFilePath ) );
            String info = br.readLine();
            encryptPasswd = info.substring( info.lastIndexOf( ":" ) + 1,
                    info.length() );
        } finally {
            if ( br != null ) {
                br.close();
            }
        }
        SdbDecrypt sdbDecrypt = new SdbDecrypt();
        String actPassword = sdbDecrypt.decryptPasswd( encryptPasswd, token );
        // check
        Assert.assertEquals( actPassword, password );
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, username,
                actPassword );
        db.close();
    }

    @AfterClass(alwaysRun = true)
    private void tearDown() throws Exception {
        try {
            sdb.removeUser( username, password );
            new File( passwordFilePath ).deleteOnExit();
            Util.removePasswdFile(
                    Util.getSdbInstallDir() + "/bin" + passwdFileName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
