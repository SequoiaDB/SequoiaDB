package com.sequoias3.core;

public class RegionSpace {
    public static final String REGION_SPACE_NAME          = "Name";
    public static final String REGION_SPACE_REGIONNAME    = "RegionName";
    public static final String REGION_SPACE_CREATETIME    = "CreateTime";

    private String name;

    private String regionName;

    private long createTime;

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setRegionName(String regionName) {
        this.regionName = regionName;
    }

    public String getRegionName() {
        return regionName;
    }

    public void setCreateTime(long createTime) {
        this.createTime = createTime;
    }

    public long getCreateTime() {
        return createTime;
    }
}
