package com.sequoiadb.split;

import java.util.ArrayList;

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
 * @FileName:SEQDB-537 源分区组数据为空，进行百分比切分:1.在cl下指定百分比数进行数据切分
 *                     2、执行split操作，其中设置的源分区组中不存在数据 3、查看数据切分是否成功
 * @author huangqiaohui
 * @version 1.00
 *
 */
public class Split537 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace commCS;
    private DBCollection currentCL;
    private String clName = "testcaseCL537";

    @SuppressWarnings("deprecation")
    @BeforeClass(enabled = true)
    public void setUp() {
        try {
            sdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            if ( CommLib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }

            if ( CommLib.getDataGroupNames( sdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            commCS = sdb.getCollectionSpace( csName );
            currentCL = commCS.createCollection( clName, ( BSONObject ) JSON
                    .parse( "{ShardingKey:{\"a\":1},ShardingType:\"range\"}" ) );
        } catch ( BaseException e ) {
            if ( sdb != null ) {
                sdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    // range分区百分比数进行数据切分 ,执行split操作，其中设置的源分区组中不存在数据
    @Test(enabled = true)
    public void test() {
        try {
            ArrayList< String > groups = SplitUtils.getGroupName( sdb, csName,
                    clName );
            currentCL.split( groups.get( 0 ), groups.get( 1 ), 50 );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -296, e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
            return;
        }
        Assert.fail( "collection(ShardingType:range) is empty,split success" );
    }

    @SuppressWarnings("deprecation")
    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "messqge:" + e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

}
