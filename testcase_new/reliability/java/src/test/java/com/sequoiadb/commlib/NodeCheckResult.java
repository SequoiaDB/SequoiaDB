/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:NodeCheckResult.java 类的详细描述
 *
 * @author 类创建者姓名 Date:2017-2-27下午12:59:45
 * @version 1.00
 */
package com.sequoiadb.commlib;

public class NodeCheckResult {
    private final int diskThreshold = 134217728;
    public String hostName;
    public String svcName;
    public int nodeID;
    public boolean connect = true;
    public boolean isPrimary = false;
    public int LSNVer = 0;
    public long LSN = -1;
    public boolean serviceStatus = true;
    public long freeSpace = -1;
    public boolean isInDeploy = true;

    public void setGroupCheckResult( GroupCheckResult result ) {
        if ( !connect ) {
            result.connCheck = false;
        }

        if ( result.primaryNode == nodeID && !isPrimary ) {
            result.primaryCheck = false;
        }

        if ( !serviceStatus ) {
            result.serviceCheck = false;
        }

        if ( result.nodesResult.size() > 0
                && ( result.nodesResult.get( 0 ).LSNVer != LSNVer
                        || result.nodesResult.get( 0 ).LSN != LSN ) ) {
            result.LSNCheck = false;
        }

        if ( freeSpace < diskThreshold ) {
            result.diskCheck = false;
        }

        if ( !isInDeploy ) {
            result.deployCheck = false;
        }
    }

    public String toString() {
        return String.format(
                "{\nHostName:\"%s\", " + "\nsvcname:\"%s\", " + "\nNodeID:%d,"
                        + "\nConnect:%s," + "\nIsPrimary:%s," + "\nLSN:%d,"
                        + "\nServiceStatus:%s," + "\nFressSpace:%d,"
                        + "\nisInDeploy:%s\n}",
                hostName, svcName, nodeID, Boolean.toString( connect ),
                Boolean.toString( isPrimary ), LSN,
                Boolean.toString( serviceStatus ), freeSpace,
                Boolean.toString( isInDeploy ) );
    }

}
