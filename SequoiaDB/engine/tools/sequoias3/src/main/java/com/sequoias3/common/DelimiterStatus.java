package com.sequoias3.common;

public enum DelimiterStatus {
    NORMAL("Normal", 0),
    SUSPENDED("Suspended", 1),
    CREATING("Creating", 2),
    TOBEDELETE("ToBeDelete", 3),
    DELETING("Deleting", 4);

    private String name;
    private int    type;

    DelimiterStatus(String name, int type){
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public static DelimiterStatus getDelimiterStatus(String status){
        for (DelimiterStatus value:DelimiterStatus.values()){
            if (value.getName().equals(status)){
                return value;
            }
        }
        return NORMAL;
    }
}
