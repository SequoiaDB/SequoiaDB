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

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32838:更新角色继承关系，两个角色形成闭环 seqDB-32839:更新角色继承关系，多个角色形成闭环
 * @Author liuli
 * @Date 2023.08.25
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.25
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32838_32839 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String roleName1 = "role_32838_1";
    private String roleName2 = "role_32838_2";
    private String roleName3 = "role_32838_3";

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
    }

    @Test
    public void test() throws Exception {
        // 创建角色role1
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        // 创建角色role2继承角色role1
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['insert','createCL'] }],Roles:['"
                + roleName1 + "'] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 更新角色信息，更新role1继承角色role2
        String updateRoleStr1 = "{Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }],Roles:['"
                + roleName2 + "'] }";
        BSONObject updateRole1 = ( BSONObject ) JSON.parse( updateRoleStr1 );
        try {
            sdb.updateRole( roleName1, updateRole1 );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 检查角色继承关系
        BSONObject roleInfo1 = sdb.getRole( roleName1,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList actRoles1 = ( BasicBSONList ) roleInfo1.get( "Roles" );
        Assert.assertNull( actRoles1 );

        BSONObject roleInfo2 = sdb.getRole( roleName2,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList actRoles2 = ( BasicBSONList ) roleInfo2.get( "Roles" );
        BasicBSONList expRoles2 = new BasicBSONList();
        expRoles2.add( roleName1 );
        Assert.assertEquals( actRoles2, expRoles2 );

        // 创建角色role3继承角色role2
        String roleStr3 = "{Role:'" + roleName3
                + "',Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['insert','createCL'] }],Roles:['"
                + roleName2 + "'] }";
        BSONObject role3 = ( BSONObject ) JSON.parse( roleStr3 );
        sdb.createRole( role3 );

        // 更新角色信息，更新role1继承角色role3
        updateRoleStr1 = "{Privileges:[{Resource:{ cs:'',cl:''}, Actions: ['find'] }"
                + ",{ Resource: { cs: '', cl: '' }, Actions: ['testCS','testCL'] }],Roles:['"
                + roleName3 + "'] }";
        updateRole1 = ( BSONObject ) JSON.parse( updateRoleStr1 );
        try {
            sdb.updateRole( roleName1, updateRole1 );
            Assert.fail( "should error but success" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_CYCLE_DETECTED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 检查角色继承关系
        roleInfo1 = sdb.getRole( roleName1,
                new BasicBSONObject( "ShowPrivileges", true ) );
        actRoles1 = ( BasicBSONList ) roleInfo1.get( "Roles" );
        Assert.assertNull( actRoles1 );

        roleInfo2 = sdb.getRole( roleName2,
                new BasicBSONObject( "ShowPrivileges", true ) );
        actRoles2 = ( BasicBSONList ) roleInfo2.get( "Roles" );
        expRoles2 = new BasicBSONList();
        expRoles2.add( roleName1 );
        Assert.assertEquals( actRoles2, expRoles2 );
        BasicBSONList actInheritedRoles2 = ( BasicBSONList ) roleInfo2
                .get( "InheritedRoles" );
        BasicBSONList expInheritedRoles2 = new BasicBSONList();
        expInheritedRoles2.add( roleName1 );
        Assert.assertEquals( actInheritedRoles2, expInheritedRoles2 );

        BSONObject roleInfo3 = sdb.getRole( roleName3,
                new BasicBSONObject( "ShowPrivileges", true ) );
        BasicBSONList actRoles3 = ( BasicBSONList ) roleInfo3.get( "Roles" );
        BasicBSONList expRoles3 = new BasicBSONList();
        expRoles3.add( roleName2 );
        Assert.assertEquals( actRoles3, expRoles3 );
        BasicBSONList actInheritedRoles3 = ( BasicBSONList ) roleInfo3
                .get( "InheritedRoles" );
        BasicBSONList expInheritedRoles3 = new BasicBSONList();
        expInheritedRoles3.add( roleName1 );
        expInheritedRoles3.add( roleName2 );
        if ( !RbacUtils.compareBSONListsIgnoreOrder( actInheritedRoles3,
                expInheritedRoles3 ) ) {
            Assert.fail( "InheritedRoles not equal,actInheritedRoles:"
                    + actInheritedRoles3 + ",expInheritedRoles:"
                    + expInheritedRoles3 );
        }
    }

    @AfterClass
    public void tearDown() {
        RbacUtils.dropRole( sdb, roleName1 );
        RbacUtils.dropRole( sdb, roleName2 );
        RbacUtils.dropRole( sdb, roleName3 );
        if ( sdb != null ) {
            sdb.close();
        }
    }

}