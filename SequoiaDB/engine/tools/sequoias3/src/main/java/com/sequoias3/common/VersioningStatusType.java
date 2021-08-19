package com.sequoias3.common;

public enum VersioningStatusType {
    NONE("None",0), ENABLED("Enabled", 1), SUSPENDED("Suspended", 2);
    private String name;
    private int    type;

    VersioningStatusType(String name, int type){
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public int getType() {
        return type;
    }

    public static VersioningStatusType getVersioningStatus(String type){
        for(VersioningStatusType value:VersioningStatusType.values()) {
            if(value.getName().equals(type)) {
                return value;
            }
        }
        return NONE;
    }
}
