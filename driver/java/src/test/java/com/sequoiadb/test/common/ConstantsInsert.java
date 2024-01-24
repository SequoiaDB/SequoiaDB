package com.sequoiadb.test.common;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import java.util.ArrayList;
import java.util.List;

public class ConstantsInsert {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;

    public static List<BSONObject> createRecordList(int listSize) {
        List<BSONObject> list = null;
        if (listSize <= 0) {
            return list;
        }
        try {
            list = new ArrayList<BSONObject>();
            for (int i = 0; i < listSize; i++) {
                BSONObject obj = new BasicBSONObject();
                BSONObject obj4 = new BasicBSONObject();

                obj4.put("0", 0 + i);
                obj4.put("1", 1 + i);
                obj.put("phone", obj4);
                obj.put("Id", i);
                obj.put("age", (int) (Math.random() * 100));
                obj.put("str", "foo_" + String.valueOf(i));

                list.add(obj);
            }
        } catch (Exception e) {
            System.out.println("Failed to create list record.");
            e.printStackTrace();
        }
        return list;
    }

    public static void insertVastRecord(String host, int port, String csName, String clName, int pageSize, int num) {
        try {
            Sequoiadb sdb;
            CollectionSpace cs;
            DBCollection cl;
            // connect
            sdb = new Sequoiadb(host, port, "", "");
            // cs
            if (sdb.isCollectionSpaceExist(csName)) {
                sdb.dropCollectionSpace(csName);
                cs = sdb.createCollectionSpace(csName, pageSize);
            } else
                cs = sdb.createCollectionSpace(csName, pageSize);
            // build shardingkey1
            BSONObject shardingkey = new BasicBSONObject();
            shardingkey.put("ShardingKey", new BasicBSONObject("Id", 1));
            // cl
            cl = cs.createCollection(clName, shardingkey);
            // index
//			cl.createIndex("IdIndex", "{\"Id\":1}", false, false);
            cl.createIndex("ageIndex", "{\"age\":-1}", false, false);
            // TO DO
            if (num <= 0) {
                throw new Exception("insert number must > 0");
            }
            long UNIT = 200000;
            long i = 0;
            if (num <= UNIT) {
                cl.bulkInsert(createRecordList(num), DBCollection.FLG_INSERT_CONTONDUP);
                return;
            }
            for (i = 0; (i + 1) * UNIT < num; i++) {
                List<BSONObject> list = new ArrayList<BSONObject>();
                for (long j = i * UNIT; j < (i + 1) * UNIT; j++) {
                    BSONObject obj = new BasicBSONObject();
                    BSONObject obj4 = new BasicBSONObject();

                    obj4.put("0", 0 + j);
                    obj4.put("1", 1 + j);

                    obj.put("phone", obj4);
                    obj.put("Id", j);
                    obj.put("age", (int) (Math.random() * 100));
                    obj.put("str", "foo_" + String.valueOf(j));
                    list.add(obj);
                }
                cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
            }
            long remainder = num - i * UNIT;
            if (remainder > 0) {
                List<BSONObject> list = new ArrayList<BSONObject>();
                for (long j = i * UNIT; j < num; j++) {
                    BSONObject obj = new BasicBSONObject();
                    BSONObject obj4 = new BasicBSONObject();

                    obj4.put("0", 0 + j);
                    obj4.put("1", 1 + j);

                    obj.put("phone", obj4);
                    obj.put("Id", j);
                    obj.put("age", (int) (Math.random() * 100));
                    obj.put("str", "foo_" + String.valueOf(j));
                    list.add(obj);
                }
                cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);
            }
        } catch (Exception e) {
            System.out.println("Failed to create list record.");
            e.printStackTrace();
        }
    }

    public static BSONObject createOneRecord() {

        BSONObject obj = null;
        try {
            obj = new BasicBSONObject();
            BSONObject obj1 = new BasicBSONObject();
            BSONObject obj2 = new BasicBSONObject();
            BSONObject obj3 = new BasicBSONObject();
            ObjectId id = new ObjectId();
            ;

//			obj.put("_id",id) ;
            obj.put("Id", 10);
            obj.put("����", "��ķ");
            obj.put("����", 30);
            obj.put("Age", 30);

            obj1.put("0", "123456");
            obj1.put("1", "654321");

            obj.put("�绰", obj1);
            obj.put("boolean1", true);
            obj.put("boolean2", false);
            obj.put("nullobj", null);
            obj.put("intnum", 999999999);
            obj.put("floatnum", 9999999999.9999999999);

        } catch (Exception e) {
            System.out.println("Failed to create chinese record.");
            e.printStackTrace();
        }
        return obj;
    }

    public static void insertRecords(DBCollection cl, int num) throws BaseException {
        try {
            // check arg
            if (null == cl || num <= 0) {
                throw new BaseException(SDBError.SDB_INVALIDARG);
            }
            // TO DO
            cl.bulkInsert(createRecordList(num), DBCollection.FLG_INSERT_CONTONDUP);
        } catch (Exception e) {
            System.out.println("Failed to insert records.");
            e.printStackTrace();
        }
    }

    public static void insertLongRecords(DBCollection cl, int length, int num) throws BaseException {
        try {
            // check arg
            if (null == cl || num <= 0 || length <= 0) {
                throw new BaseException(SDBError.SDB_INVALIDARG);
            }
            // TO DO
            BSONObject obj = new BasicBSONObject();
            long sum = length * num;
            for (int i = 0; i < sum; i++) {
                obj.put("No." + i, i);
            }
            cl.insert(obj);
        } catch (Exception e) {
            System.out.println("Failed to insert records.");
            e.printStackTrace();
        }
    }

    public static void insertSameRecords(DBCollection cl, BSONObject record, int insertTimes) throws BaseException {
        try {
            // check arg
            if (null == cl ||  null == record || insertTimes <= 0) {
                throw new BaseException(SDBError.SDB_INVALIDARG);
            }
            // TO DO
            for (int i = 0; i < insertTimes; i++) {
                BSONObject doc = new BasicBSONObject();
                doc.putAll(record);
                cl.insert(doc);
            }
        } catch (Exception e) {
            System.out.println("Failed to insert same records.");
            e.printStackTrace();
        }
    }
}
