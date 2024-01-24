package com.sequoiadb.crud.numoverflow;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.crud.numoverflow.NumOverflowUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-12586:多个字段使用$subtract操作不同类型运算溢出
 * @author wuyan
 * @date 2017/9/13
 * @updateUser wuyan
 * @updateDate 2021/10/14
 * @updateRemark SEQUOIADBMAINSTREAM-6546修改数组类型$[i]取值方式，更新用例中取值结果
 * @version 1.00
 */

public class SubtractIsSelector12586 extends SdbTestBase {

    private String clName = "subtract_selector12586";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private static DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = NumOverflowUtils.createCL( cs, clName );

        String[] records = {
                "{'no':-2147483648,'tlong':{'$numberLong':'9223372036854775807'},"
                        + "'arr':[1,3],'arr1':[1,[1,{'$numberLong':'9223372036854775800'}],2],"
                        + "obj:{a:{b:[4,{'$numberLong':'-9223372036854775808'}]}},obj1:{a:{b:-2}}}" };

        NumOverflowUtils.insert( cl, records );
    }

    @Test
    public void testSubtract() {
        String selector = "{no:{$subtract:{'$numberLong':'9223372034707292161'}},"
                + "tlong:{$subtract:-1},'arr.$[0]':{$subtract:{'$numberLong':'-9223372036854775807'}},"
                + "'arr1.$[1].$[1]':{$subtract:-8},'obj.a.b.$[1]':{$subtract:1},"
                + "'obj1.a.b':{$subtract:{'$numberLong':'9223372036854775807'}},_id:{$include:0}}";
        // TODO:SEQUOIADBMAINSTREAM-2764,the arr1.$[1].$[1] oper result is
        // error
        /*
         * String []expRecords = {"{'no':{'$decimal':'-9223372036854775809'}," +
         * "'tlong':{'$decimal':'9223372036854775808'},arr:[{'$decimal':'9223372036854775808'}]"
         * + "''arr1':[1,[1,{'$deciaml':'9223372036854775808'}],2]," +
         * "obj:{a:{b:[{'$decimal':'-9223372036854775809'}]}}," +
         * "obj1:{a:{b:{'$decimal':'-9223372036854775809'}}}}"};
         */
        String[] expRecords = { "{'no':{'$decimal':'-9223372036854775809'},"
                + "'tlong':{'$decimal':'9223372036854775808'},arr:[{'$decimal':'9223372036854775808'}],"
                + "'arr1':[[ { \"$decimal\" : \"9223372036854775808\" } ]],obj:{a:{b:[{'$decimal':'-9223372036854775809'}]}},"
                + "obj1:{a:{b:{'$decimal':'-9223372036854775809'}}}}" };
        NumOverflowUtils.multipleFieldOper( cl, selector, expRecords );

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
