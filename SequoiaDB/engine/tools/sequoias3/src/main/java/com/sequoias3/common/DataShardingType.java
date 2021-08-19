package com.sequoias3.common;

public enum DataShardingType {
    YEAR("year", 0), MONTH("month", 1), QUARTER("quarter", 2);

    private String name;
    private int type;

    DataShardingType(String name, int type) {
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public int getType() {
        return type;
    }

    public static  DataShardingType getShardingType(String type) {
        for(DataShardingType value:DataShardingType.values()) {
            if(value.getName().equals(type)) {
                return value;
            }
        }
        return null;
    }
}
