package com.sequoias3.core;

public class CommonDefine {
    // request headers
    public static final String AUTHORIZATION = "Authorization";
    public static final String X_AMZ_CONTENT_SHA256 = "x-amz-content-sha256";

    /** request path */
    public static final String PATH_REGION = "/region/";

    /** request parameters */
    public static final String ACTION = "Action";
    public static final String REGION_NAME = "RegionName";

    /** values of parameter "Action" */
    public static final String CREATE_REGION = "CreateRegion";
    public static final String DELETE_REGION = "DeleteRegion";
    public static final String GET_REGION = "GetRegion";
    public static final String HEAD_REGION = "HeadRegion";
    public static final String LIST_REGION = "ListRegions";

    /** request method */
    public static final String HTTP_METHOD_POST = "POST";

    /** S3 service */
    public static final String SERVICE_NAME = "s3";
}
