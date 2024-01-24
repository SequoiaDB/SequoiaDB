package com.sequoiadb.rbac;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32779:创建角色指定Resource包含主表，不包含子表
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32779 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32779";
    private String password = "passwd_32779";
    private String roleName = "role_32779";
    private String csName = "cs_32779";
    private String mainCLName = "maincl_32779";
    private String subCLName = "subcl_32779";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        RbacUtils.dropRole( sdb, roleName );

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        optionsM.put( "ShardingType", "range" );
        cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "a", 1 ) ) );
    }

    @Test
    public void test() throws Exception {
        testAttachCLAction( sdb );
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

    private void testAttachCLAction( Sequoiadb sdb ) {
        // 子表不具备部分权限
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate", "testCL" };
        // 子表随机取3个action
        String[] randomActions = RbacUtils.getRandomActions( actions, 3 );
        String subAction = RbacUtils
                .arrayToCommaSeparatedString( randomActions );
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + mainCLName
                + "'}, Actions: ['attachCL','find'] }" + ",{Resource:{ cs:'"
                + csName + "',cl:'" + subCLName + "'}, Actions: [" + subAction
                + "] }" + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userMainCL = userSdb.getCollectionSpace( csName )
                    .getCollection( mainCLName );

            BasicBSONObject optionS = new BasicBSONObject();
            optionS.put( "LowBound", new BasicBSONObject( "a", 0 ) );
            optionS.put( "UpBound", new BasicBSONObject( "a", 10000 ) );
            try {
                userMainCL.attachCollection( csName + "." + subCLName,
                        optionS );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
            sdb.dropRole( roleName );
        }
    }
}