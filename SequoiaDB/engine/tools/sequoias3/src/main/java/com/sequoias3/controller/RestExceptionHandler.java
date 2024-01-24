package com.sequoias3.controller;

import com.sequoias3.core.Error;
import com.sequoias3.exception.S3ServerException;
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

@ControllerAdvice
public class RestExceptionHandler {
    private static final Logger logger = LoggerFactory.getLogger(RestExceptionHandler.class);
    private static final String ERROR_ATTRIBUTE = "X-S3-ERROR";

    @Autowired
    RestUtils restUtils;

    @ExceptionHandler(S3ServerException.class)
    @ResponseBody
    public ResponseEntity s3ExceptionHandler(S3ServerException e, HttpServletRequest request,
                                             HttpServletResponse response) {
        String msg = String.format("request=%s, errcode=%s, message=%s", request.getRequestURI(), e.getError()
                .getCode(), e.getMessage());
        logger.error(msg, e);

        HttpStatus status = restUtils.convertStatus(e);

        Error exceptionBody = new Error(e, request.getRequestURI());
        if ("HEAD".equalsIgnoreCase(request.getMethod())){
            return ResponseEntity.status(status).build();
        } else {
            return ResponseEntity.status(status).body(exceptionBody);
        }
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity unexpectedExceptionHandler(Exception e, HttpServletRequest request,
                                                     HttpServletResponse response) throws Exception {
        String msg = String.format("request=%s", request.getRequestURI());
        logger.error(msg, e);

        HttpStatus status = HttpStatus.INTERNAL_SERVER_ERROR;

        Error exceptionBody = new Error(e, request.getRequestURI());
        if ("HEAD".equalsIgnoreCase(request.getMethod())) {
            String error = exceptionBody.toString();
            response.setHeader(ERROR_ATTRIBUTE, error);
            return ResponseEntity.status(status).build();
        } else {
            return ResponseEntity.status(status).body(exceptionBody);
        }
    }
}