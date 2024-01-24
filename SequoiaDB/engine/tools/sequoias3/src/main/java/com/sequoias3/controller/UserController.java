package com.sequoias3.controller;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.common.UserParamDefine;
import com.sequoias3.core.User;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.service.UserService;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping(RestParamDefine.REST_USERS)
public class UserController {
    private static final Logger logger = LoggerFactory.getLogger(UserController.class);

    @Autowired
    UserService userService;

    @Autowired
    RestUtils restUtils;

    @PostMapping(params = RestParamDefine.UserPara.CREATE_USER, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity createUser(@RequestParam(value = RestParamDefine.UserPara.ROLE, required = false) String role,
                                     @RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                     @RequestParam(RestParamDefine.UserPara.USER_NAME) String username)
            throws S3ServerException {
        User adminUser = restUtils.getOperatorByAuthorization(authorization);

        if (!adminUser.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + adminUser.getUserName() + ",role=" + adminUser.getRole());
        }

        logger.info("create user. admin={},user={},role={}", adminUser.getUserName(), username, role);
        return ResponseEntity.ok()
                .body(userService.createUser(username, role));
    }

    @PostMapping(params = RestParamDefine.UserPara.CREATE_ACCESSKEY, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity createAccessKey(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                          @RequestParam(RestParamDefine.UserPara.USER_NAME) String username)
            throws S3ServerException {
        User adminUser = restUtils.getOperatorByAuthorization(authorization);

        if (!adminUser.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + adminUser.getUserName() + ",role=" + adminUser.getRole());
        }

        logger.info("create AccessKey. admin={},user={}", adminUser.getUserName(), username);
        return ResponseEntity.ok()
                .body(userService.updateUser(username));
    }

    @PostMapping(params = RestParamDefine.UserPara.GET_ACCESSKEY, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity getAccessKey(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                       @RequestParam(RestParamDefine.UserPara.USER_NAME) String username)
            throws S3ServerException {
        User adminUser = restUtils.getOperatorByAuthorization(authorization);

        if (!adminUser.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + adminUser.getUserName() + ",role=" + adminUser.getRole());
        }

        logger.info("get AccessKey. admin={},user={}", adminUser.getUserName(), username);
        return ResponseEntity.ok()
                .body(userService.getUser(username));
    }

    @PostMapping(params = RestParamDefine.UserPara.DELETE_USER, produces = MediaType.APPLICATION_XML_VALUE)
    public ResponseEntity deleteUser(@RequestHeader(value = RestParamDefine.AUTHORIZATION, required = false) String authorization,
                                     @RequestParam(RestParamDefine.UserPara.USER_NAME) String username,
                                     @RequestParam(value = RestParamDefine.UserPara.FORCE, required = false) String force)
            throws S3ServerException {
        User adminUser = restUtils.getOperatorByAuthorization(authorization);

        if (!adminUser.getRole().equals(UserParamDefine.ROLE_ADMIN)) {
            throw new S3ServerException(S3Error.INVALID_ADMINISTRATOR,
                    "not an admin user. operator = " + adminUser.getUserName() + ",role=" + adminUser.getRole());
        }

        Boolean forceDelete = false;
        if (force != null){
            if (force.equalsIgnoreCase("true")){
                forceDelete = true;
            }else if (force.equalsIgnoreCase("false")){
                forceDelete = false;
            }else {
                throw new S3ServerException(S3Error.INVALID_ARGUMENT, "Invalid force parameter. force:"+force);
            }
        }

        logger.info("delete user. admin={},user={},force={}",
                adminUser.getUserName(), username, forceDelete);
        userService.deleteUser(username, forceDelete);
        logger.info("delete user success. admin={},user={},force={}",
                adminUser.getUserName(), username, forceDelete);
        return ResponseEntity.ok().build();
    }
}
