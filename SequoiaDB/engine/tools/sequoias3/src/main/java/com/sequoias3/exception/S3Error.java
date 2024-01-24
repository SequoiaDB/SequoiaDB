package com.sequoias3.exception;

public enum S3Error {
    UNKNOWN_ERROR(-1, "InternalError", "We encountered an internal error. Please try again."),

    //Acl error
    ACL_CONFLICT(-101, "InvalidRequest", "The x-amz-acl header and x-amz-grant-* and the request body in request body are conflict."),
    ACL_INVALID_ID(-102, "InvalidArgument", "Invalid id."),
    ACL_INVALID_EMAIL(-103, "UnresolvableGrantByEmailAddress", "The e-mail address you provided does not match any account on record."),
    ACL_INVALID_XML(-104, "InvalidXML", "Failed to parse the acl body."),

    ACL_PUT_BUCKET_ACL_FAIL(-110, "PutBucketAclFailed", "Put bucket acl failed."),
    ACL_GET_BUCKET_ACL_FAIL(-111, "GetBucketAclFailed", "Get bucket acl failed."),
    ACL_PUT_OBJECT_ACL_FAIL(-112, "PutObjectAclFailed", "Put object acl failed."),
    ACL_GET_OBJECT_ACL_FAIL(-113, "GetObjectAclFailed", "Get object acl failed."),

    //region error
    REGION_INVALID_REGIONNAME(-300, "InvalidRegionName", "Region name length must between 3 and 20, and cannot contain characters other than 'a-zA-Z0-9' and '-'."),
    REGION_INVALID_LOBPAGESIZE(-301, "InvalidLobPageSize", "LobPageSize is invalid"),
    REGION_INVALID_SHARDINGTYPE(-302, "InvalidShardingType", "Invalid shardingtype"),
    REGION_INVALID_DOMAIN(-303, "InvalidDomain", "Domain not exist."),
    REGION_CONFLICT_TYPE(-305, "ConflictRegionType", "Conflict region type"),
    REGION_CONFLICT_DOMAIN(-306, "ConflictDomain", "Conflict domain"),
    REGION_CONFLICT_LOCATION(-307, "ConflictLocation", "Conflict location"),
    REGION_CONFLICT_LOBPAGESIZE(-308, "ConflictLobPageSize", "Conflict LobPageSize"),
    REGION_CONFLICT_REPLSIZE(-309, "ConflictReplSize", "Conflict ReplSize"),
    REGION_INVALID_REPLSIZE(-310, "InvalidReplSize", "ReplSize is invalid"),

    REGION_NO_SUCH_REGION(-311, "NoSuchRegion", "No such region."),
    REGION_NOT_EMPTY(-312, "RegionNotEmpty", "Region is not empty."),
    REGION_LOCATION_NOT_EXIST(-313, "LocationNotExist", "Location not exist."),

    REGION_PUT_FAILED(-320, "PutRegionFailed", "Put region failed."),
    REGION_GET_FAILED(-321, "GetRegionFailed", "Get region config failed."),
    REGION_DELETE_FAILED(-322, "DeleteRegionFailed", "Delete region failed."),
    REGION_GET_LIST_FAILED(-323, "GetRegionListFailed", "Get region list failed."),
    REGION_HEAD_FAILED(-324, "HeadRegionFailed", "Head region config failed."),

    REGION_LOCATION_NULL(-330, "InvalidLocation", "Location cannot be null"),
    REGION_LOCATION_SAME(-331, "InvalidLocation", "MetaLocation can not same as MetaHisLocation"),
    REGION_LOCATION_SPLIT(-332, "InvalidLocation", "Location cannot be split to CS and CL."),
    REGION_LOCATION_EXIST(-333, "InvalidLocation", "Location not found"),

    REGION_COLLECTION_NOT_EXIST(-340, "RegionCollectionNotExist", "Collection not exist."),

    // dao error
    DAO_GETCONN_ERROR(-401, "GetDBConnectFail", "Get connection failed."),
    DAO_DUPLICATE_KEY(-402, "DuplicateKey", "Duplicate key."),
    DAO_CS_NOT_EXIST(-403, "CSNotExist", "Collection space does not exist."),
    DAO_CL_NOT_EXIST(-404, "CLNotExist", "Collection does not exist."),
    DAO_INSERT_LOB_FAILED(-405, "InsertLobFailed", "Insert Lob Failed."),
    DAO_DB_ERROR(-406, "DBError", "DB error."),
    DAO_LOB_FNE(407, "LobIsNotFound", "Lob is not found."),
    DAO_TRANSACTION_BEGIN_ERROR(-408, "BeginTransactionFailed", "Begin transaction failed."),
    DAO_TRANSACTION_COMMIT_FAILED(-409, "CommitTransactionFailed", "Commit transaction failed."),
    DAO_LOB_PIECES_INFO_OVERFLOW(-410, "WriteLobFailed", "Write lob failed, please try again."),

    //bucket error
    BUCKET_CREATE_FAILED(-500, "CreateBucketFailed", "Create bucket failed."),
    BUCKET_DELETE_FAILED(-501, "DeleteBucketFailed", "Delete bucket failed."),
    BUCKET_GET_SERVICE_FAILED(-502, "GetServiceFailed", "Get service failed."),
    BUCKET_VERSIONING_SET_FAILED(-503, "PutBucketVersioningFailed", "Put bucket versioning failed."),
    BUCKET_VERSIONING_GET_FAILED(-504, "GetBucketVersioningFailed", "Get bucket versioning failed."),
    BUCKET_LOCATION_GET_FAILED(-505, "GetBucketLocationFailed", "Get bucket location failed."),
    BUCKET_DELIMITER_PUT_FAILED(-506, "PutBucketDelimiterFailed", "Put bucket delimiter failed."),
    BUCKET_DELIMITER_GET_FAILED(-507, "GetBucketDelimiterFailed", "Get bucket delimiter failed."),
    BUCKET_DELIMITER_NOT_STABLE(-508, "DelimiterNotStable", "Delimiter is not stable now, please try again later."),

    BUCKET_NOT_EXIST(-510, "NoSuchBucket", "The specified bucket does not exist."),
    BUCKET_INVALID_BUCKETNAME(-511, "InvalidBucketName", "The specified bucket name is invalid."),
    BUCKET_ALREADY_EXIST(-512, "BucketAlreadyExists", "The requested bucket name is not available. The bucket namespace is shared by all users of the system. Please select a different name and try again."),
    BUCKET_ALREADY_OWNED_BY_YOU(-513, "BucketAlreadyOwnedByYou", "Your previous request to create the named bucket succeeded and you already own it."),
    BUCKET_NOT_EMPTY(-514, "BucketNotEmpty", "The bucket you tried to delete is not empty."),
    BUCKET_TOO_MANY_BUCKETS(-515, "TooManyBuckets", "You have attempted to create more buckets than allowed."),
    BUCKET_INVALID_VERSIONING_STATUS(-516, "InvalidVersioningStatus", "The versioning status is invalid."),
    BUCKET_INVALID_LOCATION(-517, "InvalidLocation", "The location is invalid."),

    //object
    OBJECT_WRITE_fAILED(-600, "WriteObjectFailed", "Put object failed."),
    OBJECT_PUT_fAILED(-601, "PutObjectFailed", "Put object failed."),
    OBJECT_GET_FAILED(-602, "GetObjectFailed", "Get object failed"),
    OBJECT_DELETE_FAILED(-603, "DeleteObjectFailed", "Delete object failed."),
    OBJECT_LIST_FAILED(-604, "ListObjectsFailed", "List objects failed."),
    OBJECT_LIST_VERSIONS_FAILED(-605, "ListVersionsFailed", "List versions failed."),
    OBJECT_LIST_V1_FAILED(-606, "ListObjectsV1Failed", "List objects V1 failed."),
    OBJECT_COPY_FAILED(-607, "CopyObjectFailed", "Copy object failed"),

    OBJECT_INVALID_KEY(-611, "InvalidKey", "Invalid Key."),
    OBJECT_KEY_TOO_LONG(-612, "KeyTooLongError", "Your key is too long."),
    OBJECT_METADATA_TOO_LARGE(-613, "MetadataTooLarge", "Your metadata headers exceed the maximum allowed metadata size."),
    OBJECT_NO_SUCH_KEY(-614, "NoSuchKey", "The specified key does not exist."),
    OBJECT_BAD_DIGEST(-615, "BadDigest", "The Content-MD5 you specified did not match what we received."),
    OBJECT_INVALID_ENCODING_TYPE(-616, "InvalidArgument", "Invalid Encoding Method specified in Request"),
    OBJECT_INVALID_TOKEN(-617, "InvalidArgument", "The continuation token provided is incorrect."),
    OBJECT_IS_IN_USE(-618, "ObjectIsInUse", "The object is in use."),
    OBJECT_RANGE_NOT_SATISFIABLE(-619, "InvalidRange", "Requested range not satisfiable."),
    OBJECT_INVALID_DIGEST(-620, "InvalidDigest", " The Content-MD5 you specified is not valid."),
    OBJECT_NO_SUCH_VERSION(-621, "NoSuchVersion", "The specified version does not exist."),
    OBJECT_INVALID_VERSION(-622, "InvalidArgument", "Invalid version id specified"),
    OBJECT_INVALID_RANGE(-623, "InvalidArgument", "Invalid range."),
    OBJECT_INCOMPLETE_BODY(-624, "IncompleteBody", "You did not provide the number bytes specified by the Content-Length HTTP header."),

    OBJECT_IF_MODIFIED_SINCE_FAILED(-631, "NotModified", "If-Modified-Since not match"),
    OBJECT_IF_UNMODIFIED_SINCE_FAILED(-632, "PreconditionFailed", "If-Unmodified-Since not match"),
    OBJECT_IF_MATCH_FAILED(-633, "PreconditionFailed", "If-Match not match"),
    OBJECT_IF_NONE_MATCH_FAILED(-634, "NotModified", "If-None-Match not match"),
    OBJECT_INVALID_TIME(-635, "InvalidArgument", "Time is invalid"),

    PART_INIT_MULTIPART_UPLOAD_FAILED(-641, "InitMultipartUploadFailed", "Init multipart upload failed."),
    PART_UPLOAD_PART_FAILED(-642, "UploadPartFailed", "Upload part failed."),
    PART_LIST_PARTS_FAILED(-643, "ListPartsFailed", "List parts failed."),
    PART_COMPLETE_MULTIPART_UPLOAD_FAILED(-644, "CompleteMultipartUploadFailed", "Complete multipart upload failed."),
    PART_ABORT_MULTIPART_UPLOAD_FAILED(-645, "AbortMultipartUploadFailed", "Abort multipart upload failed."),
    PART_LIST_MULTIPART_UPLOADS_FAILED(-646, "ListMultipartUploadsFailed", "List multipart uploads failed."),

    PART_NO_SUCH_UPLOAD(-651, "NoSuchUpload", "The specified multipart upload does not exist. The upload ID might be invalid, or the multipart upload might have been aborted or completed."),
    PART_ENTITY_TOO_SMALL(-652, "EntityTooSmall", "Your proposed upload is smaller than the minimum allowed object size. Each part must be at least 5 MB in size, except the last part."),
    PART_INVALID_PART(-653, "InvalidPart", "One or more of the specified parts could not be found. The part might not have been uploaded, or the specified entity tag might not have matched the part's entity tag."),
    PART_INVALID_PARTORDER(-654, "InvalidPartOrder", "The list of parts was not in ascending order. Parts list must be specified in order by part number."),
    PART_INVALID_PARTNUMBER(-655, "InvalidPartNumber", "Part number must be an integer between 1 and 10000, inclusive."),
    PART_COMPLETING_CONFLICT(-656, "UploadConflict", "The uploadId is completing, upload part is forbidden."),
    PART_ENTITY_TOO_LARGE(-657, "EntityTooLarge", "Your proposed upload is larger than the maximum allowed object size. Each part must be not more 5 GB in size, except the last part."),

    OBJECT_COPY_INVALID_DIRECTIVE(-661, "InvalidArgument", "Unknown metadata directive."),
    OBJECT_COPY_WITHOUT_CHANGE(-662, "InvalidRequest", "This copy request is illegal because it is trying to copy an object to itself without changing the object's metadata."),
    OBJECT_COPY_DELETE_MARKER(-663, "InvalidRequest", "The source of a copy request may not specifically refer to a delete marker by version id."),
    OBJECT_COPY_INVALID_SOURCE(-664, "InvalidArgument", "Copy source must mention the source bucket and key: sourcebucket/sourcekey."),
    OBJECT_COPY_INVALID_DEST(-665, "InvalidArgument", "You can only specify a copy source header for copy requests."),

    //authorization
    INVALID_ADMINISTRATOR(-701, "AccessDenied", "Non-admin users cannot do this operator."),
    INVALID_ACCESSKEYID(-702, "InvalidAccessKeyId", "The AWS access key ID you provided does not exist in our records."),
    SIGNATURE_NOT_MATCH(-703, "SignatureDoesNotMatch", "Signature does not match."),
    ACCESS_DENIED(-704, "AccessDenied", "Access Denied."),
    NO_CREDENTIALS(-705, "CredentialsNotSupported", "This request does not support credentials."),
    INVALID_AUTHORIZATION(-707, "AuthorizationHeaderMalformed", "The authorization header you provided is invalid."),

    // User
    USER_NOT_EXIST(-800, "NoSuchUser", "User not exist."),
    USER_CREATE_FAILED(-801, "AddUserFailed", "Create user failed."),
    USER_CREATE_NAME_INVALID(-802, "InvalidUserName", "The username is invalid."),
    USER_CREATE_ROLE_INVALID(-803, "InvalidRole", "The role is invalid."),
    USER_CREATE_EXIST(-804, "UserAlreadyExists", "The username is exist."),
    USER_DELETE_FAILED(-810, "DelUserFailed", "Delete user failed."),
    USER_DELETE_INIT_ADMIN(-811, "InitAdminCannotDelete", "Init admin user cannot be delete."),
    USER_DELETE_CLEAN_FAILED(-812, "CleanResourceFailed", "Clean resource failed, please try again."),
    USER_DELETE_RELEASE_RESOURCE(-813, "BucketMustRelease", "Please delete your bucket before delete user, or delete user force."),
    USER_UPDATE_FAILED(-820, "UpdateUserFailed", "Update user failed."),
    USER_GET_FAILED(-830, "GetUserFailed", "Get user failed."),

    INVALID_ARGUMENT(-900, "InvalidArgument", "Invalid argument."),
    METHOD_NOT_ALLOWED(-901, "MethodNotAllowed", "The specified method is not allowed against this resource."),
    MALFORMED_XML(-902, "MalformedXML", "The XML you provided was not well-formed or did not validate against our published schema."),
    NEED_A_KEY(-903, "InvalidRequest", "A key must be specified."),
    PARAMETER_NOT_SUPPORT(-904, "ParameterNotAllowed", "The specified parameter is not supported."),
    ACCESS_EXPIRED(-905, "AccessDenied", "Request has expired"),
    OPERATION_CONFLICT(-906, "OperationAborted", "A conflicting conditional operation is currently in progress against this resource. Try again.");


    private int errIndex;
    private String code;
    private String message;

    S3Error(int errIndex, String code, String desc) {
        this.errIndex = errIndex;
        this.code = code;
        this.message = desc;
    }

    public int getErrIndex() {
        return errIndex;
    }

    public String getErrorMessage() {
        return message;
    }

    public String getCode() {
        return code;
    }

    public String getErrorType() {
        return name();
    }

    @Override
    public String toString() {
        return name() + "(" + this.code + ")" + ":" + this.message;
    }

    public static S3Error getS3Error(int errorIndex) {
        for (S3Error value : S3Error.values()) {
            if (value.getErrIndex() == errorIndex) {
                return value;
            }
        }

        return UNKNOWN_ERROR;
    }
}