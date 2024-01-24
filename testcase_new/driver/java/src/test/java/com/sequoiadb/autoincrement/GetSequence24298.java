package com.sequoiadb.autoincrement;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @describe seqDB-24298:获取不存在Sequence字段，校验驱动是否报错
 * @author YiPan
 * @Date 2021.7.28
 * @version 1.00
 */
public class GetSequence24298 extends SdbTestBase {
    private Sequoiadb sdb;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @Test
    public void test() {
        try {
            sdb.getSequence( "SeqNameNotExist" );
            Assert.fail( "except fail but succeed" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_SEQUENCE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.close();
    }
}