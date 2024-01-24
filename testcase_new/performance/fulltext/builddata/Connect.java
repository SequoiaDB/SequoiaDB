package com.sequoiadb.builddata;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;

public class Connect {

    private Sequoiadb sdb;
    private DBCollection cl;

    public Connect(String hostName, String coord, String csName, String clName) {
        sdb = new Sequoiadb(hostName + ":" + coord, "", "");
        cl = sdb.getCollectionSpace(csName).getCollection(clName);
    }

    public void insert(DataRecord record) throws BuildException {
        BSONObject obj = DataUtils.getBSONRecord(record);
        cl.insert(obj);
    }

    public void insert(DataRecord record, String containStr) throws BuildException {
        BSONObject obj = DataUtils.getBSONRecord(record, containStr);
        cl.insert(obj);
    }

    public void insert(List<DataRecord> record) throws BuildException {
        List<BSONObject> l = DataUtils.getBSONRecords(record);
        cl.insert(l);
    }

    public void insert(List<DataRecord> record, List<String> containStr) throws BuildException {
        List<BSONObject> l = DataUtils.getBSONRecords(record, containStr);
        cl.insert(l);
    }

    public void insert(DataRecord record, int size) throws BuildException {
        List<BSONObject> l = null;
        for (int i = 0; i < size / 100; i++) {
            l = DataUtils.getBSONRecords(record, 100);
            cl.insert(l);
            l.clear();
        }
        if (size % 100 != 0) {
            l = DataUtils.getBSONRecords(record, size % 100);
            cl.insert(l);
            l.clear();
        }
    }

    public void insert(DataRecord record, int size, int recordNum) throws BuildException {
        List<BSONObject> l = null;
        for (int i = 0; i < size / 100; i++) {
            l = DataUtils.getBSONRecords(record, 100, "", "");
            for (int j = 0; j < recordNum; j++) {
                for (BSONObject bsonObject : l) {
                    bsonObject.removeField("_id");
                    if (j == 0) {
                        synchronized (Build.BUILD.sameRecordFileName) {
                            String field = (String) bsonObject.get(Build.BUILD.fields[0]);
                            try {
                                BuildUtils.write2File(Build.BUILD.filePath + Build.BUILD.sameRecordFileName,
                                        field + "\n");
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
                cl.insert(l);
            }
            l.clear();
        }
        if (size % 100 != 0) {
            l = DataUtils.getBSONRecords(record, size % 100, "", "");
            for (int i = 0; i < recordNum; i++) {
                for (BSONObject bsonObject : l) {
                    bsonObject.removeField("_id");
                    if (i == 0) {
                        synchronized (Build.BUILD.sameRecordFileName) {
                            String field = (String) bsonObject.get(Build.BUILD.fields[0]);
                            try {
                                BuildUtils.write2File(Build.BUILD.filePath + Build.BUILD.sameRecordFileName,
                                        field + "\n");
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }
                cl.insert(l);
            }
            l.clear();
        }
    }

    public void insert(DataRecord record, int size, boolean containStr) throws BuildException {

        if (containStr) {
            List<BSONObject> l = new ArrayList<>();
            for (int i = 0; i < size; i++) {
                while (!PollTh.insertOk) {
                    String str = BuildMQ.MSGQUEUE.pop();
                    if (str != null) {
                        String[] strs = str.split(" ");
                        if (strs.length <= 2) {
                            l.add(DataUtils.getBSONRecord(record, str, ""));
                        } else {
                            if (strs[2].equals("flag")) {
                                l.add(DataUtils.getBSONRecord(record, strs[0], strs[1]));
                            } else {
                                l.add(DataUtils.getBSONRecord(record, str, ""));
                            }
                        }
                        break;
                    }
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                if (PollTh.insertOk) {
                    String str = BuildMQ.MSGQUEUE.pop();
                    if (str != null) {
                        String[] strs = str.split(" ");
                        if (strs.length <= 2) {
                            l.add(DataUtils.getBSONRecord(record, str, ""));
                        } else {
                            if (strs[2].equals("flag")) {
                                l.add(DataUtils.getBSONRecord(record, strs[0], strs[1]));
                            } else {
                                l.add(DataUtils.getBSONRecord(record, str, ""));
                            }
                        }
                    } else {
                        l.add(DataUtils.getBSONRecord(record, "", ""));
                    }
                }
                if (l.size() % 100 == 0) {
                    cl.insert(l);
                    l.clear();
                }
            }
            if (!l.isEmpty()) {
                cl.insert(l);
            }
        } else {
            insert(record, size);
        }
    }

    public void insert(DataRecord record, int size, String containStr) throws BuildException {
        List<BSONObject> l = null;
        for (int i = 0; i < size / 100; i++) {
            l = DataUtils.getBSONRecords(record, 100, containStr);
            cl.insert(l);
            l.clear();
        }
        if (size % 100 != 0) {
            l = DataUtils.getBSONRecords(record, size % 100, containStr);
            cl.insert(l);
            l.clear();
        }
    }

    public void close() {
        if (null != sdb) {
            sdb.close();
        }
    }
}
