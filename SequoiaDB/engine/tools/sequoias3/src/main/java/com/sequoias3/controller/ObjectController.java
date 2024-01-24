package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.*;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import com.sequoias3.utils.DataFormatUtils;
import com.sequoias3.utils.MD5Utils;
import com.sequoias3.utils.RestUtils;
import org.apache.commons.codec.binary.Hex;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.util.*;

@RestController
public class ObjectController {
    private final Logger logger = LoggerFactory.getLogger(ObjectController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    ObjectService objectService;

    @Autowired
    BucketService bucketService;

    @PutMapping(value="/{bucketname:.+}/**", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putObject(@PathVariable("bucketname") String bucketName,
                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                    @RequestHeader(name = RestParamDefine.PutObjectHeader.CONTENT_MD5, required = false) String contentMD5,
                                    HttpServletRequest httpServletRequest)
            throws S3ServerException, IOException {
        try {
            User operator;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_OPERATOR) == null) {
                operator = restUtils.getOperatorByAuthorization(authorization);
            }else {
                operator = (User) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_OPERATOR);
            }

            String objectName;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_OBJECTURI) == null) {
                objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
                //check key length
                if (objectName.length() > RestParamDefine.KEY_LENGTH) {
                    throw new S3ServerException(S3Error.OBJECT_KEY_TOO_LONG,
                            "ObjectName is too long. objectName:" + objectName);
                }
            } else {
                objectName = ((ObjectUri) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_OBJECTURI)).getObjectName();
            }
            logger.debug("put object. bucketName={}, objectName={}", bucketName, objectName);

            Map<String, String> requestHeaders;
            Map<String, String> xMeta;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_HEADERS) != null
                    && httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_XMETA) != null){
                requestHeaders = (Map<String, String>)httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_HEADERS);
                xMeta = (Map<String, String>) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_XMETA);
            } else {
                requestHeaders = new HashMap<>();
                xMeta = new HashMap<>();
                restUtils.getHeaders(httpServletRequest, requestHeaders, xMeta);
                if (restUtils.getXMetaLength(xMeta) > RestParamDefine.X_AMZ_META_LENGTH) {
                    throw new S3ServerException(S3Error.OBJECT_METADATA_TOO_LARGE,
                            "metadata headers exceed the maximum. xMeta:" + xMeta.toString());
                }
            }

            //get and check bucket
            Bucket bucket;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_BUCKET) == null) {
                bucket = bucketService.getBucket(operator.getUserId(), bucketName);
            } else {
                bucket = (Bucket) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_BUCKET);
            }

            InputStream body;
            Long realContenLength = 0L;
            if (httpServletRequest.getHeader("x-amz-decoded-content-length") != null) {
                body = new S3InputStreamReaderChunk(httpServletRequest.getInputStream());
                realContenLength = Long.parseLong(httpServletRequest.getHeader("x-amz-decoded-content-length"));
            } else {
                body = httpServletRequest.getInputStream();
                if (httpServletRequest.getHeader("content-length") != null) {
                    realContenLength = Long.parseLong(httpServletRequest.getHeader("content-length"));
                }
            }

            PutDeleteResult result = objectService.putObject(bucket,
                    objectName,
                    contentMD5,
                    requestHeaders,
                    xMeta,
                    body,
                    realContenLength);

            HttpHeaders headers = new HttpHeaders();
            if (result.geteTag() != null) {
                headers.add(RestParamDefine.PutObjectResultHeader.ETAG,
                        "\"" + result.geteTag() + "\"");
            }
            if (result.getVersionId() != null) {
                headers.add(RestParamDefine.PutObjectResultHeader.VERSION_ID,
                        result.getVersionId());
            }

            logger.debug("put object success. bucketName={}, objectName={}, versionId={}, eTag={}", bucketName, objectName, result.getVersionId(), result.geteTag());
            return ResponseEntity.ok()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("put object failed. bucketName/objectName:" + httpServletRequest.getRequestURI());
            try{
                httpServletRequest.getInputStream().skip(httpServletRequest.getContentLength());
            }catch (Exception e2){
                logger.warn("skip content length fail");
            }
            throw e;
        }finally {
            try {
                httpServletRequest.getInputStream().close();
            }catch (Exception e2){
                logger.warn("close inputStream failed", e2);
            }
        }
    }

    @GetMapping(value="/{bucketname:.+}/**", produces = MediaType.APPLICATION_XML_VALUE )
    public void getObject(@PathVariable("bucketname") String bucketName,
                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                    @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                                    HttpServletRequest httpServletRequest,
                                    HttpServletResponse response)
            throws S3ServerException, IOException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("get object. bucketName={}, objectName={}", bucketName, objectName);

            getObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("get object success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("get object failed. bucketName={}, bucketName/objectName={}, versionId={}",
                    bucketName, httpServletRequest.getRequestURI(), versionId);
            throw e;
        }
    }

    @GetMapping(value="/{bucketname:.+}/**", params = RestParamDefine.CommonPara.X_AMZ_SIGNATURE, produces = MediaType.APPLICATION_XML_VALUE )
    public void getObjectUrlV4(@PathVariable("bucketname") String bucketName,
                             @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_CREDENTIAL, required = false) String credential,
                             @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_EXPIRES, required = false) Long expireTime,
                             @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                             @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_DATE, required = false) String xamzdate,
                             HttpServletRequest httpServletRequest,
                             HttpServletResponse response)
            throws S3ServerException, IOException{
        try {
            User operator = restUtils.getOperatorByCredential(credential);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("get object by url v4. bucketName={}, objectName={}", bucketName, objectName);

            checkExpireV4(expireTime, xamzdate);

            getObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("get object by url v4 success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("get object by url failed. bucketName={}, bucketName/objectName={}, versionId={}",
                    bucketName, httpServletRequest.getRequestURI(), versionId);
            throw e;
        }
    }

    @GetMapping(value="/{bucketname:.+}/**", params = RestParamDefine.CommonPara.SIGNATURE, produces = MediaType.APPLICATION_XML_VALUE )
    public void getObjectUrlV2(@PathVariable("bucketname") String bucketName,
                               @RequestParam(value = RestParamDefine.CommonPara.ACCESS_KEYID, required = false) String accessKeyId,
                               @RequestParam(value = RestParamDefine.CommonPara.EXPIRES, required = false) Long expireTime,
                               @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                               HttpServletRequest httpServletRequest,
                               HttpServletResponse response)
            throws S3ServerException, IOException{
        try {
            User operator = restUtils.getOperatorByAccessKeyId(accessKeyId);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("get object by url v2. bucketName={}, objectName={}", bucketName, objectName);

            checkExpireV2(expireTime);

            getObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("get object by url v2 success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("get object by url failed. bucketName={}, bucketName/objectName={}, versionId={}",
                    bucketName, httpServletRequest.getRequestURI(), versionId);
            throw e;
        }
    }

    @DeleteMapping(value = "/{bucketname:.+}/**", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteObject(@PathVariable("bucketname") String bucketName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       HttpServletRequest httpServletRequest)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("delete object. bucketName={}, objectName={}", bucketName, objectName);

            PutDeleteResult result = objectService.deleteObject(operator.getUserId(), bucketName, objectName);
            HttpHeaders headers = new HttpHeaders();
            if (result != null) {
                if (result.getDeleteMarker() != null) {
                    headers.add(RestParamDefine.DeleteObjectResultHeader.DELETE_MARKER,
                            result.getDeleteMarker().toString());
                }
                if (result.getVersionId() != null) {
                    headers.add(RestParamDefine.DeleteObjectResultHeader.VERSION_ID,
                            result.getVersionId());
                }
            }
            logger.debug("delete object success. bucketName={}, objectName={}", bucketName, objectName);
            return ResponseEntity.noContent()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("delete object failed. bucketName={}, bucketName/objectName={}", bucketName, httpServletRequest.getRequestURI());
            throw e;
        }
    }

    @DeleteMapping(value = "/{bucketname:.+}/**", params = RestParamDefine.VERSION_ID, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteObjectByVersionId(@PathVariable("bucketname") String bucketName,
                                                  @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                                  @RequestParam(RestParamDefine.VERSION_ID) String versionId,
                                                  HttpServletRequest httpServletRequest)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("delete object with version id. bucketName={}, objectName={}, versionId={}",
                    bucketName, objectName, versionId);

            Long cvtVersionId = convertVersionId(versionId);

            PutDeleteResult result = objectService.deleteObject(operator.getUserId(), bucketName, objectName, cvtVersionId);
            HttpHeaders headers = new HttpHeaders();
            headers.add(RestParamDefine.DeleteObjectResultHeader.VERSION_ID, versionId);
            if (result != null){
                headers.add(RestParamDefine.DeleteObjectResultHeader.DELETE_MARKER,
                        result.getDeleteMarker().toString());
            }

            logger.debug("delete object with version id success. bucketName={}, objectName={}, versionId={}",
                    bucketName, objectName, versionId);
            return ResponseEntity.noContent()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("delete object failed. bucketName={}, bucketName/objectName={}, versionId={}",
                    bucketName, httpServletRequest.getRequestURI(), versionId);
            throw e;
        }
    }

    @PostMapping(value = "/{bucketname:.+}", params = RestParamDefine.DELETE, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteObjects(@PathVariable("bucketname") String bucketName,
                                        @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                        @RequestHeader(name = RestParamDefine.PutObjectHeader.CONTENT_MD5, required = false) String contentMD5,
                                        HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("delete objects. bucketName={}", bucketName);

            bucketService.getBucket(operator.getUserId(), bucketName);

            DeleteObjectsResult deleteObjectsResult = new DeleteObjectsResult();
            DeleteObjects deleteObjects = getDeleteObject(httpServletRequest, contentMD5);

            if (deleteObjects != null && deleteObjects.getObjects() != null)
            {
                List<ObjectToDel> objects = deleteObjects.getObjects();
                for (ObjectToDel object : objects) {
                    try {
                        PutDeleteResult result = null;
                        if (object.getVersionId() != null) {
                            Long cvtVersionId = convertVersionId(object.getVersionId());
                            result = objectService.deleteObject(operator.getUserId(), bucketName, object.getKey(), cvtVersionId);
                        } else {
                            result = objectService.deleteObject(operator.getUserId(), bucketName, object.getKey());
                        }
                        if (!deleteObjects.getQuiet()) {
                            ObjectDeleted deleted = new ObjectDeleted();
                            if (result != null && result.getDeleteMarker()) {
                                deleted.setDeleteMarker(true);
                                deleted.setDeleteMarkerVersion(result.getVersionId());
                            }
                            deleted.setVersionId(object.getVersionId());
                            deleted.setKey(object.getKey());
                            deleteObjectsResult.getDeletedObjects().add(deleted);
                        }
                    } catch (S3ServerException e) {
                        DeleteError error = new DeleteError();
                        error.setCode(e.getError().getCode());
                        error.setMessage(e.getError().getErrorMessage());
                        error.setKey(object.getKey());
                        if (object.getVersionId() != null) {
                            error.setVersionId(object.getVersionId());
                        }
                        deleteObjectsResult.getErrors().add(error);
                    } catch (Exception e) {
                        DeleteError error = new DeleteError();
                        error.setCode(S3Error.OBJECT_DELETE_FAILED.getCode());
                        error.setMessage(e.getMessage());
                        error.setKey(object.getKey());
                        if (object.getVersionId() != null) {
                            error.setVersionId(object.getVersionId());
                        }
                        deleteObjectsResult.getErrors().add(error);
                    }
                }
            }

            return ResponseEntity.ok()
                    .body(deleteObjectsResult);
        }catch (Exception e){
            throw e;
        }
    }

    @GetMapping(value = "/{bucketname:.+}", params = RestParamDefine.ListObjectsPara.LIST_TYPE2,
            produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listObjectsV2(@PathVariable("bucketname") String bucketName,
                                        @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.PREFIX, required = false) String prefix,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.DELIMITER, required = false) String delimiter,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.START_AFTER, required = false) String startAfter,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.MAX_KEYS, required = false, defaultValue = "1000") Integer maxKeys,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.CONTINUATIONTOKEN, required = false) String continueToken,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.ENCODING_TYPE, required = false) String encodingType,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.FETCH_OWNER, required = false, defaultValue = "false") Boolean fetchOwner)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("list objectsV2. bucketName={}, delimiter={}, prefix={}, startAfter={}",
                    bucketName, delimiter, prefix, startAfter);

            if (null != encodingType) {
                if (!encodingType.equals(RestParamDefine.ENCODING_TYPE_URL)) {
                    throw new S3ServerException(S3Error.OBJECT_INVALID_ENCODING_TYPE, "encoding type must be url");
                }
            }

            ListObjectsResult result = objectService.listObjects(operator.getUserId(),
                    bucketName, prefix, delimiter, startAfter,
                    maxKeys, continueToken, encodingType, fetchOwner);

            logger.debug("list objectsV2 success. bucketName={}, " +
                            "commonPrefix.size={}, " +
                            "content.size={} ",
                    bucketName,
                    result.getCommonPrefixList().size(),
                    result.getContentList().size());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.error("List objects v2 failed. bucketName={}, delimiter={}, prefix={}, " +
                    "startAfter={}", bucketName, delimiter, prefix, startAfter);
            throw e;
        }
    }

    @GetMapping(value = "/{bucketname:.+}", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listObjectsV1(@PathVariable("bucketname") String bucketName,
                                        @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                        @RequestParam(value = RestParamDefine.ListObjectsPara.PREFIX, required = false) String prefix,
                                        @RequestParam(value = RestParamDefine.ListObjectsV1Para.DELIMITER, required = false) String delimiter,
                                        @RequestParam(value = RestParamDefine.ListObjectsV1Para.MARKER, required = false) String startAfter,
                                        @RequestParam(value = RestParamDefine.ListObjectsV1Para.MAX_KEYS, required = false, defaultValue = "1000") Integer maxKeys,
                                        @RequestParam(value = RestParamDefine.ListObjectsV1Para.ENCODING_TYPE, required = false) String encodingType)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("list objectsV1 with delimiter={}, marker={}, prefix={}, maxKeys={}, encodingType={}", delimiter, startAfter, prefix, maxKeys, encodingType);

            if (null != encodingType) {
                if (!encodingType.equals(RestParamDefine.ENCODING_TYPE_URL)) {
                    throw new S3ServerException(S3Error.OBJECT_INVALID_ENCODING_TYPE, "encoding type must be url");
                }
            }

            ListObjectsResultV1 result = objectService.listObjectsV1(operator.getUserId(),
                    bucketName, prefix, delimiter, startAfter,
                    maxKeys, encodingType);

            logger.debug("list objectsV1 success. bucketName={}," +
                            " commonPrefix.size={}," +
                            " content.size={}",
                    bucketName,
                    result.getCommonPrefixList().size(),
                    result.getContentList().size());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.error("list objectsV1 failed. with delimiter={}, marker={}, prefix={}, maxKeys={}, " +
                    "encodingType={}", delimiter, startAfter, prefix, maxKeys, encodingType);
            throw e;
        }
    }

    @GetMapping(value = "/{bucketname:.+}", params = RestParamDefine.VERSIONS, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listObjectsVersions(@PathVariable("bucketname") String bucketName,
                                              @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.PREFIX, required = false) String prefix,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.DELIMITER, required = false) String delimiter,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.KEY_MARKER, required = false) String keyMarker,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.VERSION_ID_MARKER, required = false) String versionIdMarker,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.ENCODING_TYPE, required = false) String encodingType,
                                              @RequestParam(value = RestParamDefine.ListObjectVersionsPara.MAX_KEYS, required = false, defaultValue = "1000") Integer maxKeys)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("list versions. bucketName={}, delimiter={}, prefix={}, startAfter={}", bucketName, delimiter, prefix, keyMarker);

            if (null != encodingType) {
                if (!encodingType.equals(RestParamDefine.ENCODING_TYPE_URL)) {
                    throw new S3ServerException(S3Error.OBJECT_INVALID_ENCODING_TYPE, "encoding type must be url");
                }
            }

            ListVersionsResult result = objectService.listVersions(operator.getUserId(),
                    bucketName, prefix, delimiter, keyMarker, versionIdMarker, maxKeys, encodingType);

            logger.debug("list versions success. bucketName={}, " +
                            "commonPrefix.size={}, " +
                            "versions.size={}, " +
                            "deleterMarker.size={}",
                    bucketName,
                    result.getCommonPrefixList().size(),
                    result.getVersionList().size(),
                    result.getDeleteMarkerList().size());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.error("list versions failed. bucketName={}, delimiter={}, prefix={}, startAfter={}",
                    bucketName, delimiter, prefix, keyMarker);
            throw e;
        }
    }

    @RequestMapping(method = RequestMethod.HEAD, value="/{bucketname:.+}/**")
    public void headObject(@PathVariable("bucketname") String bucketName,
                                     @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                     @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                                     HttpServletRequest httpServletRequest,
                                     HttpServletResponse response)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("head object. bucketName={}, objectName={}", bucketName, objectName);

            headObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("head object success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("head object failed. bucketName={}, bucketName/objectName={}",
                    bucketName, httpServletRequest.getRequestURI());
            throw e;
        }
    }

    @RequestMapping(method = RequestMethod.HEAD, value="/{bucketname:.+}/**", params = RestParamDefine.CommonPara.X_AMZ_SIGNATURE)
    public void headObjectUrlV4(@PathVariable("bucketname") String bucketName,
                                @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_CREDENTIAL, required = false) String credential,
                                @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_EXPIRES, required = false) Long expireTime,
                                @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                                @RequestParam(value = RestParamDefine.CommonPara.X_AMZ_DATE, required = false) String xamzdate,
                                HttpServletRequest httpServletRequest,
                                HttpServletResponse response)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByCredential(credential);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("head object by url auth v4. bucketName={}, objectName={}", bucketName, objectName);

            checkExpireV4(expireTime, xamzdate);

            headObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("head object success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("head object failed. bucketName={}, bucketName/objectName={}",
                    bucketName, httpServletRequest.getRequestURI());
            throw e;
        }
    }

    @RequestMapping(method = RequestMethod.HEAD, value="/{bucketname:.+}/**", params = RestParamDefine.CommonPara.SIGNATURE)
    public void headObjectUrlV2(@PathVariable("bucketname") String bucketName,
                                @RequestParam(value = RestParamDefine.CommonPara.ACCESS_KEYID, required = false) String accessKeyId,
                                @RequestParam(value = RestParamDefine.CommonPara.EXPIRES, required = false) Long expireTime,
                                @RequestParam(value = RestParamDefine.VERSION_ID, required = false) String versionId,
                                HttpServletRequest httpServletRequest,
                                HttpServletResponse response)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAccessKeyId(accessKeyId);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("head object by url auth v2. bucketName={}, objectName={}", bucketName, objectName);

            checkExpireV2(expireTime);

            headObjectCommon(bucketName, versionId, objectName, operator, httpServletRequest, response);

            logger.debug("get object by url v2 success. bucketName={}, objectName={}", bucketName, objectName);
        }catch (Exception e){
            logger.error("get object by url failed. bucketName={}, bucketName/objectName={}, versionId={}",
                    bucketName, httpServletRequest.getRequestURI(), versionId);
            throw e;
        }
    }

    @PutMapping(value="/{bucketname:.+}/**", headers = RestParamDefine.CopyObjectHeader.X_AMZ_COPY_SOURCE, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity copyObject(@PathVariable("bucketname") String bucketName,
                           @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                           @RequestHeader(name = RestParamDefine.CopyObjectHeader.X_AMZ_COPY_SOURCE) String copySource,
                           @RequestHeader(name = RestParamDefine.CopyObjectHeader.METADATA_DIRECTIVE, required = false) String directive,
                           HttpServletRequest httpServletRequest,
                           HttpServletResponse response)
            throws S3ServerException, IOException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("copy object. bucketName={}, objectName={}, source={}", bucketName, objectName, copySource);
            Map<String, String> requestHeaders = new HashMap<>();
            Map<String, String> xMeta = new HashMap<>();
            restUtils.getHeaders(httpServletRequest, requestHeaders, xMeta);

            ObjectUri sourceUri = new ObjectUri(copySource);

            boolean copyMeta = directiveCopy(directive);

            CopyObjectResult result = objectService.copyObject(operator.getUserId(),
                    bucketName, objectName, requestHeaders, xMeta, sourceUri,
                    copyMeta, response.getOutputStream());

            HttpHeaders headers = new HttpHeaders();
            if (result.getVersionId() != null) {
                headers.add(RestParamDefine.CopyObjectResultHeader.VERSION_ID,
                        result.getVersionId().toString());
            }
            if (result.getSourceVersionId() != null) {
                headers.add(RestParamDefine.CopyObjectResultHeader.SOURCE_VERSION_ID,
                        result.getVersionId().toString());
            }

            logger.debug("copy object success. bucketName={}, objectName={}, source={}", bucketName, objectName, copySource);

            return ResponseEntity.ok()
                    .headers(headers)
                    .body(result);
        }catch (Exception e){
            logger.error("copy object failed. bucketName={}, bucketName/objectName={}, " +
                    "source={}", bucketName, httpServletRequest.getRequestURI(), copySource);
            throw e;
        }
    }

    private Long convertVersionId(String versionId)
            throws S3ServerException{
        try {
            if (versionId.equals(ObjectMeta.NULL_VERSION_ID)) {
                return null;
            } else {
                return Long.parseLong(versionId);
            }
        }catch (NumberFormatException e) {
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "version id is invalid。 version id=" + versionId);
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_VERSION,
                    "versionId is invalid. versionId="+versionId+",e:"+e.getMessage());
        }
    }

    private boolean directiveCopy(String directive) throws S3ServerException{
        if (directive == null){
            return true;
        }
        if (directive.equals(RestParamDefine.REST_DIRECTIVE_COPY)){
            return true;
        }
        if (directive.equals(RestParamDefine.REST_DIRECTIVE_REPLACE)){
            return false;
        }

        throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_DIRECTIVE,
                "invalid directive. directive="+directive);
    }

    private void buildDeleteMarkerResponseHeader(ObjectMeta objectMeta, HttpServletResponse response) {
        response.addHeader(RestParamDefine.GetObjectResHeader.DELETE_MARKER, objectMeta.getDeleteMarker().toString());
        if (objectMeta.getNoVersionFlag()){
            response.addHeader(RestParamDefine.GetObjectResHeader.VERSION_ID, ObjectMeta.NULL_VERSION_ID);
        }else {
            response.addHeader(RestParamDefine.GetObjectResHeader.VERSION_ID, String.valueOf(objectMeta.getVersionId()));
        }
    }

    private void buildHeadersForGetObject(ObjectMeta objectMeta, HttpServletRequest request,
                                          Range range, HttpServletResponse response){
        response.addHeader(RestParamDefine.GetObjectResHeader.ETAG, "\""+objectMeta.geteTag()+"\"");
        response.addDateHeader(RestParamDefine.GetObjectResHeader.LAST_MODIFIED, objectMeta.getLastModified());
        if (objectMeta.getNoVersionFlag()) {
            response.addHeader(RestParamDefine.GetObjectResHeader.VERSION_ID, "null");
        }else {
            response.addHeader(RestParamDefine.GetObjectResHeader.VERSION_ID, String.valueOf(objectMeta.getVersionId()));
        }
        response.addHeader(RestParamDefine.GetObjectResHeader.ACCEPT_RANGES, "bytes");

        if (null != objectMeta.getMetaList()){
            Map metaList = objectMeta.getMetaList();
            for (Object o : metaList.entrySet()) {
                Map.Entry entry = (Map.Entry) o;
                response.addHeader(entry.getKey().toString(), entry.getValue().toString());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_CACHE_CONTROL) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.CACHE_CONTROL,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_CACHE_CONTROL));
        } else {
            if (objectMeta.getCacheControl() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.CACHE_CONTROL, objectMeta.getCacheControl());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_DISPOSITION) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_DISPOSITION,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_DISPOSITION));
        } else{
            if (objectMeta.getContentDisposition() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_DISPOSITION, objectMeta.getContentDisposition());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_ENCODING) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_ENCODING,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_ENCODING));
        } else {
            if (objectMeta.getContentEncoding() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_ENCODING, objectMeta.getContentEncoding());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_LANGUAGE) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_LANGUAGE,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_LANGUAGE));
        } else {
            if (objectMeta.getContentLanguage() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_LANGUAGE, objectMeta.getContentLanguage());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_TYPE) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_TYPE,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_CONTENT_TYPE));
        } else {
            if (objectMeta.getContentType() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_TYPE, objectMeta.getContentType());
            }
        }

        if (request.getParameter(RestParamDefine.GetObjectReqPara.RES_EXPIRES) != null){
            response.addHeader(RestParamDefine.GetObjectResHeader.EXPIRES,
                    request.getParameter(RestParamDefine.GetObjectReqPara.RES_EXPIRES));
        } else {
            if (objectMeta.getExpires() != null) {
                response.addHeader(RestParamDefine.GetObjectResHeader.EXPIRES, objectMeta.getExpires());
            }
        }

        if (null == range){
            response.setContentLengthLong(objectMeta.getSize());
        }else if (range.getContentLength() >= objectMeta.getSize()){
            response.setContentLengthLong(objectMeta.getSize());
        }else {
            response.addHeader(RestParamDefine.GetObjectResHeader.CONTENT_RANGE,
                    "bytes " + range.getStart() + "-" + range.getEnd() + "/" + objectMeta.getSize());
            response.setContentLengthLong(range.getContentLength());
            response.setStatus(HttpServletResponse.SC_PARTIAL_CONTENT);
        }
    }

    private void getObjectCommon(String bucketName, String versionId,
                                 String objectName, User operator,
                                 HttpServletRequest httpServletRequest,
                                 HttpServletResponse response)
            throws S3ServerException, IOException {
        Map<String, String> requestHeaders = new HashMap<>();
        Enumeration headerNames = httpServletRequest.getHeaderNames();
        while (headerNames.hasMoreElements()) {
            String name = headerNames.nextElement().toString();
            requestHeaders.put(name, httpServletRequest.getHeader(name));
        }

        Range range = null;
        if (requestHeaders.containsKey(RestParamDefine.GetObjectReqHeader.REQ_RANGE)) {
            range = restUtils.getRange(requestHeaders.get(RestParamDefine.GetObjectReqHeader.REQ_RANGE));
        }

        Boolean nullVersionFlag = null;
        Long cvtVersionId = null;
        if (versionId != null) {
            cvtVersionId = convertVersionId(versionId);
            if (null == cvtVersionId) {
                nullVersionFlag = true;
            }
        }

        GetResult result = objectService.getObject(operator.getUserId(), bucketName,
                objectName, cvtVersionId, nullVersionFlag, requestHeaders, range);

        try {
            if (result.getMeta().getDeleteMarker()) {
                buildDeleteMarkerResponseHeader(result.getMeta(), response);
                if (null == versionId) {
                    throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "no object. object:" + objectName);
                } else {
                    throw new S3ServerException(S3Error.METHOD_NOT_ALLOWED, "no object. object:" + objectName);
                }
            } else {
                buildHeadersForGetObject(result.getMeta(), httpServletRequest, range, response);
                objectService.readObjectData(result.getData(), response.getOutputStream(), range);
            }
        } finally {
            objectService.releaseGetResult(result);
        }
    }

    private void headObjectCommon(String bucketName, String versionId,
                                  String objectName, User operator,
                                  HttpServletRequest httpServletRequest,
                                  HttpServletResponse response)
            throws S3ServerException{

        Map<String, String> requestHeaders = new HashMap<>();
        Enumeration headerNames = httpServletRequest.getHeaderNames();
        while (headerNames.hasMoreElements()) {
            String name = headerNames.nextElement().toString();
            requestHeaders.put(name, httpServletRequest.getHeader(name));
        }

        Range range = null;
        if (requestHeaders.containsKey(RestParamDefine.GetObjectReqHeader.REQ_RANGE)) {
            range = restUtils.getRange(requestHeaders.get(RestParamDefine.GetObjectReqHeader.REQ_RANGE));
        }

        Boolean nullVersionFlag = null;
        Long cvtVersionId = null;
        if (versionId != null) {
            cvtVersionId = convertVersionId(versionId);
            if (null == cvtVersionId) {
                nullVersionFlag = true;
            }
        }

        GetResult result = objectService.getObject(operator.getUserId(), bucketName,
                objectName, cvtVersionId, nullVersionFlag, requestHeaders, range);

        try {
            if (result.getMeta().getDeleteMarker()) {
                throw new S3ServerException(S3Error.OBJECT_NO_SUCH_KEY, "no object. object:" + objectName);
            } else {
                buildHeadersForGetObject(result.getMeta(), httpServletRequest, range, response);
            }
        } finally {
            objectService.releaseGetResult(result);
        }
    }

    private void checkExpireV4(Long expireTime, String xamzdate)
            throws S3ServerException{
        if (expireTime != null) {
            if(xamzdate != null){
                long nowTime = System.currentTimeMillis();
                Date date = DataFormatUtils.parseXAMZDate(xamzdate);
                if(nowTime/1000 - date.getTime()/1000 > expireTime){
                    throw new S3ServerException(S3Error.ACCESS_EXPIRED,
                            "Request has expired.  X-Amz-Date:" + date.toString() +
                                    ", X-Amz-Expires:" + expireTime +
                                    ", ServerTime:" + DataFormatUtils.formatDate(nowTime));
                }
            }
        }
    }

    private void checkExpireV2(Long expireTime)
            throws S3ServerException {
        if (expireTime != null) {
            long nowTime = System.currentTimeMillis();
            if (nowTime / 1000 > expireTime) {
                throw new S3ServerException(S3Error.ACCESS_EXPIRED,
                        "Request has expired. Expires:" + expireTime +
                                ", ServerTime:" + DataFormatUtils.formatDate(nowTime));
            }
        }
    }

    private DeleteObjects getDeleteObject(HttpServletRequest httpServletRequest, String contentMD5)
            throws S3ServerException {
        int ONCE_READ_BYTES  = 1024;
        try {
            ServletInputStream inputStream = httpServletRequest.getInputStream();
            StringBuilder stringBuilder = new StringBuilder();
            MessageDigest MD5 = MessageDigest.getInstance("MD5");
            byte[] b = new byte[ONCE_READ_BYTES];
            int len = inputStream.read(b, 0, ONCE_READ_BYTES);
            while (len > 0) {
                MD5.update(b, 0, len);
                stringBuilder.append(new String(b, 0, len));
                len = inputStream.read(b, 0, ONCE_READ_BYTES);
            }
            if (contentMD5 != null){
                if(!MD5Utils.isMd5EqualWithETag(contentMD5, new String(Hex.encodeHex(MD5.digest())))){
                    throw new S3ServerException(S3Error.OBJECT_BAD_DIGEST,
                            "The Content-MD5 you specified does not match what we received.");
                }
            }
            String content = stringBuilder.toString();
            if (content.length() > 0) {
                ObjectMapper objectMapper = new XmlMapper();
                return objectMapper.readValue(content, DeleteObjects.class);
            }else {
                return null;
            }
        }catch (S3ServerException e){
            throw e;
        } catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML, "get delete objects failed", e);
        }
    }
}
