package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;

/**
 * Created by tanzhaobo on 2017/9/26.
 */
public class Helper {
    public static CollectionSpace getOrCreateCollectionSpace(Sequoiadb db, String csName, BSONObject options) {
        CollectionSpace cs;
        if (db.isCollectionSpaceExist(csName)) {
            cs = db.getCollectionSpace(csName);
        } else {
            cs = db.createCollectionSpace(csName, options);
        }
        return cs;
    }

    public static DBCollection getOrCreateCollection(CollectionSpace cs, String clName, BSONObject options) {
        DBCollection cl;
        if (cs.isCollectionExist(clName)) {
            cl = cs.getCollection(clName);
        } else {
            cl = cs.createCollection(clName, options);
        }
        return cl;
    }
}
