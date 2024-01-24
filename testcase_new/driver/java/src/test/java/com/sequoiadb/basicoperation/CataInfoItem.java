package com.sequoiadb.basicoperation;

public class CataInfoItem {
    public String groupName;
    public int lowBound;
    public int upBound;

    public boolean Compare( String name, int low, int up ) {
        return name.equals( groupName ) && low == lowBound && up == upBound;
    }

    public String toString() {
        return "groupName : " + groupName + " lowBound: {'':" + lowBound + "}"
                + " upBound:{'':" + upBound + "}";
    }
}
