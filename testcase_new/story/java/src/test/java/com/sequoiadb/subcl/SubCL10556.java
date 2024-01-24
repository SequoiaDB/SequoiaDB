package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @TestName:SEQDB-10556 主表对lob进行操作 1.创建主子表并挂载子表 2.通过主表对lob进行新增、删除、查询，检查操作结果
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL10556 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL10556";
    private String subCLName = "subCL10556";
    private ObjectId lobId;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "StandAlone skip this test:"
                        + this.getClass().getName() );
            }
            commCS = sdb.getCollectionSpace( csName );
            mainCL = commCS.createCollection( mainCLName, ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{sk:1},ShardingType:\"range\"}" ) );
            subCL = commCS.createCollection( subCLName,
                    ( BSONObject ) JSON.parse( "{ShardingKey:{sk:1}}" ) );

            // 为子表插入一个LOB
            DBLob lob = subCL.createLob();
            lobId = lob.getID();
            lob.write( mainCLName.getBytes() );
            lob.close();

            // attach
            mainCL.attachCollection( subCL.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{sk:100},UpBound:{sk:200}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 通过主表对lob进行新增、删除、查询，检查操作结果
    @Test(enabled = true)
    public void test() {
        ArrayList< String > resaults = new ArrayList< String >(); // 定义失败结果集
        try {
            DBLob lob = mainCL.createLob();
            lob.close();
            resaults.add( "mainCL createLob success" ); // 主表新增LOB成功
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                resaults.add( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) ); // 错误码不符合预期
            }
        }

        try {
            DBLob lob = mainCL.openLob( lobId );
            lob.close();
            resaults.add( "mainCL openLob success" ); // 主表查询LOB成功
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                resaults.add( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) ); // 错误码不符合预期
            }
        }

        try {
            mainCL.removeLob( lobId );
            resaults.add( "mainCL removeLob success" ); // 主表删除LOB成功
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                resaults.add( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) ); // 错误码不符合预期
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
