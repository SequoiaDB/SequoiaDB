package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestName:SEQDB-10555 主表上执行切分 1.创建主子表 2.通过主表进行切分操作，覆盖：同步切分，异步切分，检查结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL10555 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL10555";
    private String subCLName = "subCL10555";
    private String groupName1;
    private String groupName2;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "StandAlone skip this test:"
                        + this.getClass().getName() );
            }
            ArrayList< String > groupsName = commlib.getDataGroupNames( sdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than one groups "
                                + this.getClass().getName() );
            }
            groupName1 = groupsName.get( 0 );
            groupName2 = groupsName.get( 1 );
            commCS = sdb.getCollectionSpace( csName );
            mainCL = commCS.createCollection( mainCLName, ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{sk:1}}" ) );
            subCL = commCS.createCollection( subCLName,
                    ( BSONObject ) JSON.parse( "{ShardingKey:{sk:1}}" ) );
            mainCL.attachCollection( subCL.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:100},UpBound:{sk:200}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 通过主表进行切分操作，覆盖：同步切分，异步切分，检查结果
    @Test(enabled = true)
    public void test() {
        ArrayList< String > resaults = new ArrayList< String >(); // 定义失败结果集
        try {
            mainCL.split( groupName1, groupName2, 50 );
            resaults.add( "mainCL split success" ); // 主表同步切分成功
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -246 ) {
                resaults.add( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) ); // 主表同步切分错误码不符合预期
            }
        }

        try {
            mainCL.splitAsync( groupName2, groupName2, 50 );
            resaults.add( "mainCL splitAsync success" ); // 主表异步切分成功
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -246 ) {
                resaults.add( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) ); // 主表异步切分错误码不符合预期
            }
        }

        if ( resaults.size() > 0 ) { // 检查结果集
            Assert.fail( resaults.toString() );
        }
    }

    @AfterClass
    public void tearDown() {

        try {
            commCS.dropCollection( subCLName );
            commCS.dropCollection( mainCLName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }
}
