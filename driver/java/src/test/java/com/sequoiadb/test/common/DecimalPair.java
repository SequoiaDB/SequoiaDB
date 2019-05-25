package com.sequoiadb.test.common;

public class DecimalPair {
    public int precision;
    public int scale;

    public DecimalPair() {
        precision = -1;
        scale = -1;
    }

    public DecimalPair(int p, int s) {
        precision = p;
        scale = s;
    }
}