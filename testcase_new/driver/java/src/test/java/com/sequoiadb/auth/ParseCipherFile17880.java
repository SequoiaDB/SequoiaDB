package com.sequoiadb.auth;

import java.io.File;
import java.util.Random;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.util.SdbDecrypt;
import com.sequoiadb.util.SdbDecryptUserInfo;

/**
 * @Description: seqDB-17880:parseCipherFile参数检验,此用例密码文件中包含多个用户
 * @author fanyu
 * @Date:2019年02月22日
 * @version:1.0
 */
public class ParseCipherFile17880 extends SdbTestBase {
    private Sequoiadb sdb;
    private String[] usernames = {
            /* "user@17880","user:17880", */"  user 17880 ", "测试17880",
            "token17880A", "token17880B", "token17880C" };
    private String[] passwords = {
            /* "user@17880","user:17880", */"  user 17880 ", "测试17880",
            "token17880A", "token17880B", "token17880C" };
    private String passwdFileName = "/password17880";
    private String passwordFilePath;
    private String[] tokens = { getRandomString( 256 ), " ", "测试" };

    @BeforeClass(alwaysRun = true)
    private void setUp() throws Exception {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        cleanEnvAndPrepare();
    }

    @DataProvider(name = "range-provider")
    public Object[][] generateRangData() throws Exception {
        return new Object[][] { { usernames[ 0 ], passwords[ 0 ], null },
                { usernames[ 1 ], passwords[ 1 ], null },
                { usernames[ 2 ], passwords[ 2 ], tokens[ 0 ] },
                { usernames[ 3 ], passwords[ 3 ], tokens[ 1 ] },
                { usernames[ 4 ], passwords[ 4 ], tokens[ 2 ] } };
    }

    @DataProvider(name = "Invalidrange-provider")
    public Object[][] generateInvalidRangData() throws Exception {
        return new Object[][] { { null, new File( SdbTestBase.workDir ) },
                { usernames[ 0 ], null },
                { usernames[ 0 ],
                        new File( SdbTestBase.workDir + "/inexistence" ) },
                { usernames[ 0 ], new File( SdbTestBase.workDir ) } };
    }

    @Test(dataProvider = "Invalidrange-provider")
    private void testInvalid( String username, File passwordFile )
            throws Exception {
        SdbDecrypt sdbDecrypt = new SdbDecrypt();
        try {
            sdbDecrypt.parseCipherFile( username, passwordFile );
            Assert.fail( "exp fail but act success,username = " + username
                    + ",passwordFilePath = " + passwordFilePath );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    @Test(dataProvider = "range-provider")
    private void testParseCipherFile( String username, String password,
            String token ) throws Exception {
        SdbDecrypt sdbDecrypt = new SdbDecrypt();
        SdbDecryptUserInfo info = sdbDecrypt.parseCipherFile( username, token,
                new File( passwordFilePath ) );
        // check
        Assert.assertEquals( info.getUserName(), username, info.toString() );
        Assert.assertEquals( info.getPasswd(), password );
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, info.getUserName(),
                info.getPasswd() );
        Assert.assertFalse(
                db.isCollectionSpaceExist( "ParseCipherFile17880" ) );
        db.close();
    }

    @AfterClass(alwaysRun = true)
    private void tearDown() throws Exception {
        try {
            for ( int i = 0; i < usernames.length; i++ ) {
                sdb.removeUser( usernames[ i ], passwords[ i ] );
            }
            new File( passwordFilePath ).deleteOnExit();
            Util.removePasswdFile(
                    Util.getSdbInstallDir() + "/bin" + passwdFileName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void cleanEnvAndPrepare() throws Exception {
        for ( int i = 0; i < usernames.length; i++ ) {
            try {
                sdb.removeUser( usernames[ i ], passwords[ i ] );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -300 ) {
                    throw e;
                }
            }
            sdb.createUser( usernames[ i ], passwords[ i ] );
        }
        Util.createPasswdFile( usernames[ 0 ], passwords[ 0 ], passwdFileName );
        Util.createPasswdFile( usernames[ 1 ], passwords[ 1 ], passwdFileName );
        Util.createPasswdFile( usernames[ 2 ], passwords[ 2 ], passwdFileName,
                tokens[ 0 ] );
        Util.createPasswdFile( usernames[ 3 ], passwords[ 3 ], passwdFileName,
                tokens[ 1 ] );
        Util.createPasswdFile( usernames[ 4 ], passwords[ 4 ], passwdFileName,
                tokens[ 2 ] );
        String toolsPath = Util.getSdbInstallDir() + "/bin/";
        Util.downLoadFileToLocal( SdbTestBase.workDir,
                toolsPath + passwdFileName );
        passwordFilePath = SdbTestBase.workDir + passwdFileName;
    }

    private String getRandomString( int length ) {
        String str = "adcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( str.length() );
            sb.append( str.charAt( number ) );
        }
        return sb.toString();
    }
}
