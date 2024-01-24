package com.sequoiadb.sdb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.Test;

/**
 * Created by laojingtang on 17-12-7.
 */
public class TestGetPort13696 extends SdbTestBase {

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            int begin = SdbTestBase.coordUrl.indexOf( ':' ) + 1;
            int end = SdbTestBase.coordUrl.length();
            int expectPort = Integer
                    .parseInt( SdbTestBase.coordUrl.substring( begin, end ) );
            Assert.assertEquals( db.getPort(), expectPort );
        } finally {
            if ( db != null )
                db.disconnect();
        }
    }
}
