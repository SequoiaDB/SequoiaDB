package com.sequoiadb.explain.serial;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-14453： analyze与sync并发 1.创建cs.cl； 2.如下2个步骤并发执行多次，例如：100次：
 *                        1)在cs.cl中插入记录，执行analyze()， 2)执行sync，其中参数Block:true，检查结果 问题单：3244
 * @Author chimanzhao
 * @Date 2020-05-19
 */
public class Explain14453 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clName = "cl_14453";
    private static int EXECRECORDS = 100;
    private static int DATANUMBERS = 1000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        cs = sdb.getCollectionSpace(csName);
        cl = cs.createCollection(clName);
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection(clName);
        } finally {
            if (sdb != null) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() {
        List<BSONObject> insertObjs = new ArrayList<BSONObject>();
        for (int i = 0; i < DATANUMBERS; i++) {
            insertObjs.add((BSONObject) JSON.parse("{a:" + i + ",b:'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                    + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                    + "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'}"));
        }

        AnalyzeAndInsert analyze = new AnalyzeAndInsert(insertObjs);
        analyze.start();

        Sync sync = new Sync();
        sync.start();

        Assert.assertTrue(analyze.isSuccess(), analyze.getErrorMsg());
        Assert.assertTrue(sync.isSuccess(), sync.getErrorMsg());
    }

    private class AnalyzeAndInsert extends SdbThreadBase {
        List<BSONObject> insertObjs = null;

        AnalyzeAndInsert(List<BSONObject> insertObjs) {
            this.insertObjs = insertObjs;
        }

        @Override
        public void exec() throws BaseException {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                for (int i = 0; i < EXECRECORDS; i++) {
                    cl.insert(insertObjs);
                    db.analyze( (BSONObject)JSON.parse( "{ 'Collection': '" + csName + "." + clName + "' }" ) );
                    cl.truncate();
                }
            }
        }
    }

    private class Sync extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
                for (int i = 0; i < EXECRECORDS; i++) {
                    db.sync((BSONObject) JSON.parse("{ Block:true } "));
                }
            }
        }
    }

}
