package com.sequoias3.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.xml.XmlMapper;
import com.sequoias3.common.RestParamDefine;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.model.*;
import com.sequoias3.service.ACLService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.*;

import javax.servlet.ServletInputStream;
import javax.servlet.http.HttpServletRequest;

@Controller
public class ACLController {
    private static final Logger logger = LoggerFactory.getLogger(ACLController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    ACLService aclService;

    @PutMapping(value = "/{bucketName:.+}", params = RestParamDefine.ACL, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putBucketAcl(@PathVariable("bucketName") String bucketName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("put bucket acl. bucketName={}, operator={}", bucketName, operator.getUserName());

            boolean isCannedacl = false;
            boolean isGrantacl = false;
            boolean isAccessacl = false;
            boolean findOne = false;
            if (httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_ACL) != null) {
                isCannedacl = true;
                findOne = true;
            }

            if (httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ_ACP) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE_ACP) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_FULL_CONTROL) != null) {
                if (findOne) {
                    throw new S3ServerException(S3Error.ACL_CONFLICT, "cannot specified two acl");
                }
                isGrantacl = true;
                findOne = true;
            }

            AccessControlPolicy accessControlPolicy = getAccessControlPolicy(httpServletRequest);
            if (accessControlPolicy != null) {
                if (findOne) {
                    throw new S3ServerException(S3Error.ACL_CONFLICT, "cannot specified two acl");
                }
                isAccessacl = true;
            }

            AccessControlPolicy acl = null;
            if (isAccessacl) {
                acl = accessControlPolicy;
            }

            if (isCannedacl) {
                acl = new AccessControlPolicy();
                String amzAcl = httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_ACL).trim();
                parseCannedHeader(acl, amzAcl, operator);
            }

            if (isGrantacl) {
                acl = new AccessControlPolicy();
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_READ, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_WRITE, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_READ_ACP, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ_ACP));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_WRITE_ACP, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE_ACP));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_FULLCONTROL, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_FULL_CONTROL));
            }

            aclService.putBucketAcl(operator.getUserId(), bucketName, acl);
            return ResponseEntity.ok().build();
        }catch (Exception e){
            logger.error("put bucket acl failed. bucketName={}, authorization={}", bucketName, authorization);
            throw e;
        }
    }

    @GetMapping(value = "/{bucketName:.+}", params = RestParamDefine.ACL, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getBucketAcl(@PathVariable("bucketName") String bucketName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            logger.debug("get bucket acl. bucketName={}, operator={}", bucketName, operator.getUserName());

            return ResponseEntity.ok()
                    .body(aclService.getBucketAcl(operator.getUserId(), bucketName));
        }catch (Exception e){
            logger.debug("get bucket acl failed. bucketName={}, authorization={}", bucketName, authorization);
            throw e;
        }
    }

    @PutMapping(value = "/{bucketName:.+}/**", params = RestParamDefine.ACL, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity putObjectAcl(@PathVariable("bucketName") String bucketName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       @RequestParam(value = "versionId", required = false) String versionId,
                                       HttpServletRequest httpServletRequest)
            throws S3ServerException {
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("put object acl. bucketName={}, objectName={}", bucketName, objectName);

            boolean isCannedacl = false;
            boolean isGrantacl = false;
            boolean isAccessacl = false;
            boolean findOne = false;
            if (httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_ACL) != null) {
                isCannedacl = true;
                findOne = true;
            }

            if (httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ_ACP) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE_ACP) != null
                    || httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_FULL_CONTROL) != null) {
                if (findOne) {
                    throw new S3ServerException(S3Error.ACL_CONFLICT, "cannot specified two acl");
                }
                isGrantacl = true;
                findOne = true;
            }

            AccessControlPolicy accessControlPolicy = getAccessControlPolicy(httpServletRequest);
            if (accessControlPolicy != null) {
                if (findOne) {
                    throw new S3ServerException(S3Error.ACL_CONFLICT, "cannot specified two acl");
                }
                isAccessacl = true;
            }

            AccessControlPolicy acl = null;
            if (isAccessacl) {
                acl = accessControlPolicy;
            }

            if (isCannedacl) {
                acl = new AccessControlPolicy();
                String amzAcl = httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_ACL).trim();
                parseCannedHeader(acl, amzAcl, operator);
            }

            if (isGrantacl) {
                acl = new AccessControlPolicy();
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_READ, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_WRITE, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_READ_ACP, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_READ_ACP));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_WRITE_ACP, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_WRITE_ACP));
                parseGrantHeader(acl, RestParamDefine.Acl.ACL_FULLCONTROL, httpServletRequest.getHeader(RestParamDefine.Acl.X_AMZ_GRANT_FULL_CONTROL));
            }

            String reVersionId = aclService.putObjectAcl(operator.getUserId(), new ObjectUri(bucketName, objectName, versionId), acl);
            HttpHeaders headers = new HttpHeaders();
            if (reVersionId != null){
                headers.add(RestParamDefine.Acl.VERSION_ID, reVersionId);
            }
            return ResponseEntity.ok()
                    .headers(headers)
                    .build();
        }catch (Exception e){
            logger.error("put object acl failed. bucketName={}, authorization={}", bucketName, authorization);
            throw e;
        }
    }

    @GetMapping(value = "/{bucketName:.+}/**", params = RestParamDefine.ACL, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getObjectAcl(@PathVariable("bucketName") String bucketName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       @RequestParam(value = "versionId", required = false) String versionId,
                                       HttpServletRequest httpServletRequest)
            throws S3ServerException{
        try {
            User operator = restUtils.getOperatorByAuthorization(authorization);
            String objectName = restUtils.getObjectNameByURI(httpServletRequest.getRequestURI());
            logger.debug("get object acl. bucketName={}, objectName={}", bucketName, objectName);

            return ResponseEntity.ok()
                    .body(aclService.getObjectAcl(operator.getUserId(),
                            new ObjectUri(bucketName, objectName, versionId)));
        }catch (Exception e){
            logger.error("get object acl failed. bucketName={}, authorization={}", bucketName, authorization);
            throw e;
        }
    }

    private boolean parseGrantHeader(AccessControlPolicy acl, String permission,
                                     String grantRead){
        if (grantRead == null){
            return false;
        }
        String[] grantReaders = grantRead.trim().split(",");
        for (int i=0; i < grantReaders.length; i++){
            String reader = grantReaders[i].trim();
            Grant granti = new Grant();
            granti.setPermission(permission);
            if (reader.startsWith("id=")){
                granti.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_USER, Long.parseLong(reader.substring(3)), null, null, null));
            }else if (reader.startsWith("uri=")){
                granti.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, reader.substring(4), null));
            }else if (reader.startsWith("emailAddress=")){
                granti.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_EMAIL, null, null, null, reader.substring(13)));
            }else {
                continue;
            }
            acl.getGrants().add(granti);
        }
        return true;
    }

    private void parseCannedHeader(AccessControlPolicy acl, String amzAcl, User operator){
        Grant grant1 = new Grant();
        grant1.setPermission(RestParamDefine.Acl.ACL_FULLCONTROL);
        grant1.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_USER, operator.getUserId(), operator.getUserName(), null, null));
        acl.getGrants().add(grant1);

        if (amzAcl.equals(RestParamDefine.Acl.CANNED_PUBLICREAD)){
            Grant grant2 = new Grant();
            grant2.setPermission(RestParamDefine.Acl.ACL_READ);
            grant2.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, RestParamDefine.Acl.ALLUSERS, null));
            acl.getGrants().add(grant2);
        }else if (amzAcl.equals(RestParamDefine.Acl.CANNED_PUBLICREADWRITE)){
            Grant grant2 = new Grant();
            grant2.setPermission(RestParamDefine.Acl.ACL_READ);
            grant2.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, RestParamDefine.Acl.ALLUSERS, null));
            acl.getGrants().add(grant2);
            Grant grant3 = new Grant();
            grant3.setPermission(RestParamDefine.Acl.ACL_WRITE);
            grant3.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, RestParamDefine.Acl.ALLUSERS, null));
            acl.getGrants().add(grant3);
        }else if (amzAcl.equals(RestParamDefine.Acl.CANNED_AUTHENTICATEDREAD)){
            Grant grant2 = new Grant();
            grant2.setPermission(RestParamDefine.Acl.ACL_READ);
            grant2.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, RestParamDefine.Acl.AUTHENTICATEDUSERS, null));
            acl.getGrants().add(grant2);
        }else if (amzAcl.equals(RestParamDefine.Acl.CANNED_LOGDELIVERYWRITE)){
            Grant grant2 = new Grant();
            grant2.setPermission(RestParamDefine.Acl.ACL_WRITE);
            grant2.setGrantee(new Grantee(RestParamDefine.Acl.TYPE_GROUP, null, null, RestParamDefine.Acl.LOGDELIVERY, null));
            acl.getGrants().add(grant2);
        }
    }

    private AccessControlPolicy getAccessControlPolicy(HttpServletRequest httpServletRequest)
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
                return objectMapper.readValue(content, AccessControlPolicy.class);
            }else {
                return null;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.MALFORMED_XML, "get AccessControlPolicy failed", e);
        }
    }
}
