package com.sequoiadb.sdb;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.Test;

/**
 * Created by laojingtang on 17-12-7.
 */
public class TestGetHost13693 extends SdbTestBase {

    @Test
    public void testGethost() {
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            Assert.assertNotNull( db.getHost() );
        } finally {
            if ( db != null )
                db.disconnect();
        }
    }
}
