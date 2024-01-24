package com.sequoiadb.auth;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.util.SdbDecrypt;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-17881:decryptPasswd参数校验
 * @author fanyu
 * @Date:2019年02月22日
 * @version:1.0
 */
public class DecryptPasswd17881 extends SdbTestBase {

    @BeforeClass
    private void setUp() throws Exception {
    }

    @DataProvider(name = "Invalidrange-provider")
    public Object[][] generateInvalidRangData() throws Exception {
        return new Object[][] { { "123456", "123" }, { null, "123" } };
    }

    @Test(dataProvider = "Invalidrange-provider")
    private void test( String encryptPasswd, String token ) throws Exception {
        SdbDecrypt sdbDecrypt = new SdbDecrypt();
        try {
            sdbDecrypt.decryptPasswd( encryptPasswd, token );
            Assert.fail( "exp fail but act success,encryptPasswd = "
                    + encryptPasswd + ",token = " + token );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 ) {
                throw e;
            }
        }
    }

    @AfterClass
    private void tearDown() {
    }
}
