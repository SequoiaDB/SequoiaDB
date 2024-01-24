package com.sequoiadb.testdata;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class HaveMapPropClass {
    private Map<String, String> mapProp = null;
    private Map<String, User> userMap = null;

    public HaveMapPropClass() {
        mapProp = new HashMap<String, String>();
    }

    public Map<String, String> getMapProp() {
        return mapProp;
    }

    public void setMapProp(Map<String, String> value) {
        this.mapProp = value;
    }

    public Map<String, User> getUserMap() {
        return this.userMap;
    }

    public void setUserMap(Map<String, User> userMap) {
        this.userMap = userMap;
    }

    @Override
    public String toString() {
        return "HaveMapPropClass [mapProp=" + mapProp + ", userMap=" + userMap
            + "]";
    }

    @Override
    public int hashCode() {
        Object[] objects = new Object[]{mapProp, userMap};
        return Arrays.hashCode(objects);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof HaveMapPropClass) {
            if (this == obj) {
                return true;
            }

            HaveMapPropClass other = (HaveMapPropClass) obj;
            return this.mapProp.equals(other.mapProp) && this.userMap.equals(other.userMap);
        } else {
            return false;
        }
    }
}
