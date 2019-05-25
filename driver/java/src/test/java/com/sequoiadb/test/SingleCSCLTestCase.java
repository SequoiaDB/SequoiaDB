package com.sequoiadb.test;

import com.sequoiadb.base.DBCollection;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.AfterClass;
import org.junit.BeforeClass;

/*
 * Super class of single test that use only one collectionspace and one collection
 * */
public abstract class SingleCSCLTestCase extends SingleCSTestCase {
    protected static String clName;
    protected static DBCollection cl;

    @BeforeClass
    public static void setUpTestCase() {
        SingleCSTestCase.setUpTestCase();

        clName = "SingleCSCLTestCase_" + new ObjectId().toString();
        if (cs.isCollectionExist(clName)) {
            cs.dropCollection(clName);
        }

        BSONObject options = new BasicBSONObject();
        options.put("Group", TestConfig.getSingleGroup());
        cl = cs.createCollection(clName, options);
    }

    @AfterClass
    public static void tearDownTestCase() {
        if (sdb != null && cs != null) {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
            cl = null;
            clName = null;
        }
        SingleCSTestCase.tearDownTestCase();
    }
}
