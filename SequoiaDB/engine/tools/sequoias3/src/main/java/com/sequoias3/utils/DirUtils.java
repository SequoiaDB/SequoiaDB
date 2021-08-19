package com.sequoias3.utils;


public class DirUtils {

    public static String getDir(String objectName, String delimiter){
        int index = objectName.lastIndexOf(delimiter);
        if (index != -1){
            String dir = objectName.substring(0, index + delimiter.length());
            return dir;
        }else {
            return null;
        }
    }
}
