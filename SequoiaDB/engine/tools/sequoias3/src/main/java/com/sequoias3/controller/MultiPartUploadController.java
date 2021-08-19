package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.S3InputStreamReaderChunk;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.*;
import com.sequoias3.service.BucketService;
import com.sequoias3.service.ObjectService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

@RestController
public class MultiPartUploadController {
    private final Logger logger = LoggerFactory.getLogger(MultiPartUploadController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    ObjectService objectService;

    @Autowired
    BucketService bucketService;

    @PostMapping(value="/{bucketname:.+}/**", params = RestParamDefine.UPLOADS, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity initMultiPartUploadObject(@PathVariable("bucketname") String bucketName,
                                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                                    HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("initMultiPartUploadObject. bucketName={}, objectName={}", bucketName, objectName);

            Map<String, String> requestHeaders = new HashMap<>();
            Map<String, String> xMeta = new HashMap<>();
            restUtils.getHeaders(httpServletRequest, requestHeaders, xMeta);

            InitiateMultipartUploadResult result = objectService.initMultipartUpload(operator.getUserId(),
                    bucketName,
                    objectName,
                    requestHeaders,
                    xMeta);

            logger.debug("initMultiPartUploadObject success. bucketName={}, objectName={}, uploadId={}", bucketName, objectName, result.getUploadId());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.error("initMultiPartUploadObject failed. bucketName={}, bucketName/objectName={}",
                    bucketName, httpServletRequest.getRequestURI());
            throw e;
        }
    }

    @PutMapping(value="/{bucketname:.+}/**", params = RestParamDefine.PARTNUMBER, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity uploadPart(@PathVariable("bucketname") String bucketName,
                                     @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                     @RequestHeader(name = RestParamDefine.PutObjectHeader.CONTENT_MD5, required = false) String contentMD5,
                                     @RequestParam(RestParamDefine.PARTNUMBER) String partNumberStr,
                                     @RequestParam(RestParamDefine.UPLOADID) String uploadIdStr,
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
            } else {
                objectName = ((ObjectUri) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_OBJECTURI)).getObjectName();
            }

            logger.debug("upload part. bucketName={}, objectName={}, uploadId={}, partNumber={}",
                    bucketName, objectName, uploadIdStr, partNumberStr);

            //get and check bucket
            Bucket bucket;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_BUCKET) == null) {
                bucket = bucketService.getBucket(operator.getUserId(), bucketName);
            } else {
                bucket = (Bucket) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_BUCKET);
            }

            long uploadId;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_UPLOADID) != null){
                uploadId = (long) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_UPLOADID);
            } else {
                uploadId = restUtils.convertUploadId(uploadIdStr);
            }

            int partNumber;
            if (httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_PARTNUMBER) != null){
                partNumber = (int) httpServletRequest.getAttribute(RestParamDefine.Attribute.S3_PARTNUMBER);
            } else {
                partNumber = restUtils.convertPartNumber(partNumberStr);
                if (partNumber < RestParamDefine.PART_NUMBER_MIN
                        || partNumber > RestParamDefine.PART_NUMBER_MAX) {
                    throw new S3ServerException(S3Error.PART_INVALID_PARTNUMBER,
                            "invalid partNumber:" + partNumber);
                }
            }

            InputStream body = httpServletRequest.getInputStream();
            Long realContentLength = 0L;
            if (httpServletRequest.getHeader("x-amz-decoded-content-length") != null) {
                body = new S3InputStreamReaderChunk(httpServletRequest.getInputStream());
                realContentLength = Long.parseLong(httpServletRequest.getHeader("x-amz-decoded-content-length"));
            } else {
                if (httpServletRequest.getHeader("content-length") != null) {
                    realContentLength = Long.parseLong(httpServletRequest.getHeader("content-length"));
                }
            }
            String eTag = objectService.uploadPart(bucket,
                    objectName,
                    uploadId,
                    partNumber,
                    contentMD5,
                    body,
                    realContentLength);

            HttpHeaders headers = new HttpHeaders();
            if (eTag != null) {
                headers.add(RestParamDefine.PutObjectResultHeader.ETAG,
                        "\"" + eTag + "\"");
            }

            logger.debug("upload part success. bucketName={}, objectName={}, uploadId={}, " +
                            "partNumber={}, eTag={}, realContentLength={}",
                    bucketName, objectName, uploadId, partNumber, eTag, realContentLength);
            return ResponseEntity.ok()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("upload part failed. bucketName={}, bucketName/objectName={}," +
                    " uploadId={}, partNumber={}", bucketName,
                    httpServletRequest.getRequestURI(), uploadIdStr, partNumberStr);
            try{
                httpServletRequest.getInputStream().skip(httpServletRequest.getContentLength());
            }catch (Exception e2){
                logger.error("skip content length fail");
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

    @PostMapping(value="/{bucketname:.+}/**", params = RestParamDefine.UPLOADID, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity completeMultiPart(@PathVariable("bucketname") String bucketName,
                                            @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                            @RequestParam(RestParamDefine.UPLOADID) String uploadId,
                                            HttpServletRequest httpServletRequest,
                                            HttpServletResponse httpServletResponse)
            throws S3ServerException, IOException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("completeMultiPart. bucketName={}, objectName={}, uploadId:{}", bucketName, objectName, uploadId);

            ServletOutputStream outputStream = httpServletResponse.getOutputStream();

            CompleteMultipartUpload completeMultipartUpload = getCompleteMap(httpServletRequest);

            CompleteMultipartUploadResult result = objectService.completeUpload(operator.getUserId(),
                    bucketName,
                    objectName,
                    uploadId,
                    completeMultipartUpload.getPart(),
                    outputStream);

            result.setLocation(httpServletRequest.getRequestURI());

            logger.debug("completeMultiPart success. bucketName={}, objectName={}, uploadId={}",
                    bucketName, objectName, uploadId);

            HttpHeaders headers = new HttpHeaders();
            if (result.getVersionId() != null) {
                headers.add(RestParamDefine.PutObjectResultHeader.VERSION_ID,
                        result.getVersionId().toString());
            }

            return ResponseEntity.ok()
                    .headers(headers)
                    .body(result);
        }catch (Exception e){
            logger.error("completeMultiPart failed. bucketName={}, bucketName/objectName={}, uploadId:{}",
                    bucketName, httpServletRequest.getRequestURI(), uploadId);
            throw e;
        }
    }

    @GetMapping(value="/{bucketname:.+}/**", params = RestParamDefine.UPLOADID, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listParts(@PathVariable("bucketname") String bucketName,
                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                    @RequestParam(RestParamDefine.UPLOADID) String uploadId,
                                    @RequestParam(value = RestParamDefine.ListPartsPara.PART_NUMBER_MARKER, required = false) Integer partNumberMarker,
                                    @RequestParam(value = RestParamDefine.ListPartsPara.MAX_PARTS, required = false, defaultValue = "1000") Integer maxParts,
                                    @RequestParam(value = RestParamDefine.ListPartsPara.ENCODING_TYPE, required = false) String encodingType,
                                    HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("listParts. bucketName={}, objectName={}, uploadId:{}, encodeType={}", bucketName, objectName, uploadId, encodingType);

            ListPartsResult result = objectService.listParts(operator.getUserId(),
                    bucketName,
                    objectName,
                    uploadId,
                    partNumberMarker,
                    maxParts,
                    encodingType);

            logger.debug("listParts success. bucketName={}, objectName={}, uploadId={}, part.size={}",
                    bucketName, objectName, uploadId, result.getPartList().size());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.error("list Parts failed. bucketName={}, bucketName/objectName={}, uploadId:{}",
                    bucketName, httpServletRequest.getRequestURI(), uploadId);
            throw e;
        }
    }

    @DeleteMapping(value="/{bucketname:.+}/**", params = RestParamDefine.UPLOADID, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity abortUpload(@PathVariable("bucketname") String bucketName,
                                      @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                      @RequestParam(RestParamDefine.UPLOADID) String uploadId,
                                      HttpServletRequest httpServletRequest)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("abortUpload. bucketName={}, objectName={}, uploadId:{}", bucketName, objectName, uploadId);

            objectService.abortUpload(operator.getUserId(), bucketName, objectName, uploadId);

            logger.debug("abortUpload success. bucketName={}, objectName={}, uploadId={}", bucketName, objectName, uploadId);
            return ResponseEntity.ok()
                    .build();
        }catch (Exception e){
            logger.error("abortUpload failed. bucketName={}, bucketName/objectName={}, uploadId:{}",
                    bucketName, httpServletRequest.getRequestURI(), uploadId);
            throw e;
        }
    }

    @GetMapping(value="/{bucketname:.+}", params = RestParamDefine.UPLOADS, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listUploads(@PathVariable("bucketname") String bucketName,
                                      @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.PREFIX, required = false) String prefix,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.DELIMITER, required = false) String delimiter,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.KEY_MARKER, required = false) String keyMarker,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.UPLOAD_ID_MARKER, required = false) Long uploadIdMarker,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.ENCODING_TYPE, required = false) String encodingType,
                                      @RequestParam(value = RestParamDefine.ListUploadsPara.MAX_UPLOADS, required = false, defaultValue = "1000") Integer maxUploads)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            logger.debug("listUploads. bucketName={}, prefix={}, delimiter={}, keyMarker={}, uploadIdMarker={}",
                    bucketName, prefix, delimiter, keyMarker, uploadIdMarker);

            if (delimiter != null && delimiter.length() == 0) {
                delimiter = null;
            }

            if (prefix != null && prefix.length() == 0) {
                prefix = null;
            }

            ListMultipartUploadsResult result = objectService.listUploadLists(operator.getUserId(),
                    bucketName,
                    prefix,
                    delimiter,
                    keyMarker,
                    uploadIdMarker,
                    maxUploads,
                    encodingType);

            logger.debug("listUploads success. bucketName={}, upload.size={}, commonPrefix.size={}",
                    bucketName, result.getUploadList().size(), result.getCommonPrefixList().size());
            return ResponseEntity.ok()
                    .body(result);
        }catch (Exception e){
            logger.info("listUploads failed. bucketName={}, prefix={}, delimiter={}, keyMarker={}, uploadIdMarker={}",
                    bucketName, prefix, delimiter, keyMarker, uploadIdMarker);
            throw e;
        }
    }

    private CompleteMultipartUpload getCompleteMap(HttpServletRequest httpServletRequest)
            throws S3ServerException{
        int ONCE_READ_BYTES  = 1024;
        CompleteMultipartUpload completeMultipartUpload = null;
        try {
            ServletInputStream inputStream = httpServletRequest.getInputStream();
            StringBuilder stringBuilder = new StringBuilder();
            byte[] b = new byte[ONCE_READ_BYTES];
            int len = inputStream.read(b , 0, ONCE_READ_BYTES);
            while(len > 0){
                stringBuilder.append(new String(b,0, len));
                len = inputStream.read(b , 0, ONCE_READ_BYTES);
            }
            String content = stringBuilder.toString();
            if (content.length() > 0) {
                ObjectMapper objectMapper = new XmlMapper();
                completeMultipartUpload = objectMapper.readValue(content, CompleteMultipartUpload.class);
            }
            if (completeMultipartUpload == null){
                throw new S3ServerException(S3Error.MALFORMED_XML,
                        "completeMultipartUpload is null.");
            }
            if (completeMultipartUpload.getPart() == null){
                throw new S3ServerException(S3Error.MALFORMED_XML,
                        "completeMultipartUpload is empty, there is no part list.");
            }
            return completeMultipartUpload;
        }catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML, "get part list failed", e);
        }
    }
}
