package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: SelectSQL12620.java test content:Numeric value overflow for single
 * character,and execute sql in database. testlink case:seqDB-12620
 * 
 * @author wuyan
 * @Date 2017.9.15
 * @version 1.00
 */
public class SelectSQL12620 extends SdbTestBase {
    private String clName = "sql_select12620";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @DataProvider(name = "operData")
    public Object[][] generateIntDatas() {
        String[] expRecords1 = { "{'no':{'$decimal':'-9223372036854775809'}}" };
        String[] expRecords2 = {
                "{'long':{'$decimal':'9223372036854775808'}}" };
        String[] expRecords3 = {
                "{'long':{'$decimal':'9223810305706420000'} }" };
        String[] expRecords4 = { "{'no':753767 }" };

        return new Object[][] {
                // the parameters: operationExpression, matcherValue, expRecords
                // no:-2147483648
                new Object[] { "no - 9223372034707292161", "test=1",
                        expRecords1 },
                // long:9223372036854775808
                new Object[] { "long + 1", "test=2", expRecords2 },
                // long:-9223372036854775808
                new Object[] { "long / (-1)", "test=1", expRecords2 },
                // long:46116860387904
                new Object[] { "long * 210460000", "test=3", expRecords3 },
                // no:4294967296/5698 = 753767.5142,the result is 753767
                new Object[] { "no / 5698", "test=2", expRecords4 }, };
    }

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "connect %s failed," + coordUrl + e.getMessage() );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOption = "{ShardingKey:{no:1},ReplSize:0,Compressed:true, StrictDataMode:false}";
        cl = NumOverflowUtils.createCL( cs, clName, clOption );
        String[] records = {
                "{'no':-2147483648,'long':{'$numberLong':'-9223372036854775808'},'test':1}",
                "{'no':4294967296,'long':{'$numberLong':'9223372036854775807'},'test':2}",
                "{'no':1024,'long':{'$numberLong':'43826904427'},'test':3}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test(dataProvider = "operData")
    public void testSQL( String operExpression, String matcher,
            String[] expRecords ) {
        try {
            String sql = "select " + operExpression + " from "
                    + cl.getFullName() + " where " + matcher;
            NumOverflowUtils.sqlOper( sdb, sql, expRecords );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "select oper failed," + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

}
