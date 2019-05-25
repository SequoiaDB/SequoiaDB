package org.springframework.data.sequoiadb.core;

import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.sequoiadb.SequoiadbFactory;
import org.springframework.data.sequoiadb.assist.DBCursor;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;
import org.springframework.data.sequoiadb.assist.*;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Unit test for simple App.
 */
@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration("classpath:infrastructure.xml")
public class ThreadSafeTest
{
    @Autowired
    SequoiadbTemplate template;
    @Autowired
    SequoiadbFactory factory;
    AtomicLong atomicLong = new AtomicLong(0);

    @Before
    public void setUp() {
    }

    @After
    public void tearDown() {
        template.getDb().getSdb().close();
    }

    abstract class LocalTask implements Runnable {
        SequoiadbTemplate template ;
        int runTimes;
        String clName;
        Random random;
        int sequence;

        public LocalTask(SequoiadbTemplate template, int runTimes, String clName, int sequence) {
            this.template = template;
            this.runTimes = runTimes;
            this.clName = clName;
            this.sequence = sequence;
            this.random = new Random();
        }
    }

    class QueryTask extends LocalTask {
        public QueryTask(SequoiadbTemplate template, int runTimes, String clName, int sequence) {
            super(template, runTimes, clName, sequence);
        }

        public void run() {
            int counter = 0;
            while (runTimes-- != 0) {
                counter++;
                int limit = random.nextInt(1000);
                DB db = template.getDb();
                DBCollection cl = template.getCollection(clName);
                DBCursor cursor = cl.find(null, null, null, null, 0, limit, 0);
                int recordCounter = 0;
                try {
                    while (cursor.hasNext()) {
                        BSONObject obj = cursor.next();
                        recordCounter++;
                    }
                } finally {
                    cursor.close();
                }
                checkConnectionStatus(template, "Query task");
                try {
                    Thread.sleep(random.nextInt(1000));
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                System.out.println(String.format("In query task, thread[%d] finish running the [%d] time, return [%d] records",
                        sequence, counter, recordCounter));
            }
        }
    }

    class InsertTask extends LocalTask {
        public InsertTask(SequoiadbTemplate template, int runTimes, String clName, int sequence) {
            super(template, runTimes, clName, sequence);
        }

        public void run() {
            long beginTime = System.currentTimeMillis();
            int counter = 0;
            while (runTimes-- != 0) {
                counter++;
                int times = random.nextInt(300);
                List<BSONObject> list = new ArrayList<BSONObject>(times);
                while (times-- >= 0) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put("a", random.nextInt(1000));
                    obj.put("b", random.nextInt(1000));
                    obj.put("c", random.nextInt(1000));
                    list.add(obj);
                }
                DBCollection cl = template.getCollection(clName);
                cl.insert(list);
                checkConnectionStatus(template, "Insert task");
                System.out.println(String.format("In insert task, thread[%d] finish running the [%d] time",
                        sequence, counter));
            }
            long endTime = System.currentTimeMillis();
            atomicLong.addAndGet(endTime - beginTime);
        }
    }

    void initForTest(SequoiadbTemplate template, String clName) {
        if (template.collectionExists(clName)) {
            template.dropCollection(clName);
        }
        template.createCollection(clName);
    }

    void checkConnectionStatus(SequoiadbTemplate template, String msg) {
        Sdb sdb = template.getDb().getSdb();
        int usedConnCount = sdb.getUsedConnCount();
        int idleConnCount = sdb.getIdleConnCount();
        System.out.println(String.format("%s, used: %d, idle: %d", msg, usedConnCount, idleConnCount));
    }

    @Test
    public void runTasksTest() {
        String clName = "test2";
        int insertRumTimes = 30;
        int queryRumTimes = 30;
        int insertThreadCount = 10;
        int queryThreadCount = 10;
        Thread[] queryTaskTheads = new Thread[queryThreadCount];
        Thread[] insertTaskTheads = new Thread[insertThreadCount];

        initForTest(template, clName);

        for(int i = 0; i < insertThreadCount; i++) {
            insertTaskTheads[i] = new Thread(new InsertTask(template, insertRumTimes, clName, i));
        }

        for(int i = 0; i < insertThreadCount; i++) {
            insertTaskTheads[i].start();
        }

        for(int i = 0; i < queryThreadCount; i++) {
            queryTaskTheads[i] = new Thread(new QueryTask(template, queryRumTimes, clName, i));
        }

        for(int i = 0; i < queryThreadCount; i++) {
            queryTaskTheads[i].start();
        }

        for(int i = 0; i < queryThreadCount; i++) {

            try {
                queryTaskTheads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        for(int i = 0; i < insertThreadCount; i++) {

            try {
                insertTaskTheads[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        System.out.println(String.format("Takes %dms", (int)atomicLong.get() / insertThreadCount));
        System.out.println("Finish!");
    }

}


