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
 * @Description seqDB-32780:创建角色指定Resource包含子表，不包含主表
 * @Author liuli
 * @Date 2023.08.16
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.16
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32780 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32780";
    private String password = "passwd_32780";
    private String roleName = "role_32780";
    private String csName = "cs_32780";
    private String mainCLName = "maincl_32780";
    private String subCLName = "subcl_32780";

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
        testdetachCLAction( sdb );
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
        // 主表不具备权限
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + subCLName
                + "'}, Actions: ['attachCL','find','insert','update','remove'] }"
                + ",{ Resource: { cs: '" + csName
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

    private void testdetachCLAction( Sequoiadb sdb ) {
        // 子表具有detachCL权限，主表不具备detachCL权限
        String roleStr = "{Role:'" + roleName + "',Privileges:[{Resource:{ cs:'"
                + csName + "',cl:'" + subCLName + "'}, Actions: ['detachCL'] }"
                + ",{ Resource: { cs: '" + csName
                + "', cl: '' }, Actions: ['testCS','testCL'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['" + roleName + "']}" ) );

        DBCollection rootMainCL = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        BasicBSONObject optionS = new BasicBSONObject();
        optionS.put( "LowBound", new BasicBSONObject( "a", 0 ) );
        optionS.put( "UpBound", new BasicBSONObject( "a", 10000 ) );
        rootMainCL.attachCollection( csName + "." + subCLName, optionS );
        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            DBCollection userMainCL = userSdb.getCollectionSpace( csName )
                    .getCollection( mainCLName );
            try {
                userMainCL.detachCollection( csName + "." + subCLName );
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