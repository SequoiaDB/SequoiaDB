package com.sequoiadb.builddata;

public class DataField implements Field {

    public static final String STRING = "string";
    public static final String ARRAY = "array";

    String name;
    String type;
    int len;
    int size;

    public DataField(String name, int len) {
        this.name = name;
        this.len = len;
        this.type = STRING;
    }

    public DataField(String name, int len, int size) {
        this.name = name;
        this.len = len;
        this.size = size;
        this.type = ARRAY;
    }

    @Override
    public int getLen() {
        return len;
    }

    @Override
    public int getSize() {
        return size;
    }

    @Override
    public String getType() {
        return type;
    }

    @Override
    public String getName() {
        return name;
    }

}
