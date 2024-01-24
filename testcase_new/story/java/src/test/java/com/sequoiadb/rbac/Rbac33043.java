package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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
 * @Description seqDB-33043:findandmodify权限测试
 * @Author wangxingming
 * @Date 2023.09.01
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.09.01
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac33043 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user1 = "user_33043_1";
    private String user2 = "user_33043_2";
    private String user3 = "user_33043_3";
    private String password = "passwd_33043";
    private String RoleName1 = "subrole_33043_1";
    private String RoleName2 = "subrole_33043_2";
    private String RoleName3 = "subrole_33043_3";
    private String csName = "cs_33043";
    private String clName = "cl_33043";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        RbacUtils.removeUser( sdb, user1, password );
        RbacUtils.removeUser( sdb, user2, password );
        RbacUtils.removeUser( sdb, user3, password );
        RbacUtils.dropRole( sdb, RoleName1 );
        RbacUtils.dropRole( sdb, RoleName2 );
        RbacUtils.dropRole( sdb, RoleName3 );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( clName );
        cs.getCollection( clName ).insertRecord( new BasicBSONObject( "a", 3 )
                .append( "a", 3 ).append( "a", 3 ) );
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
            sdb.dropRole( RoleName1 );
            sdb.dropRole( RoleName2 );
            sdb.dropRole( RoleName3 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions1 = { "find", "update", };
        String[] actions2 = { "find", "remove" };
        String[] actions3 = { "find" };
        String Actions1 = RbacUtils.arrayToCommaSeparatedString( actions1 );
        String Actions2 = RbacUtils.arrayToCommaSeparatedString( actions2 );
        String Actions3 = RbacUtils.arrayToCommaSeparatedString( actions3 );

        // 创建多个角色，包含多种权限
        String RoleStr1 = "{Role:'" + RoleName1
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions1
                + ",'testCS', 'testCL'] }] }";
        String RoleStr2 = "{Role:'" + RoleName2
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions2
                + ",'testCS', 'testCL'] }] }";
        String RoleStr3 = "{Role:'" + RoleName3
                + "',Privileges:[{Resource:{ cs:'" + csName
                + "',cl:''}, Actions: [" + Actions3
                + ",'testCS', 'testCL'] }] }";

        BSONObject Role1 = ( BSONObject ) JSON.parse( RoleStr1 );
        BSONObject Role2 = ( BSONObject ) JSON.parse( RoleStr2 );
        BSONObject Role3 = ( BSONObject ) JSON.parse( RoleStr3 );
        sdb.createRole( Role1 );
        sdb.createRole( Role2 );
        sdb.createRole( Role3 );

        // 创建用户指定特定角色
        sdb.createUser( user1, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + RoleName1 + "']}" ) );
        sdb.createUser( user2, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + RoleName2 + "']}" ) );
        sdb.createUser( user3, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + RoleName3 + "']}" ) );

        // user1执行find().update()，预期成功，执行find().remove()，预期失败
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user1,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            userCL.queryAndUpdate( null, null, null, null, new BasicBSONObject(
                    "$set", new BasicBSONObject( "a", 1 ) ), 0, -1, 0, false );
            try {
                userCL.queryAndRemove( new BasicBSONObject( "a", 1 ), null,
                        null, null, -1, -1, 0 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        }

        // user2执行find().update()，预期失败，执行find().remove()，预期成功
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user2,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            userCL.queryAndRemove( new BasicBSONObject( "a", 1 ), null, null,
                    null, -1, -1, 0 );
            try {
                userCL.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 1 ) ),
                        0, -1, 0, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        }

        // user3执行find()，预期成功，执行find().remove()，预期失败
        sdb.getCollectionSpace( csName ).getCollection( clName )
                .insertRecord( new BasicBSONObject( "a", 3 ).append( "a", 3 )
                        .append( "a", 3 ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user3,
                password )) {
            DBCollection userCL = userSdb.getCollectionSpace( csName )
                    .getCollection( clName );
            userCL.query( null, null, null, null, -1, -1, 0 );
            try {
                userCL.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 1 ) ),
                        0, -1, 0, false );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
            try {
                userCL.queryAndRemove( new BasicBSONObject( "a", 3 ), null,
                        null, null, -1, -1, 0 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(),
                        SDBError.SDB_NO_PRIVILEGES.getErrorCode() );
            }
        }
    }
}
