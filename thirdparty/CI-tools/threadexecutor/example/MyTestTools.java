package com.sequoiadb.testschedule;

import java.io.Closeable;

import com.sequoiadb.base.Sequoiadb;

public class MyTestTools {
    public static void closeClosable(Closeable c) {
        if (null != c) {
            try {
                c.close();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public static void closeSequoiadb(Sequoiadb sdb) {
        try {
            if (null != sdb && !sdb.isClosed()) {
                closeClosable(sdb);
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
}
