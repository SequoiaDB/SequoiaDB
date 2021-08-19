package com.sequoias3.util;

import com.sequoias3.model.CreateRegionRequest;

import java.util.regex.Pattern;

public class RegionUtil {

    public static void validateRegion(CreateRegionRequest request) throws IllegalArgumentException{
        if (null == request){
            throw new IllegalArgumentException("request cannot be null");
        }

        isValidRegionName(request.getRegion().getName());
    }

    public static void isValidRegionName(String regionName){
        if (null == regionName){
            throw new IllegalArgumentException("Region name cannot be null");
        }
        String pattern = "[a-zA-Z0-9-]{3,20}";
        if (!Pattern.matches(pattern, regionName)){
            throw new IllegalArgumentException("region name is invalid. regionName:"+regionName);
        }
    }
}
