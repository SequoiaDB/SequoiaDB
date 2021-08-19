package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.DelimiterConfiguration;
import com.sequoias3.service.BucketDelimiterService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;
import java.net.URLDecoder;

@RestController
public class BucketDelimiterController {
    private static final Logger logger = LoggerFactory.getLogger(BucketDelimiterController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketDelimiterService bucketDelimiterService;

    @PutMapping(value = "/{bucketname:.+}", params = RestParamDefine.DELIMITER,
            produces = MediaType.APPLICATION_XML_VALUE)
    public void putBucketDelimiter(@PathVariable("bucketname") String bucketName,
                                   @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                   @RequestHeader(value = RestParamDefine.DelimiterHeader.ENCODING_TYPE, required = false) String encodingType,
                                   HttpServletRequest request )
            throws S3ServerException {
        User operator = restUtils.getOperatorByAuthorization(authorization);

        DelimiterConfiguration delimiterCon = getDelimiterConfig(request, encodingType);

        if (null != encodingType) {
            if (!encodingType.equals(RestParamDefine.ENCODING_TYPE_URL)) {
                throw new S3ServerException(S3Error.OBJECT_INVALID_ENCODING_TYPE, "encoding type must be url");
            }
        }

        logger.info("put delimiter. bucket=" + bucketName + "@delimiter=" + delimiterCon.getDelimiter());

        bucketDelimiterService.putBucketDelimiter(operator.getUserId(), bucketName, delimiterCon.getDelimiter());

        logger.info("put delimiter success. bucket=" + bucketName + "@delimiter=" + delimiterCon.getDelimiter());
    }

    @GetMapping(value = "/{bucketname:.+}", params = RestParamDefine.DELIMITER,
            produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getBucketDelimiter(@PathVariable("bucketname") String bucketName,
                                             @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                             @RequestHeader(value = RestParamDefine.DelimiterHeader.ENCODING_TYPE, required = false) String encodingType)
            throws S3ServerException{
        User operator = restUtils.getOperatorByAuthorization(authorization);
        logger.debug("get delimiter. bucket={}, encodingType:{}", bucketName, encodingType);

        if (null != encodingType) {
            if (!encodingType.equals(RestParamDefine.ENCODING_TYPE_URL)) {
                throw new S3ServerException(S3Error.OBJECT_INVALID_ENCODING_TYPE, "encoding type must be url");
            }
        }

        return ResponseEntity.ok(bucketDelimiterService.getBucketDelimiter(operator.getUserId(), bucketName, encodingType));
    }

    private DelimiterConfiguration getDelimiterConfig(HttpServletRequest httpServletRequest, String encodingType)
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
                DelimiterConfiguration configuration = objectMapper.readValue(content, DelimiterConfiguration.class);
                if (encodingType != null) {
                    configuration.setDelimiter(URLDecoder.decode(configuration.getDelimiter(), "UTF-8"));
                }
                return configuration;
            }else {
                return null;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML, "get delimiter failed", e);
        }finally {
            try {
                httpServletRequest.getInputStream().close();
            }catch (Exception e2){
                logger.warn("close inputStream failed", e2);
            }
        }
    }
}
