package com.sequoias3.utils;

import com.sequoias3.common.InitAdminUserDefine;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.config.AuthorizationConfig;
import com.sequoias3.core.Range;
import com.sequoias3.core.User;
import com.sequoias3.dao.UserDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Component;

import javax.servlet.http.HttpServletRequest;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.Enumeration;
import java.util.Map;

@Component
public class RestUtils {
    private static final Logger logger = LoggerFactory.getLogger(RestUtils.class);

    @Autowired
    UserDao userDao;

    @Autowired
    AuthorizationConfig authConfig;

    public User getOperatorByAuthorization(String authorization) throws S3ServerException {
        //       1.get access key id
        String accessKeyId = null;
        if (authConfig.isCheck()) {
            if (authorization == null){
                throw new S3ServerException(S3Error.INVALID_AUTHORIZATION, "authorization is null");
            }

            int beginIndex = authorization.indexOf(RestParamDefine.REST_CREDENTIAL);
            if (-1 != beginIndex) {
                int endIndex = authorization.indexOf(RestParamDefine.REST_DELIMITER, beginIndex);
                if (endIndex != -1) {
                    accessKeyId = authorization.substring(beginIndex + RestParamDefine.REST_CREDENTIAL.length(), endIndex);
                } else {
                    throw new S3ServerException(S3Error.INVALID_AUTHORIZATION, "authorization is invalid. authorization="+authorization);
                }
            }else {
                int beginIndexV2 = authorization.indexOf(RestParamDefine.REST_AWS);
                if (-1 == beginIndexV2){
                    throw new S3ServerException(S3Error.INVALID_AUTHORIZATION, "authorization is invalid. authorization = " + authorization);
                }

                String accessKeys = authorization.substring(beginIndexV2+RestParamDefine.REST_AWS.length());

                String[] keys = accessKeys.split(":");

                if (keys.length < 2){
                    throw new S3ServerException(S3Error.INVALID_AUTHORIZATION, "no aws accesskeyId. authorization:"+authorization);
                }

                accessKeyId = keys[0].trim();
            }
        }else {
            return userDao.getUserByName(InitAdminUserDefine.ADMIN_NAME);
        }

        //       2.check access key
        User user = userDao.getUserByAccessKeyID(accessKeyId);
        if (null == user) {
            throw new S3ServerException(S3Error.INVALID_ACCESSKEYID,
                    "Invalid accessKeyId. accessKeyId = " + accessKeyId);
        }

        //       3.check signature

        return user;
    }

    public String getObjectNameByURI(String uri) throws S3ServerException {
        String decodeUrl;
        try {
            decodeUrl = URLDecoder.decode(uri, "UTF-8");
        }catch (UnsupportedEncodingException e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_KEY, "Invalid key. uri = " + uri);
        }

        int beginObject = decodeUrl.indexOf(RestParamDefine.REST_DELIMITER, 1);
        if (beginObject == -1) {
            throw new S3ServerException(S3Error.OBJECT_INVALID_KEY, "Invalid key. uri = " + uri);
        }

        String objectName = decodeUrl.substring(beginObject+1);
        if (objectName.length() == 0){
            throw new S3ServerException(S3Error.NEED_A_KEY, "Invalid key.uri = " + uri);
        }
        return objectName;
    }

    public Range getRange(String rangeHeader){
        try {
            return new Range(rangeHeader);
        }catch (S3ServerException e){
            logger.error("get range fail. range:{}", rangeHeader);
            return null;
        }
    }

    public void getHeaders(HttpServletRequest httpServletRequest,
                           Map<String, String> requestHeaders, Map<String, String> xMeta ){
        Enumeration names = httpServletRequest.getHeaderNames();
        while (names.hasMoreElements()){
            String name = names.nextElement().toString();
            if (name.startsWith(RestParamDefine.PutObjectHeader.X_AMZ_META_PREFIX)){
                xMeta.put(name,httpServletRequest.getHeader(name));
            }else {
                requestHeaders.put(name, httpServletRequest.getHeader(name));
            }
        }
    }

    public long convertUploadId(String uploadId) throws S3ServerException{
        try{
            return Long.parseLong(uploadId);
        }catch (Exception e){
            throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD,
                    "uploadId is invalid, uploadId:"+ uploadId, e);
        }
    }

    public int convertPartNumber(String partNumber) throws S3ServerException{
        try{
            return Integer.parseInt(partNumber);
        }catch (Exception e){
            throw new S3ServerException(S3Error.PART_INVALID_PARTNUMBER,
                    "partNumber is invalid, partNumber:"+ partNumber, e);
        }
    }

    public HttpStatus convertStatus(S3ServerException e){
        HttpStatus status;
        switch (e.getError()) {
            case OBJECT_IF_NONE_MATCH_FAILED:
            case OBJECT_IF_MODIFIED_SINCE_FAILED:
                status = HttpStatus.NOT_MODIFIED;
                break;
            case INVALID_ARGUMENT:
            case USER_CREATE_NAME_INVALID:
            case USER_CREATE_ROLE_INVALID:
            case BUCKET_INVALID_BUCKETNAME:
            case BUCKET_TOO_MANY_BUCKETS:
            case BUCKET_INVALID_VERSIONING_STATUS:
            case OBJECT_KEY_TOO_LONG:
            case OBJECT_METADATA_TOO_LARGE:
            case OBJECT_INVALID_TOKEN:
            case OBJECT_BAD_DIGEST:
            case OBJECT_INVALID_KEY:
            case OBJECT_INVALID_DIGEST:
            case OBJECT_INVALID_VERSION:
            case OBJECT_INVALID_RANGE:
            case REGION_INVALID_SHARDINGTYPE:
            case REGION_INVALID_LOBPAGESIZE:
            case REGION_INVALID_REPLSIZE:
            case REGION_LOCATION_NULL:
            case REGION_LOCATION_SAME:
            case REGION_LOCATION_SPLIT:
            case REGION_LOCATION_EXIST:
            case REGION_INVALID_DOMAIN:
            case REGION_INVALID_REGIONNAME:
            case PART_ENTITY_TOO_SMALL:
            case PART_ENTITY_TOO_LARGE:
            case PART_INVALID_PART:
            case PART_INVALID_PARTORDER:
            case PART_INVALID_PARTNUMBER:
            case OBJECT_COPY_WITHOUT_CHANGE:
            case OBJECT_COPY_INVALID_DIRECTIVE:
            case OBJECT_COPY_DELETE_MARKER:
            case OBJECT_COPY_INVALID_SOURCE:
            case OBJECT_INCOMPLETE_BODY:
            case ACL_CONFLICT:
            case ACL_INVALID_ID:
            case ACL_INVALID_EMAIL:
            case MALFORMED_XML:
            case NEED_A_KEY:
            case OBJECT_INVALID_ENCODING_TYPE:
            case PARAMETER_NOT_SUPPORT:
            case OBJECT_COPY_INVALID_DEST:
                status = HttpStatus.BAD_REQUEST;
                break;
            case INVALID_ACCESSKEYID:
            case INVALID_ADMINISTRATOR:
            case USER_DELETE_INIT_ADMIN:
            case SIGNATURE_NOT_MATCH:
            case ACCESS_DENIED:
            case NO_CREDENTIALS:
            case INVALID_AUTHORIZATION:
                status = HttpStatus.FORBIDDEN;
                break;
            case USER_NOT_EXIST:
            case BUCKET_NOT_EXIST:
            case OBJECT_NO_SUCH_KEY:
            case OBJECT_NO_SUCH_VERSION:
            case REGION_NO_SUCH_REGION:
            case PART_NO_SUCH_UPLOAD:
                status = HttpStatus.NOT_FOUND;
                break;
            case METHOD_NOT_ALLOWED:
                status = HttpStatus.METHOD_NOT_ALLOWED;
                break;
            case USER_CREATE_EXIST:
            case BUCKET_ALREADY_EXIST:
            case BUCKET_ALREADY_OWNED_BY_YOU:
            case BUCKET_NOT_EMPTY:
            case OBJECT_IS_IN_USE:
            case REGION_NOT_EMPTY:
            case REGION_CONFLICT_TYPE:
            case REGION_CONFLICT_DOMAIN:
            case REGION_CONFLICT_LOCATION:
            case REGION_CONFLICT_LOBPAGESIZE:
            case REGION_CONFLICT_REPLSIZE:
            case BUCKET_DELIMITER_NOT_STABLE:
                status = HttpStatus.CONFLICT;
                break;
            case OBJECT_IF_MATCH_FAILED:
            case OBJECT_IF_UNMODIFIED_SINCE_FAILED:
                status = HttpStatus.PRECONDITION_FAILED;
                break;
            case OBJECT_RANGE_NOT_SATISFIABLE:
                status = HttpStatus.REQUESTED_RANGE_NOT_SATISFIABLE;
                break;
            default:
                status = HttpStatus.INTERNAL_SERVER_ERROR;
        }

        return status;
    }

    public int getXMetaLength(Map<String, String> xMeta){
        if (xMeta == null){
            return 0;
        }

        int length = 0;
        int prefixLength = RestParamDefine.PutObjectHeader.X_AMZ_META_PREFIX.length();
        for (Map.Entry<String, String> entry : xMeta.entrySet()){
            String headerName = entry.getKey();
            String headerValue = entry.getValue();
            if(headerName.startsWith(RestParamDefine.PutObjectHeader.X_AMZ_META_PREFIX)){
                length += (headerName.length()-prefixLength);
                length += headerValue.length();
            }
        }

        return length;
    }
}
