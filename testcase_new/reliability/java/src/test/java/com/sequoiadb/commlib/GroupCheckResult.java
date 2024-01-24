/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:GroupCheckResult.java 类的详细描述
 *
 * @author 类创建者姓名 Date:2017-2-27下午12:54:17
 * @version 1.00
 */
package com.sequoiadb.commlib;

import java.util.ArrayList;
import java.util.List;

public class GroupCheckResult {
    public boolean connCheck = true;
    public boolean primaryCheck = true;
    public boolean LSNCheck = true;
    public boolean serviceCheck = true;
    public boolean diskCheck = true;
    public boolean deployCheck = true;
    public int primaryNode = -1;

    public String groupName;
    public int groupID;

    public List< NodeCheckResult > nodesResult = new ArrayList< NodeCheckResult >();

    public void addNodeCheckResult( NodeCheckResult res ) {
        res.setGroupCheckResult( this );
        nodesResult.add( res );
    }

    public String toString() {
        String ret = String.format(
                "{GroupName:%s, " + "\nGroupID:%d," + "\nPrimaryNode:%d,"
                        + "\nConnCheck:%s," + "\nPrimaryCheck:%s,"
                        + "\nLSNCheck:%s," + "\nServiceCheck:%s,"
                        + "\nDiskCheck:%s," + "\nDeployCheck:%s\n",
                groupName, groupID, primaryNode, Boolean.toString( connCheck ),
                Boolean.toString( primaryCheck ), Boolean.toString( LSNCheck ),
                Boolean.toString( serviceCheck ), Boolean.toString( diskCheck ),
                Boolean.toString( deployCheck ) );
        ret += "[";
        for ( NodeCheckResult nodeResult : nodesResult ) {
            ret += nodeResult.toString();
        }
        ret += "]";
        ret += "}";
        return ret;
    }

    public boolean check() {
        return check( false );
    }

    public boolean check( boolean ignoreInDepoly ) {
        if ( ignoreInDepoly ) {
            return connCheck && primaryCheck && serviceCheck;
        } else {
            return connCheck && primaryCheck && serviceCheck && deployCheck;
        }
    }

    public boolean checkWithLSN( boolean ignoreIndeploy ) {
        boolean ret = connCheck && primaryCheck && serviceCheck && LSNCheck;
        if ( !ignoreIndeploy ) {
            ret = ret && deployCheck;
        }
        return ret;
    }

    public boolean checkWithLSNAndDiskThreshold() {
        return connCheck && primaryCheck && serviceCheck && deployCheck
                && LSNCheck && diskCheck;
    }
}
