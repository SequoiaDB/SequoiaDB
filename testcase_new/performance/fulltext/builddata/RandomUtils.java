package com.sequoiadb.builddata;

import java.util.Random;

public enum RandomUtils {

    UTILS;

    private String str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
    private Random random = new Random();

    String getNonSpaceStr(int len) {
        String s = getStr(len);
        s = s.replace(" ", "");
        if (s.length() != len) {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < len - s.length(); i++) {
                int n = random.nextInt(str.length() - 1);
                sb.append(str.charAt(n));
            }
            s = s + sb.toString();
        }
        return s;
    }

    String getStr(int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            int n = random.nextInt(str.length());
            sb.append(str.charAt(n));
        }
        return sb.toString();
    }

    String[] getArray(int size, int len) {
        String[] arr = new String[size];
        for (int i = 0; i < arr.length; i++) {
            arr[i] = "'" + getStr(len) + "'";
        }
        return arr;
    }

    String[] getNonSpaceArray(int size, int len) {
        String[] arr = new String[size];
        for (int i = 0; i < arr.length; i++) {
            arr[i] = "'" + getNonSpaceStr(len) + "'";
        }
        return arr;
    }
}
