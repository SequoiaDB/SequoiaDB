package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.Bucket;
import com.sequoias3.model.*;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;

@RestController
public class BucketController {
    private static final Logger logger = LoggerFactory.getLogger(BucketController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketService bucketService;

    @PutMapping(value = "/{bucketname:.+}", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putBucket(@PathVariable("bucketname") String bucketName,
                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                    HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            if (httpServletRequest.getParameterNames().hasMoreElements()){
                throw new S3ServerException(S3Error.PARAMETER_NOT_SUPPORT,
                        "parameter " +
                                httpServletRequest.getParameterNames().nextElement() +
                                " is not supported for bucket.");
            }

            String location = getLocation(httpServletRequest);
            logger.debug("Create bucket. bucketName ={}, operator={}, location={} ",
                    bucketName, operator.getUserName(), location);
            bucketService.createBucket(operator.getUserId(), bucketName, location);
            return ResponseEntity.ok()
                    .header(RestParamDefine.LOCATION, RestParamDefine.REST_DELIMITER + bucketName)
                    .build();
        }catch (Exception e){
            logger.error("Create bucket failed. bucketName ={}, authorization={}",
                    bucketName, authorization);
            throw e;
        }
    }

    @GetMapping(value="", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity listBuckets( @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            logger.debug("list buckets. operator={}", operator.getUserName());
            return ResponseEntity.ok()
                    .body(bucketService.getService(operator));
        }catch (Exception e){
            logger.error("list buckets failed. authorization={}", authorization);
            throw e;
        }
    }

    @DeleteMapping(value = "/{bucketname:.+}", produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteBucket(@PathVariable("bucketname") String bucketName,
                               @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            if (httpServletRequest.getParameterNames().hasMoreElements()){
                throw new S3ServerException(S3Error.PARAMETER_NOT_SUPPORT,
                        "parameter " +
                                httpServletRequest.getParameterNames().nextElement() +
                                " is not supported for bucket.");
            }

            logger.debug("delete bucket. bucketName={}, operator={}", bucketName, operator.getUserName());
            bucketService.deleteBucket(operator.getUserId(), bucketName);
            logger.debug("delete bucket success. bucketName={}, operator={}", bucketName, operator.getUserName());
            return ResponseEntity.noContent().build();
        }catch (Exception e){
            logger.error("delete bucket failed. bucketName ={}, authorization={}",
                    bucketName, authorization);
            throw e;
        }
    }

    @RequestMapping(method = RequestMethod.HEAD, value = "/{bucketName:.+}")
    public ResponseEntity headBucket(@PathVariable("bucketName") String bucketName,
                               @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("head bucket. bucketName={}, operator={}", bucketName, operator.getUserName());
            Bucket bucket = bucketService.getBucket(operator.getUserId(), bucketName);
            HttpHeaders headers = new HttpHeaders();
            if (bucket.getRegion() != null) {
                headers.add(RestParamDefine.HeadBucketResultHeader.REGION, bucket.getRegion());
            }
            return ResponseEntity.ok()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("head bucket failed. bucketName ={}, authorization={}",
                    bucketName, authorization);
            throw e;
        }
    }

    @RequestMapping(method = RequestMethod.HEAD, value = "")
    public ResponseEntity headNone(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException {
        restUtils.getOperatorByAuthorization(authorization);

        logger.error("Method not allowed. head none bucket.");
        return ResponseEntity.status(HttpStatus.METHOD_NOT_ALLOWED)
                .build();
    }

    @RequestMapping(value = "/{bucketname:.+}", params = RestParamDefine.UPLOADID, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity bucketRejectUploadId(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                               HttpServletRequest httpServletRequest)
            throws S3ServerException{
        try {
            restUtils.getOperatorByAuthorization(authorization);
            logger.error("bucketRejectUploadId: need a key name.");
            throw new S3ServerException(S3Error.NEED_A_KEY, "need a key");
        }catch (Exception e){
            try{
                if (httpServletRequest.getInputStream() != null) {
                    httpServletRequest.getInputStream().skip(httpServletRequest.getContentLength());
                    httpServletRequest.getInputStream().close();
                }
            }catch (Exception e2){
                logger.error("skip content length fail");
            }
            throw e;
        }
    }

    @RequestMapping(value = "/{bucketname:.+}", headers = RestParamDefine.CopyObjectHeader.X_AMZ_COPY_SOURCE, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity bucketRejectCopy(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        restUtils.getOperatorByAuthorization(authorization);
        logger.error("bucketRejectCopy: need a key name.");
        throw new S3ServerException(S3Error.OBJECT_COPY_INVALID_DEST, "You can only specify a copy source header for copy requests.");
    }

    private String getLocation(HttpServletRequest httpServletRequest)
            throws S3ServerException{
        int ONCE_READ_BYTES  = 1024;
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
                return objectMapper.readValue(content, CreateBucketConfiguration.class).getLocationConstraint();
            }else {
                return null;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML, "get location failed", e);
        }
    }


}
