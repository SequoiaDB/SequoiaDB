package com.sequoias3.common;

public enum DataShardingType {
    /**
     * year
     */
    YEAR("year"),

    /**
     * month
     */
    MONTH("month"),

    /**
     * quarter
     */
    QUARTER("quarter");

    private String desc;

    DataShardingType(String name) {
        this.desc = name;
    }

    public String getDesc(){
        return desc;
    }

    @Override
    public String toString() {
        return desc;
    }

    public static DataShardingType getType(String name){
        for (DataShardingType type : DataShardingType.values()){
            if (type.getDesc().equals(name)){
                return type;
            }
        }
        return null;
    }
}
