package com.sequoiadb.util.lobid;

public class UtilBitAssist {
    public static byte setBit1(byte value, int pos) {
        return (byte) (value | 1 << pos);
    }

    public static byte setBit0(byte value, int pos) {
        return (byte) (value & ~(1 << pos));
    }

    public static boolean isBit1(byte value, int pos) {
        return (value & (1 << pos)) > 0;
    }

    //1. bytePos start form 0
    //for example. value = 0xFFEE, 0xEE's bytePos is 0; 0xFF's bytePos is 1
    //2. operation 0xFF is to clean the sign flag
    public static short setByte(short value, int bytePos, byte byteValue) {
        short tmpValue = (short) (0xFFFF ^ 0xFF << (bytePos * 8));
        value = (short) (value & tmpValue);

        tmpValue = (short) ((byteValue << (bytePos * 8)));
        return (short) (value | tmpValue);
    }

    public static byte getByte(short value, int bytePos) {
        return (byte) (value >> (bytePos * 8) & 0x0FF);
    }

    public static int setByte(int value, int bytePos, byte byteValue) {
        return value | (byteValue << (bytePos * 8));
    }

    public static byte getByte(int value, int bytePos) {
        return (byte) (value >> (bytePos * 8) & 0x0FF);
    }

    public static long setByte(long value, int startPos, byte byteValue) {
        return value | (byteValue << startPos);
    }

    public static byte getByte(long value, int bytePos) {
        return (byte) (value >> (bytePos * 8) & 0x0FF);
    }

    public static String toHex(byte value) {
        int x = value & 0xFF;
        return Integer.toHexString(x);
    }

    private static byte[] toByteArray(short s) {
        byte[] b = new byte[2];
        b[0] = getByte(s, 1);
        b[1] = getByte(s, 0);

        return b;
    }

    private static byte[] toByteArray(int s) {
        byte[] b = new byte[4];
        b[0] = getByte(s, 3);
        b[1] = getByte(s, 2);
        b[2] = getByte(s, 1);
        b[3] = getByte(s, 0);

        return b;
    }

    private static byte[] toByteArray(long s) {
        byte[] b = new byte[8];
        b[0] = getByte(s, 7);
        b[1] = getByte(s, 6);
        b[2] = getByte(s, 5);
        b[3] = getByte(s, 4);
        b[4] = getByte(s, 3);
        b[5] = getByte(s, 2);
        b[6] = getByte(s, 1);
        b[7] = getByte(s, 0);

        return b;
    }

    public static String toHex(short value) {
        byte[] b = toByteArray(value);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < b.length; ++i) {
            int x = b[i] & 0xFF;
            String s = Integer.toHexString(x);
            if (s.length() == 1) {
                sb.append("0");
            }

            sb.append(s);
        }

        return sb.toString();
    }

    public static String toHex(int value) {
        byte[] b = toByteArray(value);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < b.length; ++i) {
            int x = b[i] & 0xFF;
            String s = Integer.toHexString(x);
            if (s.length() == 1) {
                sb.append("0");
            }

            sb.append(s);
        }

        return sb.toString();
    }

    public static String toHex(long value) {
        byte[] b = toByteArray(value);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < b.length; ++i) {
            int x = b[i] & 0xFF;
            String s = Integer.toHexString(x);
            if (s.length() == 1) {
                sb.append("0");
            }

            sb.append(s);
        }

        return sb.toString();
    }

    public static String toBinary(byte value) {
        int x = value & 0xFF;
        return Integer.toBinaryString(x);
    }

    public static void main(String[] args) {
        short a = -2;

        short tmpValue = (short) ((a << (8)));
    }

}
