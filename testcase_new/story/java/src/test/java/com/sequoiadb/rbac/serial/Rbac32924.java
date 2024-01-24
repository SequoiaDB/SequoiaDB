package com.sequoiadb.rbac.serial;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.rbac.RbacUtils;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32924:创建角色Action指定analyze
 * @Author tangtao
 * @Date 2023.08.30
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.30
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32924 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user1 = "user_32924_1";
    private String user2 = "user_32924_2";
    private String user3 = "user_32924_3";
    private String password = "passwd_32924";
    private String roleName1 = "role_32924_1";
    private String roleName2 = "role_32924_2";
    private String roleName3 = "role_32924_3";
    private String csName = "cs_32924";
    private String clName = "cl_32924";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        RbacUtils.dropRole( sdb, roleName3 );

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
            sdb.removeUser( user1, password );
            sdb.removeUser( user2, password );
            sdb.removeUser( user3, password );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
            RbacUtils.dropRole( sdb, roleName3 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {

        // 创建角色
        String roleStr = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['analyze'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        roleStr = "{Role:'" + roleName2 + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:''}, Actions: ['analyze'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        roleStr = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['analyze'] }] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role3 );

        // 使用角色创建用户
        sdb.createUser( user1, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName1 + "']}" ) );
        Sequoiadb userSdb1 = new Sequoiadb( SdbTestBase.coordUrl, user1,
                password );

        sdb.createUser( user2, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName2 + "']}" ) );
        Sequoiadb userSdb2 = new Sequoiadb( SdbTestBase.coordUrl, user2,
                password );

        sdb.createUser( user3, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName3 + "']}" ) );
        Sequoiadb userSdb3 = new Sequoiadb( SdbTestBase.coordUrl, user3,
                password );

        userSdb1.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        try {
            userSdb1.analyze(
                    new BasicBSONObject( "CollectionSpace", csName ) );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                    .getErrorCode() ) {
                throw e;
            }
        }
        try {
            userSdb1.analyze();
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                    .getErrorCode() ) {
                throw e;
            }
        }

        userSdb2.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        userSdb2.analyze( new BasicBSONObject( "CollectionSpace", csName ) );
        try {
            userSdb2.analyze();
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                    .getErrorCode() ) {
                throw e;
            }
        }

        userSdb3.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        userSdb3.analyze( new BasicBSONObject( "CollectionSpace", csName ) );
        userSdb3.analyze();

        userSdb1.close();
        userSdb2.close();
        userSdb3.close();
    }
}