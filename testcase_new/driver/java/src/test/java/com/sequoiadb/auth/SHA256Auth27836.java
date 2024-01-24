package com.sequoiadb.auth;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Random;


/**
 * @Description: seqDB-27836:SCRAM-SHA256鉴权测试
 * @author Chen wenjia
 * @Date:2022.09.22
 */

public class SHA256Auth27836 extends SdbTestBase {

    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String csName = "cs_27836";
    private String clName = "cl_27836";
    private String coordAddr;
    private StringBuilder strBuilder;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.sdb = new Sequoiadb( this.coordAddr, "", "" );
        if ( !Util.isCluster( this.sdb ) ) {
            throw new SkipException( "Skip StandAlone" );
        }
        if ( this.sdb.isCollectionSpaceExist( this.csName ) ) {
            this.sdb.dropCollectionSpace( this.csName );
        }
        this.cs = this.sdb.createCollectionSpace( this.csName );
        if ( this.cs.isCollectionExist( this.clName ) ) {
            this.cs.dropCollection( this.clName );
        }
        this.cl = this.cs.createCollection( this.clName );

        strBuilder = new StringBuilder()
                .append( "abcdefghijklmnopqrstuvwxyz" )
                .append( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
                .append( "123456789" )
                .append( "~!@#$%^&*()_+{}|:\"<>?`-=[]\\;',./~！@#￥%……&*（）{}|：“《》？·-=【】、；‘，。/" )
                .append( "中文测试字符，鉴权测试" );
    }

    @Test
    public void authWithoutUserTest() {
        //case1:empty string or null
        authCheck( "", "" );
        authCheck( null, null );

        //case2:letter,number,special character and chinese
        for ( int i = 0; i < 100; i++ ) {
            String username = generateString();
            String password = generateString();
            authCheck( username, password );
        }

        //case3:over 256 character
        StringBuilder text = new StringBuilder();
        for ( int i = 0; i < 257; i++ ) {
            text.append( "a" );
        }
        authCheck( text.toString(), text.toString() );
    }

    @Test
    public void authWithUserTest() {
        //case1:password is empty string
        authCheckWithUser( "emptyPassword", "" );

        //case2:letter,number,special character and chinese
        for ( int i = 0; i < 100; i++ ) {
            String username = generateString();
            String password = generateString();
            authCheckWithUser( username, password );
        }

        //case3:create user success when over 256 character, bug:SEQUOIADBMAINSTREAM-8804
    }

    @Test
    public void authFailTest() {
        Sequoiadb errorConn = null;
        String str = "test";
        this.sdb.createUser( str, str );
        try {
            errorConn = new Sequoiadb( this.coordAddr, str, "errorPassword" );
            Assert.fail( "Auth should be fail" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode(), e.getErrorCode() );
        } finally {
            this.sdb.removeUser( str, str );
        }
    }

    private void authCheckWithUser(String username, String password ) {
        this.sdb.createUser( username, password );
        try {
            authCheck( username, password );
        } finally {
            this.sdb.removeUser( username, password );
        }
    }

    private void authCheck(String username, String password ) {
        BSONObject obj = new BasicBSONObject();
        obj.put( "key","val" );
        try ( Sequoiadb db = new Sequoiadb( this.coordAddr, username, password ); ) {
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            cl.insertRecord( obj );
        }
    }

    private String generateString() {
        Random random = new Random();
        StringBuilder text = new StringBuilder();
        //generate 1-255 character
        int len = random.nextInt( 255 ) + 1;
        for ( int i = 0; i < len; i++ ) {
            int post = random.nextInt( strBuilder.length() );
            text.append( strBuilder.charAt( post ) );
        }
        return text.toString();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.sdb.isCollectionSpaceExist( csName ) ) {
                this.sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }
}
