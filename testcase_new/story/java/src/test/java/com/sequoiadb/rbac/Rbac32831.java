package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32831:存在多层继承关系，删除中间父角色
 * @Author liuli
 * @Date 2023.08.24
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.24
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32831 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32831";
    private String password = "passwd_32831";
    private String roleName1 = "role_32831_1";
    private String roleName2 = "role_32831_2";
    private String roleName3 = "role_32831_3";
    private String csName = "cs_32831";
    private String clName = "cl_32831";

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
        // 创建角色role1
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert','remove'] }" + ",{ Resource: { cs: '"
                + csName + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 创建角色role2继承角色role1
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['update'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] ,Roles:['"
                + roleName1 + "'] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 创建角色role3继承角色role2
        String roleStr3 = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find'] },{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] ,Roles:['"
                + roleName2 + "'] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr3 );
        sdb.createRole( role3 );

        // 创建用户
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName3 + "']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            // 执行支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );
            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 删除角色role2
            sdb.dropRole( roleName2 );

            // 校验角色信息
            BSONObject roleInfo = sdb.getRole( roleName3,
                    new BasicBSONObject() );
            BasicBSONList roles = ( BasicBSONList ) roleInfo.get( "Roles" );
            BasicBSONList expRoles = new BasicBSONList();
            Assert.assertEquals( roles, expRoles );

            BasicBSONList inheritedRoles = ( BasicBSONList ) roleInfo
                    .get( "InheritedRoles" );
            BasicBSONList expInheritedRoles = new BasicBSONList();
            Assert.assertEquals( inheritedRoles, expInheritedRoles );

            // 执行角色role1支持的操作
            try {
                userCL.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
            try {
                userCL.deleteRecords( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行角色role2支持的操作
            try {
                userCL.updateRecords( null, new BasicBSONObject( "$set",
                        new BasicBSONObject( "a", 2 ) ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            // 执行角色role2和role3支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    true );
        } finally {
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, roleName1 );
            RbacUtils.dropRole( sdb, roleName2 );
            RbacUtils.dropRole( sdb, roleName3 );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}