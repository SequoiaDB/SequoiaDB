/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: SequoiaSQL OLAP common
@author:
    2016-6-1 David Li  Init
*/

var SSQL_OLAP_COMMON_FILE_NAME = "ssqlOlapCommon.js";

var SsqlOlapReturnValue = function() {
    this[Errno] = SDB_OK,
    this[Detail] = "",
    this[HostName] = "",
    this[Role] = "",
    this[TaskID] = -1
};

SsqlOlapReturnValue.prototype.toString = function() {
    return JSON.stringify(this);
};

var SsqlOlapCommon = function() {
};

SsqlOlapCommon.prototype.checkConfig = function SsqlOlapCommon_checkConfig(config) {
    if (!isTypeOf(config, "object")) {
        throw new SdbError(SDB_INVALIDARG, "invalid config");
    }

    if (!isNotNullString(config[HostName])) {
        throw new SdbError(SDB_INVALIDARG, "HostName is missed or invalid in config");
    }

    if (!isNotNullString(config[Role])) {
        throw new SdbError(SDB_INVALIDARG, "role is missed or invalid in config");
    }

    if (!isNotNullString(config[MasterHost])) {
        throw new SdbError(SDB_INVALIDARG, "master_host is missed or invalid in config");
    }

    if (!isNotNullString(config[MasterPort])) {
        throw new SdbError(SDB_INVALIDARG, "master_port is missed or invalid in config");
    }

    if (!isNotNullString(config[MasterDir])) {
        throw new SdbError(SDB_INVALIDARG, "master_dir is missed or invalid in config");
    }

    if (!isTypeOf(config[StandbyHost], "string")) {
        throw new SdbError(SDB_INVALIDARG, "standby_host is missed or invalid in config");
    }

    if (config[StandbyHost] == "") {
        config[StandbyHost] = "none";
    }

    if (!isNotNullString(config[SegmentPort])) {
        throw new SdbError(SDB_INVALIDARG, "segment_port is missed or invalid in config");
    }

    if (!isNotNullString(config[SegmentDir])) {
        throw new SdbError(SDB_INVALIDARG, "segment_dir is missed or invalid in config");
    }

    if (!isTypeOf(config[SegmentHosts], "object") || !(config[SegmentHosts] instanceof Array) || config[SegmentHosts].length == 0) {
        throw new SdbError(SDB_INVALIDARG, "segment_hosts is missed or invalid in config");
    }

    if (!isNotNullString(config[HdfsUrl])) {
        throw new SdbError(SDB_INVALIDARG, "hdfs_url is missed or invalid in config");
    }

    if (!isNotNullString(config[MasterTempDir])) {
        throw new SdbError(SDB_INVALIDARG, "master_temp_dir is missed or invalid in config");
    }

    if (!isNotNullString(config[SegmentTempDir])) {
        throw new SdbError(SDB_INVALIDARG, "segment_temp_dir is missed or invalid in config");
    }

    if (!isNotNullString(config[InstallDir])) {
        throw new SdbError(SDB_INVALIDARG, "install_dir is missed or invalid in config");
    }

    if (!isNotNullString(config[LogDir])) {
        throw new SdbError(SDB_INVALIDARG, "log_dir is missed or invalid in config");
    }

    if (!isNotNullString(config[MaxConnections])) {
        throw new SdbError(SDB_INVALIDARG, "max_connections is missed or invalid in config");
    }

    if (!isNotNullString(config[SharedBuffers])) {
        throw new SdbError(SDB_INVALIDARG, "shared_buffers is missed or invalid in config");
    }

    if (!isNotNullString(config[User])) {
        throw new SdbError(SDB_INVALIDARG, "User is missed or invalid in config");
    }

    if (!isNotNullString(config[Passwd])) {
        throw new SdbError(SDB_INVALIDARG, "Passwd is missed or invalid in config");
    }

    if (!isNotNullString(config[SshPort])) {
        throw new SdbError(SDB_INVALIDARG, "SshPort is missed or invalid in config");
    }

    if (config[IsSingle] != undefined && typeof(config[IsSingle]) != "string") {
        throw new SdbError(SDB_INVALIDARG, "is_single is invalid in config");
    }
};

SsqlOlapCommon.prototype.checkSysInfo = function SsqlOlapCommon_checkSysInfo(sysInfo) {
    if (!isTypeOf(sysInfo, "object")) {
        throw new SdbError(SDB_INVALIDARG, "invalid sysInfo");
    }

    if (!isTypeOf(sysInfo[TaskID], "number")) {
        throw new SdbError(SDB_INVALIDARG, "TaskID is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[ClusterName])) {
        throw new SdbError(SDB_INVALIDARG, "ClusterName is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[BusinessType]) || sysInfo[BusinessType] != Sequoiasql) {
        throw new SdbError(SDB_INVALIDARG, "BusinessType is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[BusinessName])) {
        throw new SdbError(SDB_INVALIDARG, "BusinessName is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[DeployMod]) || sysInfo[DeployMod] != Olap) {
        throw new SdbError(SDB_INVALIDARG, "DeployMod is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[SdbUser])) {
        throw new SdbError(SDB_INVALIDARG, "SdbUser is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[SdbPasswd])) {
        throw new SdbError(SDB_INVALIDARG, "SdbPasswd is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[SdbUserGroup])) {
        throw new SdbError(SDB_INVALIDARG, "SdbUserGroup is missed or invalid in sysInfo");
    }

    if (!isNotNullString(sysInfo[InstallPacket])) {
        throw new SdbError(SDB_INVALIDARG, "InstallPacket is missed or invalid in sysInfo");
    }
};

SsqlOlapCommon.prototype.connectToHost = function SsqlOlapCommon_connectToHost(hostName, user, passwd, sshPort) {
    var ssh;
    try {
        ssh = new Ssh(hostName, user, passwd, sshPort);
        return ssh;
    } catch(e) {
        var msg = sprintf("failed to connect to ?@?:? through SSH", user, hostName, sshPort);
        throw new SdbError(e, msg);
    }
};

SsqlOlapCommon.prototype.needInstall = function SsqlOlapCommon_needInstall(config) {
    if (!isObject(config)) {
        throw new SdbError(SDB_INVALIDARG, "invalid config object");
    }

    try {
        var role = config[Role];
        if (role == Master || role == Standby) {
            return true;
        }

        if (role == Segment) {
            var isSingle = config[IsSingle];
            if (isSingle != undefined && isSingle == "true") {
                return true;
            }
        }

        return false;
    } catch(e) {
        throw new SdbError(e);
    }
};

var ObjCluster = "cluster";
var ObjMaster = "master";
var ObjSegment = "segment";
var ObjStandby = "standby";
var ObjAllSegments = "allsegments";

SsqlOlapCommon.prototype.start = function SsqlOlapCommon_start(sdbSsh, config, object) {
    if (!(isObject(sdbSsh) && (sdbSsh instanceof Ssh))) {
        throw new SdbError(SDB_INVALIDARG, "sdbSsh should be instance of Ssh");
    }

    if (object != ObjCluster &&
        object != ObjMaster &&
        object != ObjSegment &&
        object != ObjStandby &&
        object != ObjAllSegments) {
        throw new SdbError(SDB_INVALIDARG, sprintf("invalid object: ?", object));
    }

    var hostName = config[HostName];
    var installDir = config[InstallDir];

    if (!sdbSsh.isPathExist(installDir)) {
        throw new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", installDir, hostName));
    }

    var binDir = adaptPath(installDir) + "bin";
    if (!sdbSsh.isPathExist(binDir)) {
        throw new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", binDir, hostName));
    }

    var env = adaptPath(installDir) + "sequoiasql_path.sh";
    var ssql = adaptPath(binDir) + "ssql";
    if (!sdbSsh.isPathExist(ssql)) {
        throw new SdbError(SDB_SYS, sprintf("the file [?] does not exist in host [?]", ssql, hostName));
    }
    if (!sdbSsh.isFile(ssql)) {
        throw new SdbError(SDB_SYS, sprintf("the path [?] is not file in host [?]", ssql, hostName));
    }

    var shell = "source " + env + "; ";
    shell += ssql + " start " + object;
    shell += " -a -v";

    try {
        sdbSsh.exec(shell);
    } catch(e) {
    }

    if (sdbSsh.getLastRet() != 0) {
        var msg = sdbSsh.getLastOut();
        throw new SdbError(SDB_SYS, msg);
    }
};

SsqlOlapCommon.prototype.stop = function SsqlOlapCommon_stop(sdbSsh, config, object) {
    if (!(isObject(sdbSsh) && (sdbSsh instanceof Ssh))) {
        throw new SdbError(SDB_INVALIDARG, "sdbSsh should be instance of Ssh");
    }

    if (object != ObjCluster &&
        object != ObjMaster &&
        object != ObjSegment &&
        object != ObjStandby &&
        object != ObjAllSegments) {
        throw new SdbError(SDB_INVALIDARG, sprintf("invalid object: ?", object));
    }

    var hostName = config[HostName];
    var installDir = config[InstallDir];

    if (!sdbSsh.isPathExist(installDir)) {
        throw new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", installDir, hostName));
    }

    var binDir = adaptPath(installDir) + "bin";
    if (!sdbSsh.isPathExist(binDir)) {
        throw new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", binDir, hostName));
    }

    var env = adaptPath(installDir) + "sequoiasql_path.sh";
    var ssql = adaptPath(binDir) + "ssql";
    if (!sdbSsh.isPathExist(ssql)) {
        throw new SdbError(SDB_SYS, sprintf("the file [?] does not exist in host [?]", ssql, hostName));
    }
    if (!sdbSsh.isFile(ssql)) {
        throw new SdbError(SDB_SYS, sprintf("the path [?] is not file in host [?]", ssql, hostName));
    }

    var shell = "source " + env + "; ";
    shell += ssql + " stop " + object;
    shell += " -a -v";

    try {
        sdbSsh.exec(shell);
    } catch(e) {
    }

    if (sdbSsh.getLastRet() != 0) {
        var msg = sdbSsh.getLastOut();
        throw new SdbError(SDB_SYS, msg);
    }
};
