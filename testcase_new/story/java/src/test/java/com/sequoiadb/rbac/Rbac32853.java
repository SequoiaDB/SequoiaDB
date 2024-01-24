package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32853:角色添加继承关系，子角色和父角色没有交集
 * @Author tangtao
 * @Date 2023.08.30
 * @UpdateAuthor tangtao
 * @UpdateDate 2023.08.30
 * @version 1.0
 */
@Test(groups = "rbac")
public class Rbac32853 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32853";
    private String password = "passwd_32853";
    private String hierarchicRoleName1 = "role_32853_1";
    private String hierarchicRoleName2 = "role_32853_2";
    private String csName = "cs_32853";
    private String clName = "cl_32853";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.dropRole( sdb, hierarchicRoleName1 );
        RbacUtils.dropRole( sdb, hierarchicRoleName2 );

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
            RbacUtils.dropRole( sdb, hierarchicRoleName1 );
            RbacUtils.dropRole( sdb, hierarchicRoleName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "find", "insert", "update", "remove" };
        String[] actions2 = { "testCS", "testCL", "createIndex", "dropIndex" };

        // 创建父角色
        String action = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String roleStr = "{Role:'" + hierarchicRoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role1 );

        // 创建子角色
        action = RbacUtils.arrayToCommaSeparatedString( actions2 );
        roleStr = "{Role:'" + hierarchicRoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + action + "] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role2 );

        // 使用子角色创建用户
        sdb.createUser( user, password, ( BSONObject ) JSON
                .parse( "{Roles:['" + hierarchicRoleName2 + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {

            BasicBSONList roleList = new BasicBSONList();
            roleList.add( hierarchicRoleName1 );
            sdb.grantRolesToRole( hierarchicRoleName2, roleList );

            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );

            RbacUtils.findActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.insertActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.updateActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.removeActionSupportCommand( sdb, csName, clName, userCL,
                    false );

            RbacUtils.createIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

            RbacUtils.dropIndexActionSupportCommand( sdb, csName, clName,
                    userCL, false );

        }
    }
}