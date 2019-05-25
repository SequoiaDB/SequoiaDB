package com.sequoiadb.testdata;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.*;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

public class SDBTestHelper {
    public static long getTotalBySnapShotKey(DBCursor snapshotCur, String key) {
        long total = 0;
        while (snapshotCur.hasNext()) {
            BSONObject result = snapshotCur.getNext();
            total += (Long) result.get(key);
        }

        return total;
    }

    public static int waitIndexCreateFinish(DBCollection cl, String indexName,
                                            int count) {
        int i = 0;
        DBCursor cursor = cl.getIndex(indexName);
        while ((cursor == null || !cursor.hasNext())) {
            if (i > count) {
                throw new RuntimeException(
                    ("wait index create failed:" + indexName));
            }
            i++;
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            cursor = cl.getIndex(indexName);
        }

        return 0;
    }

    public static int waitIndexDropFinish(DBCollection cl, String indexName,
                                          int count) {
        int i = 0;
        DBCursor cursor = cl.getIndex(indexName);
        while ((cursor != null && cursor.hasNext())) {
            if (i > count) {
                throw new RuntimeException(("wait index drop failed:" + indexName));
            }
            i++;
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            cursor = cl.getIndex(indexName);
        }

        return 0;
    }

    public static BSONObject byteArrayToBSONObject(byte[] array)
        throws BaseException {
        if (array == null || array.length == 0) {
            return null;
        }

        return BSON.decode(array);
    }

    public static void displayByteBuffer(ByteBuffer buffer) {
        displayByteArray(buffer.array());
    }

    public static void displayByteArray(byte[] array) {
        for (int i = 0; i < array.length; i++) {
            int tmp = array[i] & 0xFF;
            if (tmp < 0x10) {
                System.out.print("0");
            }
            System.out.print(Integer.toHexString(tmp).toUpperCase());
            System.out.print(" ");
        }

        System.out.println();
    }

    public static Date millisToDate(long millis) {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis(millis);
        return calendar.getTime();
    }

    public static void println(String msg) {
        String fileName;
        int lineNum;
        fileName = Thread.currentThread().getStackTrace()[2].getFileName();
        lineNum = Thread.currentThread().getStackTrace()[2].getLineNumber();
        System.out.println(getCurrentData() + " " + fileName + ":" + lineNum
            + " - " + msg);
    }

    public static String getCurrentData() {
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        return df.format(new Date());
    }
}
