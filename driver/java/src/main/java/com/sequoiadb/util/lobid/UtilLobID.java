package com.sequoiadb.util.lobid;

import java.util.Date;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

public class UtilLobID {
    private static Random r = new Random();
    private static AtomicInteger atomicSerial = new AtomicInteger(r.nextInt(100000));

    private static final byte isOddArray[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0,
            0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0,
            1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
            1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1,
            0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1,
            1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
            0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
            0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
            0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0,
            0, 1, 0, 1, 1, 0};

    private long seconds; // use lower 6 bytes
    private byte oddCheck; // user lower 6 bits
    private short id;
    private int serial;

    public UtilLobID(Date d, short id) {
        seconds = d.getTime() / 1000;
        setOddCheckBit();
        this.id = id;
        serial = atomicSerial.getAndIncrement();
    }

    public UtilLobID(short id) {
        seconds = System.currentTimeMillis() / 1000;
        setOddCheckBit();
        this.id = id;
        serial = atomicSerial.getAndIncrement();
    }

    public UtilLobID(String s) {
        if (!isValid(s)) {
            throw new IllegalArgumentException("invalid ObjectId [" + s + "]");
        }

        byte b[] = new byte[12];
        for (int i = 0; i < b.length; i++) {
            b[i] = (byte) Integer.parseInt(s.substring(i * 2, i * 2 + 2), 16);
        }

        fromByteArray(b);
    }

    public long getSeconds() {
        return seconds;
    }

    public short getID() {
        return id;
    }

    public int getSerial() {
        return serial;
    }

    public boolean isValid(String s) {
        if (s == null) {
            return false;
        }

        final int len = s.length();
        if (len != 24) {
            return false;
        }

        for (int i = 0; i < len; i++) {
            char c = s.charAt(i);
            if (c >= '0' && c <= '9') {
                continue;
            }
            if (c >= 'a' && c <= 'f') {
                continue;
            }
            if (c >= 'A' && c <= 'F') {
                continue;
            }

            return false;
        }

        return true;
    }

    private void setOddCheckBit() {
        oddCheck = 0;

        for (int i = 5; i >= 0; --i) {
            int s = (int) ((seconds >> (i * 8)) & 0x0FF);
            if (!isOdd(s)) {
                oddCheck = (byte) (oddCheck | 1 << i);
            }
        }
    }

    private boolean bitIsOne(byte value, int pos) {
        if ((value & (1 << pos)) > 0) {
            return true;
        }

        return false;
    }

    private void checkOddBit() {
        for (int i = 5; i >= 0; --i) {
            int s = (int) ((seconds >> (i * 8)) & 0x0FF);
            if ((isOdd(s) && !bitIsOne(oddCheck, i)) || (!isOdd(s) && bitIsOne(oddCheck, i))) {
                continue;
            }

            throw new IllegalArgumentException(
                    "check odd bit failed:seconds=" + seconds + ",oddCheck=" + oddCheck);
        }
    }

    @Override
    public String toString() {
        byte b[] = toByteArray();

        StringBuilder buf = new StringBuilder(24);

        for (int i = 0; i < b.length; i++) {
            int x = b[i] & 0xFF;
            String s = Integer.toHexString(x);
            if (s.length() == 1) {
                buf.append("0");
            }
            buf.append(s);
        }

        return buf.toString();
    }

    private byte[] toByteArray() {
        byte array[] = new byte[12];
        int arrayIdx = 0;
        for (int i = 5; i >= 0; --i) {
            array[arrayIdx++] = (byte) (seconds >> (i * 8) & 0x0FF);
        }

        array[arrayIdx++] = oddCheck;

        for (int i = 1; i >= 0; --i) {
            array[arrayIdx++] = (byte) (id >> (i * 8) & 0x0FF);
        }

        for (int i = 2; i >= 0; --i) {
            array[arrayIdx++] = (byte) (serial >> (i * 8) & 0x0FF);
        }

        return array;
    }

    private void fromByteArray(byte array[]) {
        int arrayIdx = 0;
        seconds = 0;
        for (int i = 5; i >= 0; --i) {
            seconds = (seconds | (array[arrayIdx++] & 0xFF) << (i * 8));
        }

        oddCheck = array[arrayIdx++];

        checkOddBit();

        id = 0;
        for (int i = 1; i >= 0; --i) {
            id = (short) (id | (array[arrayIdx++] & 0xFF) << (i * 8));
        }

        serial = 0;
        for (int i = 2; i >= 0; --i) {
            serial = (serial | (array[arrayIdx++] & 0xFF) << (i * 8));
        }
    }

    private boolean isOdd(int v) {
        if (1 == isOddArray[v]) {
            return true;
        } else {
            return false;
        }
    }

    public static void main(String[] args) {
        Date d = new Date(1560549792000L);
        UtilLobID id = new UtilLobID(d, (short) 41096);
        UtilLobID id2 = new UtilLobID(d, (short) 41096);

        System.out.println(id);
        System.out.println(id2);

        UtilLobID id3 = new UtilLobID(id2.toString());

        System.out.println(id3);
    }
}
