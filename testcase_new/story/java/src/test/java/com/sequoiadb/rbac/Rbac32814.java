package com.sequoiadb.rbac;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description seqDB-32814:创建用户使用内建角色，指定为_dbAdmin
 * @Author liuli
 * @Date 2023.08.30
 * @UpdateAuthor liuli
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32814 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32814";
    private String password = "passwd_32814";
    private String mainCSName = "maincs_32814";
    private String csName1 = "cs_32814_1";
    private String csName2 = "cs_32814_2";
    private String clName1 = "cl_32814_1";
    private String clName2 = "cl_32814_2";
    private String mainCLName = "maincl_32777";
    private String subCLName = "subcl_32777";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, SdbTestBase.rootUserName,
                SdbTestBase.rootUserPassword );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        // 创建主表、子表和普通表
        if ( sdb.isCollectionSpaceExist( mainCSName ) ) {
            sdb.dropCollectionSpace( mainCSName );
        }
        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }

        CollectionSpace maincs = sdb.createCollectionSpace( mainCSName );
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        optionsM.put( "ShardingType", "range" );
        optionsM.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection maincl = maincs.createCollection( mainCLName, optionsM );

        maincs.createCollection( subCLName, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "date", 1 ) ) );

        BasicBSONObject optionS = new BasicBSONObject();
        optionS.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        optionS.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        maincl.attachCollection( mainCSName + "." + subCLName, optionS );

        CollectionSpace cs1 = sdb.createCollectionSpace( csName1 );
        cs1.createCollection( clName1 );

        CollectionSpace cs2 = sdb.createCollectionSpace( csName2 );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "AutoSplit", true );
        cs2.createCollection( clName2, options );
    }

    @Test
    public void test() throws Exception {
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            RbacUtils.removeUser( sdb, user, password );
            sdb.dropCollectionSpace( mainCSName );
            sdb.dropCollectionSpace( csName1 );
            sdb.dropCollectionSpace( csName2 );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_dbAdmin']}" ) );

        try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                password )) {
            CollectionSpace mainCS = userSdb.getCollectionSpace( mainCSName );
            DBCollection mainCL = mainCS.getCollection( mainCLName );
            DBCollection subCL = mainCS.getCollection( subCLName );
            CollectionSpace userCS1 = userSdb.getCollectionSpace( csName1 );
            DBCollection userCL1 = userCS1.getCollection( clName1 );
            DBCollection userCL2 = userSdb.getCollectionSpace( csName2 )
                    .getCollection( clName2 );

            RbacUtils.findActionSupportCommand( sdb, csName1, clName1, userCL1,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.updateActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.removeActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.getDetailActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.createIndexActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.alterCLActionSupportCommand( sdb, csName1, clName1,
                    userCL1, false );
            RbacUtils.createCLActionSupportCommand( sdb, csName1, clName1,
                    userCS1, false );
            RbacUtils.dropCLActionSupportCommand( sdb, csName1, clName1,
                    userCS1, false );
            RbacUtils.renameCLActionSupportCommand( sdb, csName1, clName1,
                    userCS1, false );
            userCL1.truncate();

            RbacUtils.findActionSupportCommand( sdb, csName2, clName2, userCL2,
                    false );
            RbacUtils.insertActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.updateActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.removeActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.getDetailActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.createIndexActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, csName2, clName2,
                    userCL2, false );
            userCL2.truncate();

            RbacUtils.findActionSupportCommand( sdb, mainCSName, mainCLName,
                    mainCL, false );
            RbacUtils.insertActionSupportCommand( sdb, mainCSName, mainCLName,
                    mainCL, false );
            RbacUtils.updateActionSupportCommand( sdb, mainCSName, mainCLName,
                    mainCL, false );
            RbacUtils.removeActionSupportCommand( sdb, mainCSName, mainCLName,
                    mainCL, false );
            RbacUtils.getDetailActionSupportCommand( sdb, mainCSName,
                    mainCLName, mainCL, false );
            RbacUtils.createIndexActionSupportCommand( sdb, mainCSName,
                    mainCLName, mainCL, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, mainCSName,
                    mainCLName, mainCL, false );
            mainCL.truncate();
            mainCS.getCollectionNames();

            RbacUtils.findActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            RbacUtils.insertActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            RbacUtils.updateActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            RbacUtils.removeActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            RbacUtils.getDetailActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            RbacUtils.createIndexActionSupportCommand( sdb, mainCSName,
                    subCLName, subCL, false );
            RbacUtils.dropIndexActionSupportCommand( sdb, mainCSName, subCLName,
                    subCL, false );
            subCL.truncate();
            try {
                DBCursor cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_DATABASE, new BasicBSONObject(),
                        null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        } finally {
            sdb.removeUser( user, password );
        }
    }
}