package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.VersioningStatusType;
import com.sequoias3.core.User;
import com.sequoias3.model.*;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.BucketVersioningService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;
import java.io.IOException;

@RestController
public class BucketVersioningController {
    private static final Logger logger = LoggerFactory.getLogger(BucketVersioningController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketVersioningService versioningService;

    @PutMapping(value = "/{bucketname:.+}", params = RestParamDefine.VERSIONING,
            produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putBucketVersioning(@PathVariable("bucketname") String bucketName,
                                      @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                      HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            String status = getVersioningStatus(httpServletRequest);

            logger.debug("put bucket versioning. bucket={}, status={}", bucketName, status);

            versioningService.putBucketVersioning(operator.getUserId(), bucketName, status);
            return ResponseEntity.ok()
                    .build();
        }catch (Exception e){
            logger.error("put bucket versioning failed. bucket={}", bucketName);
            throw e;
        }
    }

    @GetMapping(value = "/{bucketname:.+}", params = RestParamDefine.VERSIONING,
            produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getBucketVersioning(@PathVariable("bucketname") String bucketName,
                                              @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);

            logger.debug("get bucket versioning. bucket={}", bucketName);

            return ResponseEntity.ok()
                    .body(versioningService.getBucketVersioning(operator.getUserId(), bucketName));
        }catch (Exception e){
            logger.error("get bucket versioning failed. bucket={}", bucketName);
            throw e;
        }
    }

    private String getVersioningStatus(HttpServletRequest httpServletRequest)
            throws S3ServerException{
        int ONCE_READ_BYTES  = 1024;
        try {
            ServletInputStream inputStream = httpServletRequest.getInputStream();
            byte[] b = new byte[ONCE_READ_BYTES];
            StringBuilder stringBuilder = new StringBuilder();
            int len = inputStream.read(b , 0, ONCE_READ_BYTES);
            while(len > 0){
                stringBuilder.append(new String(b,0, len));
                len = inputStream.read(b , 0, ONCE_READ_BYTES);
            }

            String content = stringBuilder.toString();
            if (0 == content.length()){
                throw new S3ServerException(S3Error.MALFORMED_XML,
                        "no body");
            }

            ObjectMapper objectMapper = new XmlMapper();
            VersioningConfiguration versioningCfg = objectMapper.readValue(content, VersioningConfiguration.class);

            String status = versioningCfg.getStatus();
            if (status.equals(VersioningStatusType.ENABLED.getName())
                    || status.equals(VersioningStatusType.SUSPENDED.getName())){
                return status;
            }else {
                throw new S3ServerException(S3Error.BUCKET_INVALID_VERSIONING_STATUS,
                        "invalid status="+status);
            }
        }catch (S3ServerException e){
            throw e;
        } catch (IOException e){
            throw new S3ServerException(S3Error.MALFORMED_XML,
                    "parse versioning status failed", e);
        }catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML,
                    "get versioning status failed", e);
        }
    }
}
