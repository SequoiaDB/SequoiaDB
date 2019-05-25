package com.sequoiadb.test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.AfterClass;
import org.junit.BeforeClass;

/*
 * Super class of single test that use only one collectionspace
 * */
public abstract class SingleCSTestCase extends SingleTestCase {
    protected static String csName;
    protected static CollectionSpace cs;

    @BeforeClass
    public static void setUpTestCase() {
        SingleTestCase.setUpTestCase();

        csName = "SingleCSTestCase_" + new ObjectId().toString();
        if (sdb.isCollectionSpaceExist(csName)) {
            sdb.dropCollectionSpace(csName);
        }

        BSONObject options = new BasicBSONObject();
        options.put("LobPageSize", Sequoiadb.SDB_PAGESIZE_4K);
        cs = sdb.createCollectionSpace(csName, options);
    }

    @AfterClass
    public static void tearDownTestCase() {
        if (sdb != null) {
            if (sdb.isCollectionSpaceExist(csName)) {
                sdb.dropCollectionSpace(csName);
            }
            cs = null;
            csName = null;
            SingleTestCase.tearDownTestCase();
        }
    }
}
