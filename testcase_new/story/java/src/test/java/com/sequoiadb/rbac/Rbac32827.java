package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @Description seqDB-32827:父角色和子角色存在相同权限，删除父角色
 * @Author liuli
 * @Date 2023.08.24
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.24
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32827 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32827";
    private String password = "passwd_32827";
    private String mainRoleName = "mainrole_32827";
    private String subRoleName = "subrole_32827";
    private String csName = "cs_32827";
    private String clName = "cl_32827";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, mainRoleName );
        RbacUtils.dropRole( sdb, subRoleName );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
    }

    @Test
    public void test() throws Exception {
        // 创建父角色
        String mainRoleStr = "{Role:'" + mainRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['insert','update','remove'] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
        sdb.createRole( mainRole );

        // 创建子角色继承父角色，子角色和父角色有相同的权限
        String subRoleStr = "{Role:'" + subRoleName
                + "',Privileges:[{Resource:{ cs:'" + csName + "',cl:'" + clName
                + "'}, Actions: ['find','insert'] },{ Resource: { cs: '"
                + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] ,Roles:['"
                + mainRoleName + "'] }";
        BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
        sdb.createRole( subRole );

        // 创建用户
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + subRoleName + "']}" ) );

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

            // 删除父角色
            sdb.dropRole( mainRoleName );

            // 执行仅父角色支持的操作
            try {
                userCL.updateRecords( new BasicBSONObject( "a", 2 ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 3 ) ) );
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

            // 执行父角色和子角色都支持的操作
            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            // 执行仅子角色支持的操作
            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );
        } finally {
            sdb.removeUser( user, password );
            RbacUtils.dropRole( sdb, subRoleName );
            RbacUtils.dropRole( sdb, mainRoleName );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( csName );
            RbacUtils.dropRole( sdb, mainRoleName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}