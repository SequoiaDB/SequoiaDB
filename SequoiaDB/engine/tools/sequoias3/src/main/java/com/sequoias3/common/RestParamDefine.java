package com.sequoias3.common;

public class RestParamDefine {
    public static final String AUTHORIZATION           = "authorization";

    public static final String ACL                     = "acl";
    public static final String VERSIONS                = "versions";
    public static final String VERSION_ID              = "versionId";
    public static final String VERSIONING              = "versioning";
    public static final String LOCATION                = "location";
    public static final String DELIMITER               = "delimiter-config";

    public static final String UPLOADS                 = "uploads";
    public static final String UPLOADID                = "uploadId";
    public static final String PARTNUMBER              = "partNumber";
    public static final String DELETE                  = "delete";

    public static final int    MAX_KEYS_DEFAULT        = 1000;

    public static class Attribute{
        public static final String S3_OPERATOR         = "s3Operator";
        public static final String S3_BUCKET           = "s3Bucket";
        public static final String S3_OBJECTURI        = "s3ObjectUri";
        public static final String S3_PARTNUMBER       = "s3PartNumber";
        public static final String S3_UPLOADID         = "s3UploadId";
        public static final String S3_HEADERS          = "s3Headers";
        public static final String S3_XMETA            = "s3XMeta";
    }
    public static class UserPara{
        public static final String CREATE_USER             = "Action=CreateUser";
        public static final String CREATE_ACCESSKEY        = "Action=CreateAccessKey";
        public static final String GET_ACCESSKEY           = "Action=GetAccessKey";
        public static final String DELETE_USER             = "Action=DeleteUser";
        public static final String USER_NAME               = "UserName";
        public static final String ROLE                    = "Role";
        public static final String FORCE                   = "Force";
    }

    public static class RegionPara{
        public static final String CREATE_REGION           = "Action=CreateRegion";
        public static final String GET_REGION              = "Action=GetRegion";
        public static final String HEAD_REGION             = "Action=HeadRegion";
        public static final String DELETE_REGION           = "Action=DeleteRegion";
        public static final String REGION_NAME             = "RegionName";
        public static final String LIST_REGIONS            = "Action=ListRegions";
    }

    public static class CommonPara{
        public static final String DATE                 = "date";
        public static final String X_AMZ_ALGORITHM      = "X-Amz-Algorithm";
        public static final String X_AMZ_DATE           = "X-Amz-Date";
        public static final String X_AMZ_SIGNEDHEADERS  = "X-Amz-SignedHeaders";
        public static final String X_AMZ_EXPIRES        = "X-Amz-Expires";
        public static final String X_AMZ_CREDENTIAL     = "X-Amz-Credential";
        public static final String X_AMZ_SIGNATURE      = "X-Amz-Signature";
        public static final String SIGNATURE            = "Signature";
        public static final String EXPIRES              = "Expires";
        public static final String ACCESS_KEYID         = "AWSAccessKeyId";
    }

    public static class ListObjectsPara{
        public static final String LIST_TYPE2              = "list-type=2";
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String CONTINUATIONTOKEN       = "continuation-token";
        public static final String START_AFTER             = "start-after";
        public static final String MAX_KEYS                = "max-keys";
        public static final String ENCODING_TYPE           = "encoding-type";
        public static final String FETCH_OWNER             = "fetch-owner";
    }

    public static class ListObjectsV1Para{
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String MARKER                  = "marker";
        public static final String MAX_KEYS                = "max-keys";
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static class ListObjectVersionsPara{
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String KEY_MARKER              = "key-marker";
        public static final String VERSION_ID_MARKER       = "version-id-marker";
        public static final String MAX_KEYS                = "max-keys";
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static class PutObjectHeader {
        public static final String CONTENT_LENGTH       = "content-length";
        public static final String CONTENT_MD5          = "content-md5";
        public static final String CONTENT_ENCODING     = "content-encoding";
        public static final String CONTENT_TYPE         = "content-type";
        public static final String X_AMZ_META_PREFIX    = "x-amz-meta-";
        public static final String CACHE_CONTROL        = "cache-control";
        public static final String CONTENT_DISPOSITION  = "content-disposition";
        public static final String EXPIRES              = "expires";
        public static final String CONTENT_LANGUAGE     = "content-language";
    }

    public static class PutObjectResultHeader {
        public static final String ETAG         = "ETag";
        public static final String VERSION_ID   = "x-amz-version-id";
    }

    public static class CopyObjectHeader {
        public static final String X_AMZ_COPY_SOURCE    = "x-amz-copy-source";
        public static final String METADATA_DIRECTIVE   = "x-amz-metadata-directive";
        public static final String IF_MODIFIED_SINCE    = "x-amz-copy-source-if-modified-since";
        public static final String IF_UNMODIFIED_SINCE  = "x-amz-copy-source-if-unmodified-since";
        public static final String IF_MATCH             = "x-amz-copy-source-if-match";
        public static final String IF_NONE_MATCH        = "x-amz-copy-source-if-none-match";
    }

    public static class CopyObjectResultHeader {
        public static final String VERSION_ID          = "x-amz-version-id";
        public static final String SOURCE_VERSION_ID   = "x-amz-copy-source-version-id";
    }

    public static class GetObjectReqPara{
        public static final String RES_CONTENT_TYPE         = "response-content-type";
        public static final String RES_CONTENT_LANGUAGE     = "response-content-language";
        public static final String RES_EXPIRES              = "response-expires";
        public static final String RES_CACHE_CONTROL        = "response-cache-control";
        public static final String RES_CONTENT_DISPOSITION  = "response-content-disposition";
        public static final String RES_CONTENT_ENCODING     = "response-content-encoding";
    }

    public static class GetObjectReqHeader{
        public static final String REQ_RANGE                = "range";
        public static final String REQ_IF_MODIFIED_SINCE    = "if-modified-since";
        public static final String REQ_IF_UNMODIFIED_SINCE  = "if-unmodified-since";
        public static final String REQ_IF_MATCH             = "if-match";
        public static final String REQ_IF_NONE_MATCH        = "if-none-match";
    }

    public static class GetObjectResHeader{
        public static final String CONTENT_ENCODING        = "Content-Encoding";
        public static final String CONTENT_TYPE            = "Content-Type";
        public static final String CACHE_CONTROL           = "Cache-Control";
        public static final String CONTENT_DISPOSITION     = "Content-Disposition";
        public static final String EXPIRES                 = "Expires";
        public static final String CONTENT_LANGUAGE        = "Content-Language";
        public static final String LAST_MODIFIED           = "Last-Modified";
        public static final String CONTENT_LENGTH          = "Content-Length";
        public static final String ETAG                    = "ETag";
        public static final String CONTENT_RANGE           = "Content-Range";
        public static final String VERSION_ID              = "x-amz-version-id";
        public static final String DELETE_MARKER           = "x-amz-delete-marker";
        public static final String ACCEPT_RANGES           = "Accept-Ranges";
    }

    public static class DeleteObjectResultHeader {
        public static final String  VERSION_ID    = "x-amz-version-id";
        public static final String  DELETE_MARKER = "x-amz-delete-marker";
    }

    public static class HeadBucketResultHeader {
        public static final String REGION = "x-amz-bucket-region";
    }

    public static class ListPartsPara{
        public static final String PART_NUMBER_MARKER      = "part-number-marker";
        public static final String MAX_PARTS               = "max-parts";
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static class ListUploadsPara{
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String KEY_MARKER              = "key-marker";
        public static final String UPLOAD_ID_MARKER        = "upload-id-marker";
        public static final String MAX_UPLOADS             = "max-uploads";
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static class Acl{
        public static final String X_AMZ_ACL               = "x-amz-acl";

        public static final String X_AMZ_GRANT_READ        = "x-amz-grant-read";
        public static final String X_AMZ_GRANT_WRITE       = "x-amz-grant-write";
        public static final String X_AMZ_GRANT_READ_ACP    = "x-amz-grant-read-acp";
        public static final String X_AMZ_GRANT_WRITE_ACP   = "x-amz-grant-write-acp";
        public static final String X_AMZ_GRANT_FULL_CONTROL= "x-amz-grant-full-control";

        public static final String ACL_FULLCONTROL         = "FULL_CONTROL";
        public static final String ACL_READ                = "READ";
        public static final String ACL_WRITE               = "WRITE";
        public static final String ACL_READ_ACP            = "READ_ACP";
        public static final String ACL_WRITE_ACP           = "WRITE_ACP";

        public static final String TYPE_GROUP              = "Group";
        public static final String TYPE_USER               = "CanonicalUser";
        public static final String TYPE_EMAIL              = "AmazonCustomerByEmail";

        public static final String ALLUSERS                = "http://acs.amazonaws.com/groups/global/AllUsers";
        public static final String AUTHENTICATEDUSERS      = "http://acs.amazonaws.com/groups/global/AuthenticatedUsers";
        public static final String LOGDELIVERY             = "http://acs.amazonaws.com/groups/s3/LogDelivery";

        public static final String CANNED_PRIVATE                = "private";
        public static final String CANNED_PUBLICREAD             = "public-read";
        public static final String CANNED_PUBLICREADWRITE        = "public-read-write";
        public static final String CANNED_AUTHENTICATEDREAD      = "authenticated-read";
        public static final String CANNED_LOGDELIVERYWRITE       = "log-delivery-write";
        public static final String CANNED_BUCKETOWNERREAD        = "bucket-owner-read";
        public static final String CANNED_BUCKETOWNERFULLCONTROL = "bucket-owner-full-control";

        public static final String VERSION_ID                    = "x-amz-version-id";
    }

    public static class DelimiterHeader{
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static final String EXPECT               = "Expect";

    public static final int PART_NUMBER_MIN         = 1;
    public static final int PART_NUMBER_MAX         = 10000;

    public static final int KEY_LENGTH              = 900;
    public static final int X_AMZ_META_LENGTH       = 2*1024;

    public static final String REST_CREDENTIAL      = "Credential=";
    public static final String REST_AWS             = "AWS";
    public static final String REST_SOURCE_VERSIONID= "?versionId=";

    public static final String REST_DELIMITER       = "/";
    public static final String REST_S3              = "/s3";
    public static final String REST_USERS           = "/users";
    public static final String REST_REGION          = "/region";

    public static final String REST_RANGE_START     = "bytes=";
    public static final String REST_HYPHEN          = "-";

    public static final String ENCODING_TYPE_URL    = "url";

    public static final String REST_DIRECTIVE_COPY     = "COPY";
    public static final String REST_DIRECTIVE_REPLACE  = "REPLACE";
}
