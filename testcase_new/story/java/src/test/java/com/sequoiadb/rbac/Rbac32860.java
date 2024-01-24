package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-32860:getUser获取用户信息
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32860 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32860";
    private String password = "passwd_32860";
    private String csName = "cs_32860";
    private String clName = "cl_32860";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.removeUser( sdb, user, password );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            sdb.removeUser( user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String roleName = "_" + csName + ".readWrite";
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        // 获取用户信息，指定用户不存在
        try {
            sdb.getUser( "notExistUser",
                    new BasicBSONObject( "ShowPrivileges", true ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_AUTH_USER_NOT_EXIST.getErrorCode() );
        }

        // 获取用户信息，不指定options
        BSONObject user2 = sdb.getUser( user, new BasicBSONObject() );
        Assert.assertEquals( JSON.serialize( user2.get( "InheritedRoles" ) ),
                " " + null + " " );

        // 获取用户信息，指定ShowPrivileges为false
        BSONObject user3 = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", false ) );
        Assert.assertEquals( JSON.serialize( user3.get( "InheritedRoles" ) ),
                " " + null + " " );

        // 获取用户信息，指定ShowPrivileges为true
        BSONObject user4 = sdb.getUser( user,
                new BasicBSONObject( "ShowPrivileges", true ) );
        Assert.assertEquals( JSON.serialize( user4.get( "InheritedRoles" ) ),
                "[ \"_cs_32860.readWrite\" ]" );
        String inheritedPrivileges = JSON
                .serialize( user4.get( "InheritedPrivileges" ) );
        Assert.assertTrue( inheritedPrivileges.contains( "find" ) );
        Assert.assertTrue( inheritedPrivileges.contains( "insert" ) );
        Assert.assertTrue( inheritedPrivileges.contains( "update" ) );
        Assert.assertTrue( inheritedPrivileges.contains( "remove" ) );
        Assert.assertTrue( inheritedPrivileges.contains( "getDetail" ) );
    }
}
