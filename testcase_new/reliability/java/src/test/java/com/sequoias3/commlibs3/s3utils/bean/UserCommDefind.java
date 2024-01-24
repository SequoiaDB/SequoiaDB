package com.sequoias3.commlibs3.s3utils.bean;

public class UserCommDefind {
    // request headers
    public final static String authorization = "Authorization";
    public final static String authValPre = "Credential=";
    /*
     * public final static String content_length = "Content-Length"; public
     * final static String content_Type = "Content-Type"; public final static
     * String host = "Host"; public final static String expect = "Expect";
     */

    public final static String admin = "admin";
    public final static String normal = "normal";

    // response normal
    public final static String accessKeys = "AccessKeys";
    public final static String accessKeyID = "AccessKeyID";
    public final static String secretAccessKey = "SecretAccessKey";

    // response error
    public final static String error = "Error";
    public final static String errorCode = "Code";
    public final static String errorMesg = "Message";
    public final static String errorRes = "Resource";
}
