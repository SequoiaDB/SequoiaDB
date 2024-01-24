package com.sequoiadb.rbac;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * @Description seqDB-32803:创建角色指定继承关系，子角色包含父角色
 * @Author wangxingming
 * @Date 2023.08.30
 * @UpdateAuthor wangxingming
 * @UpdateDate 2023.08.30
 * @version 1.10
 */
@Test(groups = "rbac")
public class Rbac32803 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String user = "user_32803";
    private String password = "passwd_32803";
    private String mainRoleName = "mainrole_32803";
    private String subRoleName = "subrole_32803";
    private String csName = "cs_32803";
    private String clName = "cl_32803";

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
        testAccessControl( sdb );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
            RbacUtils.removeUser( sdb, user, password );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void testAccessControl( Sequoiadb sdb ) {
        String[] actions = { "find", "insert", "update", "remove", "getDetail",
                "alterCL", "createIndex", "dropIndex", "truncate" };

        for ( String action1 : actions ) {
            String[] newActions = desRemoveAction( actions, action1 );
            String[] randomActions = RbacUtils.getRandomActions( newActions,
                    2 );
            String strActions = RbacUtils
                    .arrayToCommaSeparatedString( randomActions );

            // 创建父角色具有testCS和testCL权限
            String mainRoleStr = "{Role:'" + mainRoleName
                    + "',Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: [" + strActions + "] }] }";
            BSONObject mainRole = ( BSONObject ) JSON.parse( mainRoleStr );
            sdb.createRole( mainRole );

            // 创建子角色权限包含在父角色所有权限
            String subRoleStr = "{Role:'" + subRoleName
                    + "',Privileges:[{Resource:{ cs:'" + csName
                    + "',cl:''}, Actions: [" + strActions + " ,'" + action1
                    + "', 'testCS', 'testCL'] }] ,Roles:['" + mainRoleName
                    + "'] }";
            BSONObject subRole = ( BSONObject ) JSON.parse( subRoleStr );
            sdb.createRole( subRole );
            sdb.createUser( user, password, ( BSONObject ) JSON
                    .parse( "{Roles:['" + subRoleName + "']}" ) );
            try ( Sequoiadb userSdb = new Sequoiadb( SdbTestBase.coordUrl, user,
                    password )) {
                DBCollection userCL = userSdb.getCollectionSpace( csName )
                        .getCollection( clName );
                // 添加子角色的所有权限到集合
                ArrayList< String > actionList = new ArrayList<>();
                actionList.addAll( Arrays.asList( randomActions ) );
                actionList.add( action1 );
                for ( String action2 : actionList ) {
                    switch ( action2 ) {
                    case "find":
                        RbacUtils.findActionSupportCommand( sdb, csName, clName,
                                userCL, false );
                        break;
                    case "insert":
                        RbacUtils.insertActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "update":
                        RbacUtils.updateActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "remove":
                        RbacUtils.removeActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "getDetail":
                        RbacUtils.getDetailActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "alterCL":
                        RbacUtils.alterCLActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "createIndex":
                        RbacUtils.createIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "dropIndex":
                        RbacUtils.dropIndexActionSupportCommand( sdb, csName,
                                clName, userCL, false );
                        break;
                    case "truncate":
                        userCL.truncate();
                        break;
                    default:
                        break;
                    }
                }
            } finally {
                sdb.removeUser( user, password );
                sdb.dropRole( subRoleName );
                sdb.dropRole( mainRoleName );
            }
        }
    }

    private String[] desRemoveAction( String[] actions, String desRemove ) {
        int newLength = actions.length - 1;
        String[] newActions = new String[ newLength ];
        int newIndex = 0;
        for ( String action : actions ) {
            if ( !action.equals( desRemove ) ) {
                newActions[ newIndex ] = action;
                newIndex++;
            }
        }
        return newActions;
    }
}