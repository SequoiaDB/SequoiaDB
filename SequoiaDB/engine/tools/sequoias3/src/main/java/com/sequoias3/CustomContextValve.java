package com.sequoias3;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.Bucket;
import com.sequoias3.core.Error;
import com.sequoias3.core.UploadMeta;
import com.sequoias3.core.User;
import com.sequoias3.dao.UploadDao;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.ObjectUri;
import com.sequoias3.service.BucketService;
import com.sequoias3.utils.RestUtils;
import org.apache.catalina.Valve;
import org.apache.catalina.connector.Request;
import org.apache.catalina.connector.Response;
import org.apache.catalina.valves.ValveBase;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;

import javax.servlet.ServletException;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class CustomContextValve extends ValveBase {
    private static final Logger logger = LoggerFactory.getLogger(CustomContextValve.class);
    @Autowired
    RestUtils restUtils;

    @Autowired
    BucketService bucketService;

    @Autowired
    UploadDao uploadDao;

    @Autowired
    SystemStatus systemStatus;

    @Override
    public void invoke(Request request, Response response) throws IOException, ServletException {
        if (systemStatus.getSystemStatus() != SystemStatus.STATUS_NORMAL){
            response.setStatus(500);
            ObjectMapper objectMapper = new XmlMapper();
            Error exceptionBody = new Error(new S3ServerException(S3Error.UNKNOWN_ERROR, "system status is not normal"), request.getRequestURI());
            response.getResponse().getOutputStream().write(objectMapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(exceptionBody));
            return;
        }

        if (request.getHeader(RestParamDefine.EXPECT) != null
                && request.getMethod().equals("PUT")
                && request.getContentLength() > 512 * 1024
                ){
            logger.debug("get expect.");
            try {
                ObjectUri objectUri = null;
                try {
                    objectUri =new ObjectUri(request.getRequestURI());
                }catch (Exception e){
                    //do nothing. 只对对象的put操作做检查，如果不是对象操作，不做下面的检查
                }
                if (objectUri != null) {
                    User operator = restUtils.getOperatorByAuthorization(request.getHeader(RestParamDefine.AUTHORIZATION));
                    Bucket bucket = bucketService.getBucket(operator.getUserId(), objectUri.getBucketName());

                    if (request.getParameter(RestParamDefine.UPLOADID) != null) {
                        long uploadId = restUtils.convertUploadId(request.getParameter(RestParamDefine.UPLOADID));
                        UploadMeta upload = uploadDao.queryUploadByUploadId(null, bucket.getBucketId(),
                                objectUri.getObjectName(), uploadId, false);
                        if (upload == null || upload.getUploadStatus() != UploadMeta.UPLOAD_INIT) {
                            throw new S3ServerException(S3Error.PART_NO_SUCH_UPLOAD, "no such upload. uploadId:" + uploadId);
                        }
                        request.setAttribute(RestParamDefine.Attribute.S3_UPLOADID, uploadId);
                    }

                    //uploadPart
                    if (request.getParameter(RestParamDefine.PARTNUMBER) != null) {
                        int partNumber = restUtils.convertPartNumber(request.getParameter(RestParamDefine.PARTNUMBER));
                        if (partNumber < RestParamDefine.PART_NUMBER_MIN
                                || partNumber > RestParamDefine.PART_NUMBER_MAX) {
                            throw new S3ServerException(S3Error.PART_INVALID_PARTNUMBER,
                                    "invalid partNumber:" + partNumber);
                        }
                        request.setAttribute(RestParamDefine.Attribute.S3_PARTNUMBER, partNumber);
                    } else { //put object
                        if (objectUri.getObjectName().length() > RestParamDefine.KEY_LENGTH) {
                            throw new S3ServerException(S3Error.OBJECT_KEY_TOO_LONG,
                                    "ObjectName is too long. objectName:" + objectUri.getObjectName());
                        }
                        Map<String, String> requestHeaders = new HashMap<>();
                        Map<String, String> xMeta = new HashMap<>();
                        restUtils.getHeaders(request, requestHeaders, xMeta);
                        if (restUtils.getXMetaLength(xMeta) > RestParamDefine.X_AMZ_META_LENGTH) {
                            throw new S3ServerException(S3Error.OBJECT_METADATA_TOO_LARGE,
                                    "metadata headers exceed the maximum. xMeta:" + xMeta.toString());
                        }

                        request.setAttribute(RestParamDefine.Attribute.S3_HEADERS, requestHeaders);
                        request.setAttribute(RestParamDefine.Attribute.S3_XMETA, xMeta);
                    }

                    request.setAttribute(RestParamDefine.Attribute.S3_OPERATOR, operator);
                    request.setAttribute(RestParamDefine.Attribute.S3_BUCKET, bucket);
                    request.setAttribute(RestParamDefine.Attribute.S3_OBJECTURI, objectUri);
                }
            }catch (S3ServerException e){
                String msg = String.format("request=%s, errcode=%s, message=%s", request.getRequestURI(),
                        e.getError().getCode(), e.getMessage());
                logger.error(msg, e);
                response.setStatus(restUtils.convertStatus(e).value());
                ObjectMapper objectMapper = new XmlMapper();
                Error exceptionBody = new Error(e, request.getRequestURI());
                response.getResponse().getOutputStream().write(objectMapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(exceptionBody));
                return;
            } catch (Exception e){
                String msg = String.format("request=%s", request.getRequestURI());
                logger.error(msg, e);
                response.setStatus(500);
                ObjectMapper objectMapper = new XmlMapper();
                Error exceptionBody = new Error(e, request.getRequestURI());
                response.getResponse().getOutputStream().write(objectMapper.writerWithDefaultPrettyPrinter().writeValueAsBytes(exceptionBody));
                return;
            }
//            request.getCoyoteRequest().setExpectation(false);
        }

        Valve next = getNext();
        if (next == null){
            return;
        }
        next.invoke(request, response);
    }
}
