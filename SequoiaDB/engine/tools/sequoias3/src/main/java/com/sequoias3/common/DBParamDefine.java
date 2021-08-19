package com.sequoias3.common;

public class DBParamDefine {
    public static final int DB_DUPLICATE_MAX_TIME = 10000;

    public static final String DB_AUTO_DELIMITER = "/";

    public static final String CS_S3   = "S3_";
    public static final String CS_META = "_MetaCS";
    public static final String CS_DATA = "_DataCS";

    public static final String MODIFY_SET   = "$set";
    public static final String MODIFY_UNSET = "$unset";
    public static final String GREATER      = "$gt";
    public static final String NOT_SMALL    = "$gte";
    public static final String LESS_THAN    = "$lt";
    public static final String INCREASE     = "$inc";
    public static final String IN           = "$in";
    public static final String OR           = "$or";
    public static final String NOT_EQUAL    = "$ne";
    public static final String IS_NULL      = "$isnull";

    public static final int    CREATE_OK     = 1;
    public static final int    CREATE_EXIST  = 2;
}
