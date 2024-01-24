package com.sequoiadb.builddata;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.util.JSON;

public class DataUtils {

    private static String getFieldValue(Field f) throws BuildException {
        StringBuilder sb = new StringBuilder();

        String key = f.getName();
        String type = f.getType();
        int len = f.getLen();

        switch (type) {
        case DataField.STRING:
            String value = RandomUtils.UTILS.getStr(len);
            sb.append("'" + key + "':'" + value + "', ");
            break;
        case DataField.ARRAY:
            int size = f.getSize();
            String[] value1 = RandomUtils.UTILS.getArray(size, len);
            sb.append("'" + key + "':" + Arrays.toString(value1) + ", ");
            break;
        default:
            throw new BuildException("Cannot match any field type");
        }
        return sb.toString();
    }

    private static String containStr(String t, String containStr) {
        Random random = new Random();
        int len = t.length() - containStr.length();
        if (0 == len) {
            t = containStr;
        } else if (1 == len) {
            if (random.nextBoolean()) {
                t = containStr + " ";
            } else {
                t = " " + containStr;
            }
        } else {
            t = t.substring(0, len);
            int n = random.nextInt(len);
            if (0 == n) {
                t = containStr + " " + t.substring(1, t.length());
            } else if (n == t.length() - 1) {
                t = t.substring(0, t.length() - 1) + " " + containStr;
            } else {
                t = t.substring(0, n - 1) + " " + containStr + " " + t.substring(n + 1, t.length());
            }
        }
        return t;
    }

    public static BSONObject getBSONRecord(DataRecord record) throws BuildException {
        List<Field> l = record.getFields();
        StringBuilder sb = new StringBuilder();
        sb.append("{");

        for (Field f : l) {
            String field = getFieldValue(f);
            if (null == field) {
                throw new BuildException("Field cannot be null");
            }
            sb.append(field);
        }

        sb.append("}");

        BSONObject obj = (BSONObject) JSON.parse(sb.toString());
        return obj;
    }

    /**
     * get a bsonObject contains a specified string, the first field will contain it
     * 
     * @param record
     * @param containStr
     * @return
     * @throws BuildException
     */
    public static BSONObject getBSONRecord(DataRecord record, String containStr) throws BuildException {
        List<Field> l = record.getFields();
        if (l.size() == 0) {
            throw new BuildException("Record fields size cannot be 0");
        }
        Field f = l.get(0);
        if (f.getType().equals(DataField.STRING)) {
            if (f.getLen() < containStr.length()) {
                throw new BuildException("Field length cannot less than contain string");
            }
        }

        StringBuilder sb = new StringBuilder();
        sb.append("{");
        String field = getFieldValue(f);
        if (null == field) {
            throw new BuildException("Field cannot be null");
        } else {
            String str[] = field.split("'");
            String t = str[3];
            if (f.getType().equals(DataField.STRING)) {
                t = containStr(t, containStr);
            } else if (f.getType().equals(DataField.ARRAY)) {
                t = containStr;
            }
            StringBuilder s = new StringBuilder();
            for (int i = 0; i < str.length; i++) {
                if (0 != i) {
                    s.append("'");
                }
                if (3 == i) {
                    s.append(t);
                    continue;
                }
                s.append(str[i]);
            }
            field = s.toString();
        }
        sb.append(field);

        for (int i = 1; i < l.size(); i++) {
            field = getFieldValue(l.get(i));
            if (null == field) {
                throw new BuildException("Field cannot be null");
            }
            sb.append(field);
        }

        sb.append("}");

        BSONObject obj = (BSONObject) JSON.parse(sb.toString());
        return obj;
    }

    /**
     * get a bsonObject contains a specified string, the first field will contain it
     * 
     * @param record
     * @param containStr
     * @return
     * @throws BuildException
     */
    public static BSONObject getBSONRecord(DataRecord record, String containStr, String containStr2)
            throws BuildException {
        List<Field> l = record.getFields();
        if (l.size() <= 2) {
            throw new BuildException("Record fields size cannot less then 2");
        }
        Field f = l.get(0);
        if (f.getType().equals(DataField.STRING)) {
            if (f.getLen() < containStr.length()) {
                throw new BuildException("Field length cannot less than contain string, field length: " + f.getLen()
                        + " containStr length: " + containStr.length());
            }
        }

        StringBuilder sb = new StringBuilder();
        sb.append("{");
        String field = getFieldValue(f);
        if (null == field) {
            throw new BuildException("Field cannot be null");
        } else {
            String str[] = field.split("'");
            String t = str[3];
            if (f.getType().equals(DataField.STRING)) {
                t = containStr(t, containStr);
            } else if (f.getType().equals(DataField.ARRAY)) {
                t = containStr;
            }
            StringBuilder s = new StringBuilder();
            for (int i = 0; i < str.length; i++) {
                if (0 != i) {
                    s.append("'");
                }
                if (3 == i) {
                    s.append(t);
                    continue;
                }
                s.append(str[i]);
            }
            field = s.toString();
        }
        sb.append(field);

        Field f2 = l.get(1);
        String k = f2.getName();
        String v = containStr2;
        if ("".equals(containStr2)) {
            v = RandomUtils.UTILS.getNonSpaceStr(30);
        }
        sb.append("'" + k + "':'" + v + "', ");

        for (int i = 2; i < l.size(); i++) {
            f2 = l.get(i);
            k = f2.getName();
            v = RandomUtils.UTILS.getNonSpaceStr(30);
            sb.append("'" + k + "':'" + v + "', ");
        }

        sb.append("}");

        BSONObject obj = (BSONObject) JSON.parse(sb.toString());
        return obj;
    }

    public static List<BSONObject> getBSONRecords(List<DataRecord> records) throws BuildException {
        List<BSONObject> l = new ArrayList<>();
        for (int i = 0; i < records.size(); i++) {
            l.add(getBSONRecord(records.get(i)));
        }
        return l;
    }

    public static List<BSONObject> getBSONRecords(DataRecord records, int size) throws BuildException {
        List<BSONObject> l = new ArrayList<>();
        for (int i = 0; i < size; i++) {
            l.add(getBSONRecord(records));
        }
        return l;
    }

    public static List<BSONObject> getBSONRecords(List<DataRecord> records, List<String> containStr)
            throws BuildException {
        List<BSONObject> l = new ArrayList<>();
        for (int i = 0; i < records.size(); i++) {
            l.add(getBSONRecord(records.get(i), containStr.get(i)));
        }
        return l;
    }

    public static List<BSONObject> getBSONRecords(DataRecord records, int size, String containStr)
            throws BuildException {
        List<BSONObject> l = new ArrayList<>();
        for (int i = 0; i < size; i++) {
            l.add(getBSONRecord(records, containStr));
        }
        return l;
    }

    public static List<BSONObject> getBSONRecords(DataRecord records, int size, String containStr, String containStr2)
            throws BuildException {
        List<BSONObject> l = new ArrayList<>();
        for (int i = 0; i < size; i++) {
            l.add(getBSONRecord(records, containStr, containStr2));
        }
        return l;
    }
}
