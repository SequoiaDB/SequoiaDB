package com.sequoias3.controller;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.UserParamDefine;
import com.sequoias3.core.Region;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.RegionService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.regex.Pattern;

@RestController
@RequestMapping(RestParamDefine.REST_REGION)
public class RegionController {
    private static final Logger logger = LoggerFactory.getLogger(RegionController.class);

    @Autowired
    RestUtils restUtils;

    @Autowired
    RegionService regionService;

    @PostMapping(params = RestParamDefine.RegionPara.CREATE_REGION, produces = MediaType.APPLICATION_XML_VALUE)
    public void putRegion(@RequestParam(RestParamDefine.RegionPara.REGION_NAME) String regionName,
                          @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                          @RequestBody(required = false) Region regionCon)
            throws S3ServerException {
        logger.info("put region. regionName:{}", regionName.toLowerCase());
        User operator = restUtils.getOperatorByAuthorization(authorization);

        if (!operator.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + operator.getUserName() +
                            ",role=" + operator.getRole());
        }

        checkRegionName(regionName);
        if (null == regionCon){
            regionCon = new Region();
        }


        regionCon.setName(regionName.toLowerCase());
        regionService.putRegion(regionCon);
    }

    @PostMapping(params = RestParamDefine.RegionPara.LIST_REGIONS, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity ListRegions(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        logger.debug("list regions.");
        restUtils.getOperatorByAuthorization(authorization);

        return ResponseEntity.ok()
                .body(regionService.ListRegions());
    }

    @PostMapping(params = RestParamDefine.RegionPara.GET_REGION, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity GetRegion(@RequestParam(RestParamDefine.RegionPara.REGION_NAME) String regionName,
                                    @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        logger.info("get region. regionName:{}", regionName);
        User operator = restUtils.getOperatorByAuthorization(authorization);

        if (!operator.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + operator.getUserName() +
                            ",role=" + operator.getRole());
        }

        return ResponseEntity.ok()
                .body(regionService.getRegion(regionName.toLowerCase()));
    }

    @PostMapping(params = RestParamDefine.RegionPara.HEAD_REGION, produces = MediaType.APPLICATION_XML_VALUE)
    public void headRegion(@RequestParam(RestParamDefine.RegionPara.REGION_NAME) String regionName,
                           @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        logger.info("head region. regionName:{}", regionName);
        User operator = restUtils.getOperatorByAuthorization(authorization);

        if (!operator.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + operator.getUserName() +
                            ",role=" + operator.getRole());
        }

        regionService.headRegion(regionName.toLowerCase());
    }

    @PostMapping(params = RestParamDefine.RegionPara.DELETE_REGION, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity DeleteRegion(@RequestParam(RestParamDefine.RegionPara.REGION_NAME) String regionName,
                                       @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization)
            throws S3ServerException{
        logger.info("delete region. regionName:{}", regionName);
        User operator = restUtils.getOperatorByAuthorization(authorization);

        if (!operator.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + operator.getUserName() +
                            ",role=" + operator.getRole());
        }

        checkRegionName(regionName);

        regionService.deleteRegion(regionName.toLowerCase());

        return ResponseEntity.noContent().build();
    }

    private void checkRegionName(String regionName) throws S3ServerException{
        String pattern = "[a-zA-Z0-9-]{3,20}";
        if (!Pattern.matches(pattern, regionName)){
            throw new S3ServerException(S3Error.REGION_INVALID_REGIONNAME,
                    "region name is invalid. regionName:"+regionName);
        }
    }
}
