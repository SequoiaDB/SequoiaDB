package com.sequoiadb.subcl;

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
 * @TestName:SEQDB-36 attach子表时，区间范围类型为所有的字段类型 : 1.创建主表，子表；
 *                    2.挂载子表时指定区间的Key为整数，长整数，浮点数，高精度数，字符串，日期，时间戳；
 *                    3.往子表内写入区间同类型的属于区间的数据； 4.验证数据写入是否成功和正确；
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class SubCL36 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL36";
    private String subCLName = "subCL36";

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
                    .parse( "{IsMainCL:true,ShardingKey:{\"alph\":1}}" ) );
            subCL = commCS.createCollection( subCLName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"tx_id\":1},ShardingType:\"hash\"}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase36 setUp error, error description:"
                    + e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }

    }

    // 挂载子表时指定区间的Key为整数，长整数，浮点数，高精度数，字符串，日期，时间戳,往子表内写入区间同类型的属于区间的数据
    @Test
    public void test() {
        for ( int i = 0; i < 7; i++ ) {
            try {
                // mainCL挂载subCL，挂载区间由i指定，i从0-6，对应挂载6种类型区间
                attachSelctor( i );
                // 向mainCL插入数据，数据类型有i指定，i从0-6，对应插入6种数据，并检查插入结果，最后删除插入的数据，解除挂载
                dataSelector( i );
            } catch ( BaseException e ) {
                Assert.fail( e.getMessage() + "\r\n"
                        + SubCLUtils2.getKeyStack( e, this ) );
            }
        }
    }

    // 根据type，为mainCL attach不同的上下线类型
    public void attachSelctor( int type ) throws BaseException {
        try {
            switch ( type ) {
            case 0:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":1},UpBound:{\"alph\":100}}" ) );
                break;
            case 1:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":3000000001},UpBound:{\"alph\":3000000010}}" ) );
                break;
            case 2:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":123.456},UpBound:{\"alph\":456.123}}" ) );
                break;
            case 3:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":{$decimal:\"123.456\"}},UpBound:{\"alph\":{$decimal:\"456.123\"}}}" ) );
                break;
            case 4:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":\"abc\"},UpBound:{\"alph\":\"zero\"}}" ) );
                break;
            case 5:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":{$date:\"2012-01-01\"}},UpBound:{\"alph\":{$date:\"2012-12-12\"}}}" ) );
                break;
            case 6:
                mainCL.attachCollection( this.subCL.getFullName(),
                        ( BSONObject ) JSON.parse(
                                "{LowBound:{\"alph\":{$timestamp:\"2012-01-01-13.14.26.124233\"}},UpBound:{\"alph\":{$timestamp:\"2012-12-12-13.14.26.124233\"}}}" ) );
                break;

            }
        } catch ( Exception e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    // 根据type，为mainCL插入不同的数据，并校验,删除插入记录，解除主子表关联
    public void dataSelector( int type ) throws BaseException {
        BSONObject bobj = null;
        switch ( type ) {
        case 0:
            bobj = ( BSONObject ) JSON.parse( "{name:\"Tom1\",alph:23}" );
            break;
        case 1:
            bobj = ( BSONObject ) JSON
                    .parse( "{name:\"Tom2\",alph:3000000001}" );
            break;
        case 2:
            bobj = ( BSONObject ) JSON.parse( "{name:\"Tom3\",alph:124.456}" );
            break;
        case 3:
            bobj = ( BSONObject ) JSON
                    .parse( "{name:\"Tom4\",alph:{$decimal:\"124.456\"}}" );
            break;
        case 4:
            bobj = ( BSONObject ) JSON.parse( "{name:\"Tom5\",alph:\"boy\"}" );
            break;
        case 5:
            bobj = ( BSONObject ) JSON
                    .parse( "{name:\"Tom6\",alph:{$date:\"2012-02-01\"}}" );
            break;
        case 6:
            bobj = ( BSONObject ) JSON.parse(
                    "{name:\"Tom7\",alph:{$timestamp:\"2012-02-01-13.14.26.124233\"}}" );
            break;
        }
        try {
            mainCL.insert( bobj );

            if ( !SubCLUtils2.isCollectionContainThisJSON( subCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }
            if ( !SubCLUtils2.isCollectionContainThisJSON( mainCL,
                    bobj.toString() ) ) {
                Assert.fail( "check resault not pass" );
            }

            mainCL.delete( bobj );
            mainCL.detachCollection( subCL.getFullName() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SubCLUtils2.getKeyStack( e, this ) );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( mainCLName );
            commCS.dropCollection( subCLName );
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
