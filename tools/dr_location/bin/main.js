// dr_location 工具 - 主入口文件

// 是否按 domain 过滤 show 的展示信息
var showFilterWithDomain = true;
// domain 条件
var domainList = [];
var domainMode = 1;

// 在文件开头加载配置文件
if (typeof configJs !== 'undefined' && "" != configJs) {
    import(configJs);
} else {
    import('../config/config.js');
}
import('../lib/lib.js');

// 全局对象
var db = null;
var dc = null;

// 从 -f/--file 文件中读取的节点信息
var nodeTargetInfo = null;

// 输出文件
var showFile = scriptDir + "/output/show.out";
var checkFile = scriptDir + "/output/check.out";
var defaultLocationFile = scriptDir + "/output/location.info";

// 创建上层目录
try {
    if (!File.exist(scriptDir + "/output")) {
        File.mkdir(scriptDir + "/output", 0755);
    }
} catch(error) {
    throw new Error("[ERROR] Failed to create dir [ " + scriptDir + "/output ], error info: " + getLastErrObj());
}

// 显示帮助信息
function showHelp() {
    var helpText =
        "Usage: sdb -f bin/main.js -e \"var mode = 'command';  ... \"\n\n" +
        "Commands:\n" +
        "  show          Show current cluster location information\n" +
        "  check         Check location configuration against expected\n" +
        "  init          Initialize location configuration\n" +
        "  start_maintenance  Start MaintenanceMode\n" +
        "  stop_maintenance   Stop MaintenanceMode\n" +
        "  start_critical     Start CriticalMode\n" +
        "  stop_critical      Stop CriticalMode\n" +
        "  restore          Restore cluster (stop all modes)\n\n" +
        "Variables:\n" +
        "  mode          Command name\n" +
        "  c             Config file path\n" +
        "  file          Node information file\n" +
        "  l             Location name\n" +
        "  H             Hostname(s)\n" +
        "  n             Nodename(s)\n" +
        "  d             Domain(s)\n" +
        "  check         Enable check before stopping mode (1 or 0)\n\n" +
        "Examples:\n" +
        "  sdb -f bin/main.js -e \"var mode = 'show'\"\n" +
        "  sdb -f bin/main.js -e \"var mode = 'check'\"\n" +
        "  sdb -f bin/main.js -e \"var mode = 'init'\"\n" +
        "  sdb -f bin/main.js -e \"var mode = 'start_maintenance'; var l = 'GuangZhou'\"\n" +
        "  sdb -f bin/main.js -e \"var mode = 'stop_maintenance'; var H = 'host1,host2'; var check = 1\"\n" +
        "  sdb -f bin/main.js -e \"var mode = 'restore'\"";

    println(helpText);
}

// 连接到 SequoiaDB
function connectToSdb() {
    print("[INFO] Connect sdb ... ");
    try {
        // 设置连接参数（使用全局变量）
        var cipherUser = "";
        if ('' != sdbCipherFile) {
            if ('' != sdbToken) {
                cipherUser = new CipherUser(sdbUser).token(sdbToken).cipherFile(sdbCipherFile);
            } else {
                cipherUser = new CipherUser(sdbUser).cipherFile(sdbCipherFile);
            }
            db = new Sdb(sdbCoord.split(':')[0], sdbCoord.split(':')[1], cipherUser);
        } else {
            db = new Sdb(sdbCoord.split(':')[0], sdbCoord.split(':')[1], sdbUser, sdbPassword);
        }
    } catch (e) {
        if (db !== null) {
            db.close();
            db = null;
        }
        println("Error connecting to SequoiaDB: " + e);
        throw e;
    }
    println("ok");
}

function getDc() {
    try {
        dc = db.getDC();
    } catch (e) {
        println("Error getting dc: " + e);
        throw e;
    }
}

// 断开连接
function disconnectFromSdb() {
    // 清空全局对象
    if (db !== null) {
        db.close();
        db = null;
    }
    dc = null;
}

// 获取 dc.locationAnalyze() 结果
function locationAnalyze() {
    try {
        var result = dc.locationAnalyze().toObj();
        // if (0 == result.LocationInfo.length) {
        //     // 完全没有设置 location，没有结果
        //     result = [];
        // }
    } catch (e) {
        println("Error in dc.locationAnalyze(): " + e);
        throw e;
    }
    return result;
}

// 获取 dc.reelectAnalyze() 结果
function reelectAnalyze() {
    try {
        var result = dc.reelectAnalyze().toObj();
    } catch (e) {
        println("Error in dc.reelectAnalyze(): " + e);
        throw e;
    }
    return result;
}

// 获取 SDB_LIST_DOMAINS 列表
function listDomains() {
    var cursor = null;
    try {
        var result = [];
        cursor = db.list(SDB_LIST_DOMAINS);
        while (cursor.next()) {
            result.push(cursor.current().toObj());
        }
    } catch (e) {
        println("Error in db.list(SDB_LIST_DOMAINS): " + e);
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return result;
}

// 获取 SDB_LIST_GROUPMODES 列表
function listGroupModes() {
    try {
        var result = [];
        var cursor = db.list(SDB_LIST_GROUPMODES);
        while (cursor.next()) {
            result.push(cursor.current().toObj());
        }
    } catch (e) {
        println("Error in db.list(SDB_LIST_GROUPMODES): " + e);
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return result;
}

// 获取 SDB_LIST_GROUPS 列表
function listGroups() {
    try {
        var result = [];
        var cursor = db.list(SDB_LIST_GROUPS);
        while (cursor.next()) {
            result.push(cursor.current().toObj());
        }
    } catch (e) {
        println("Error in db.list(SDB_LIST_GROUPS): " + e);
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return result;
}

// 获取当前节点信息
function getNodeInfo() {
    try {
        var result = [];
        var cursor = db.exec('select HostName,NodeName,GroupName,IsPrimary,Location,IsLocationPrimary from $SNAPSHOT_DB');
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (undefined != current.ErrNodes) {
                for (let i in current.ErrNodes) {
                    result.push({
                        HostName: current.ErrNodes[i].NodeName.split(':')[0],
                        NodeName: current.ErrNodes[i].NodeName,
                        GroupName: current.ErrNodes[i].GroupName,
                        IsPrimary: false,
                        Location: "ErrNodes",
                        IsLocationPrimary: false,
                    });
                }
            } else {
                result.push({
                    HostName: current.HostName,
                    NodeName: current.NodeName,
                    GroupName: current.GroupName,
                    IsPrimary: current.IsPrimary,
                    Location: current.Location,
                    IsLocationPrimary: current.IsLocationPrimary,
                });
            }
        }

        var cursor = db.exec('select HostName,NodeName,GroupName,IsPrimary,Location,IsLocationPrimary from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup"');
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (undefined != current.ErrNodes) {
                for (let i in current.ErrNodes) {
                    result.push({
                        HostName: current.ErrNodes[i].NodeName.split(':')[0],
                        NodeName: current.ErrNodes[i].NodeName,
                        GroupName: current.ErrNodes[i].GroupName,
                        IsPrimary: false,
                        Location: "ErrNodes",
                        IsLocationPrimary: false,
                    });
                }
            } else {
                result.push({
                    HostName: current.HostName,
                    NodeName: current.NodeName,
                    GroupName: current.GroupName,
                    IsPrimary: current.IsPrimary,
                    Location: current.Location,
                    IsLocationPrimary: current.IsLocationPrimary,
                });
            }
        }
    } catch (e) {
        println("Error getting node info from $SNAPSHOT_DB: " + getLastErrObj());
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return result;
}

// 获取当前 GroupMode 信息 todo
function getGroupModeInfo() {
    try {
        var result = [];
        var cursor = db.list(SDB_LIST_GROUPMODES);
        while (cursor.next()) {
            var current = cursor.current().toObj();
            result.push({
                GroupName: current.GroupName,
                GroupMode: current.GroupMode,
                UpdateTime: current.UpdateTime,
                MaxTime: current.MaxTime,
                MinTime: current.MinTime
            });
        }
    } catch (e) {
        println("Error getting group mode info from SDB_LIST_GROUPMODES: " + getLastErrObj());
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return result;
}

// 处理节点信息，构造 {hostname:[nodename],groupname:[nodename],location:[groupname]} 对象并返回
// flag -> 是否排除 coord 信息
function handleNodeInfo(nodeInfo, flag) {
    var result = {};
    for (let i = 0; i < nodeInfo.length; i++) {
        var groupname = nodeInfo[i].GroupName;
        if (flag && groupname == "SYSCoord") {
            continue;
        }
        var location = nodeInfo[i].Location;
        var hostname = nodeInfo[i].HostName;
        var nodename = nodeInfo[i].NodeName;

        if (result[hostname]) {
            result[hostname].push(nodename);
        } else {
            result[hostname] = [nodename];
        }
        if (result[groupname]) {
            result[groupname].push(nodename);
        } else {
            result[groupname] = [nodename];
        }
        if (result[location]) {
        if (-1 == result[location].indexOf(groupname)) {
                result[location].push(groupname);
            }
        } else {
            result[location] = [groupname];
        }
    }
    return result;
}

// 使用 dc.reelectAnlyze() 进行切主
function reelect(reelectLevel) {
    if (undefined == reelectLevel || -1 == [0, 1, 2].indexOf(reelectLevel)) {
        return false;
    }

    var filterLevel = "";
    switch (reelectLevel) {
        case 0:
            filterLevel = "GroupMode";
            break;
        case 1:
            filterLevel = "Location";
            break;
        case 2:
            filterLevel = "Weight";
            break;
        default:
            break;
    }

    println("[INFO] Executing reelect, reelectLevel: " + reelectLevel + " -> " +  filterLevel);
    // 等待一会，避免前置操作未执行完成，导致切主不及预期
    sleep(2000);
    try {
        // 展示预计切换的主节点
        var result = dc.reelectAnalyze({"FilterLevel": filterLevel}, false);
        var detail = result.toObj().Detail;
        var needReelect = false;
        for (var i in detail) {
            needReelect = true
            var OldPrimary = detail[i].OldPrimary;
            var NewPrimary = detail[i].NewPrimary;
            var GroupName = detail[i].GroupName;
            var CausedBy = detail[i].CausedBy;
            println("   Reelect group \"" + GroupName + "\" primary node: \"" + OldPrimary + "\" -> \"" + NewPrimary + "\", caused by: \"" + CausedBy + "\"");
        }
        // 实际切换
        if (needReelect) {
            dc.reelectAnalyze({"FilterLevel": filterLevel}, true);
            println("[INFO] Reelect done");
        } else {
            println("[INFO] Not need reelect");
        }
    } catch (e) {
        println("Error reelect with dc.reelectAnalyze(): " + getLastErrObj());
        throw e;
    }
    println(separator);

    return true;
}

// 获取节点状态
function getNodeHealthInfo() {
    try {
        // 记录每个 Location / HostName 下是否有故障节点
        var nodeHealthInfo = {};
        var errHosts = [];

        // 检查并统计节点状态
        var cursor = db.exec('select NodeName,HostName,Location,ServiceStatus,Status from $SNAPSHOT_DB');
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (undefined != current.ErrNodes) {
                for (let i = 0; i < current.ErrNodes.length; i++) {
                    var nodename = current.ErrNodes[i].NodeName;
                    var hostname = nodename.split(':')[0];
                    if (nodeHealthInfo[hostname]) {
                        nodeHealthInfo[hostname].push({NodeName: nodename, Status: current.ErrNodes[i].Flag, ServiceStatus: current.ErrNodes[i].Flag});
                    } else {
                        nodeHealthInfo[hostname] = [{NodeName: nodename, Status: current.ErrNodes[i].Flag, ServiceStatus: current.ErrNodes[i].Flag}];
                    }
                    // 无法获取 location 信息（需要从 catalog 中获取）
                    errHosts.push(hostname);
                }
            } else {
                if ("Normal" != current.Status || ! current.ServiceStatus) {
                    var nodename = current.NodeName;
                    var hostname = current.HostName;
                    var location = current.Location;
                    if (nodeHealthInfo[hostname]) {
                        nodeHealthInfo[hostname].push({NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus});
                    } else {
                        nodeHealthInfo[hostname] = [{NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus}];
                    }
                    if (null != location && "" != location) {
                        if (nodeHealthInfo[location]) {
                            nodeHealthInfo[location] = {NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus};
                        } else {
                            nodeHealthInfo[location] = {NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus};
                        }
                    }
                }
            }
        }
        cursor.close();

        cursor = db.exec('select NodeName,HostName,Location,ServiceStatus,Status from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup"');
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (undefined != current.ErrNodes) {
                for (let i = 0; i < current.ErrNodes.length; i++) {
                    var nodename = current.ErrNodes[i].NodeName;
                    var hostname = nodename.split(':')[0];
                    if (nodeHealthInfo[hostname]) {
                        nodeHealthInfo[hostname].push({NodeName: nodename, Status: current.ErrNodes[i].Flag, ServiceStatus: current.ErrNodes[i].Flag});
                    } else {
                        nodeHealthInfo[hostname] = [{NodeName: nodename, Status: current.ErrNodes[i].Flag, ServiceStatus: current.ErrNodes[i].Flag}];
                    }
                    // 无法获取 location 信息（需要从 catalog 中获取）
                    errHosts.push(hostname);
                }
            } else {
                if ("Normal" != current.Status || ! current.ServiceStatus) {
                    var nodename = current.NodeName;
                    var hostname = current.HostName;
                    var location = current.Location;
                    if (nodeHealthInfo[hostname]) {
                        nodeHealthInfo[hostname].push({NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus});
                    } else {
                        nodeHealthInfo[hostname] = [{NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus}];
                    }
                    if (null != location && "" != location) {
                        if (nodeHealthInfo[location]) {
                            nodeHealthInfo[location] = {NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus};
                        } else {
                            nodeHealthInfo[location] = {NodeName: nodename, Status: current.Status, ServiceStatus: current.ServiceStatus};
                        }
                    }
                }
            }
        }
        cursor.close();
    } catch (e) {
        println("[ERROR] Failed to get node health info from coord: " + getLastErrObj());
        throw e;
    }

    // 从 catalog 系统表中获取 errHosts location 信息
    try {
        if (0 < errHosts.length) {
            var cata = connCatalog();
            var locationInfo = {};
            cursor = cata.exec('select Group from SYSCAT.SYSNODES split by Group');
            while (cursor.next()) {
                var current = cursor.current().toObj();
                var location = current.Group.Location;
                var hostname = current.Group.HostName;
                if (-1 != errHosts.indexOf(hostname)) {
                    if (locationInfo[location] && -1 == locationInfo[location].indexOf(hostname)) {
                        locationInfo[location].push(hostname);
                    } else {
                        locationInfo[location] = [hostname];
                    }
                }
            }
            cursor.close();
            nodeHealthInfo["_location"] = locationInfo; 
        }
    } catch (e) {
        println("[ERROR] Failed to get node health info from catalog: " + getLastErrObj());
        throw e;
    } finally {
        if (null != cata) {
            cata.close();
        }
    }

    return nodeHealthInfo;
}

// 关闭所有 MaintenanceMode
function stopAllMaintenanceMode() {
    try {
        var modes = listGroupModes();
        var count = 0;

        for (var i = 0; i < modes.length; i++) {
            if (modes[i].GroupMode === "maintenance") {
                count++;
            }
        }

        if (0 < count) {
            dc.stopMaintenanceMode();
        }

        return count;
    } catch (e) {
        println("Error stopping all maintenance modes: " + getLastErrObj());
        throw e;
    }
}

// 关闭所有 CriticalMode
function stopAllCriticalMode() {
    try {
        var modes = listGroupModes();
        var count = 0;

        for (var i = 0; i < modes.length; i++) {
            if (modes[i].GroupMode === "critical") {
                count++;
            }
        }

        if (0 < count) {
            dc.stopCriticalMode();
        }

        return count;
    } catch (e) {
        println("Error stopping all critical modes: " + getLastErrObj());
        throw e;
    }
}

function checkStandAlone() {
    try {
        var isStandAlone = false;
        var cursor = db.exec('select * from $SNAPSHOT_CONFIGS where role = "standalone"');
        while (cursor.next()) {
            isStandAlone = true;
        }
    } catch (e) {
        // 忽略错误，可能是集群状态不正常
        println("[WARNING] Failed to check standealone node for $SNAPSHOT_CONFIGS: " + getLastErrObj());
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }
    return isStandAlone;
}

function checkCataLogPrimary() {
    try {
        var existPrimary = false;
        var cursor = db.exec('select * from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup"');
        // 记录存活的编目节点数量是否满足多数派，防止因为存在的强制升主的编目节点而误判
        var errorNodes = 0;
        var normalNodes = 0;
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (null != current.ErrNodes){
                errorNodes += current.ErrNodes.length;
                continue;
            }
            normalNodes++;
        }
    } catch (e) {
        println("Error checking catalog group primary node: " + getLastErrObj());
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }

    if (normalNodes > errorNodes) {
        existPrimary = true;
    }

    return existPrimary;
}

// 检查集群健康状态，要求所有节点正常
function checkClusterHealth() {
    try {
        var hasError = false;
        var cursor = db.snapshot(SDB_SNAP_HEALTH,{},{NodeName:"",Status:"",ServiceStatus:"",DataStatus:""});
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (undefined != current.ErrNodes) {
                for (let i in current.ErrNodes) {
                    println('[ERROR] Node "' + current.ErrNodes[i].NodeName + '" error: ' + current.ErrNodes[i].Flag);
                }
                hasError = true;
            } else if ("Normal" != current.Status || ! current.ServiceStatus || "Normal" != current.DataStatus) {
                println('[ERROR] Node "' + current.NodeName + '" error, Status: ' + current.Status + " ServiceStatus: " + current.ServiceStatus + " DataStatus: " + current.DataStatus);
                hasError = true;
            }
        }
        cursor.close();
    } catch (e) {
        println("Error checking cluster health: " + getLastErrObj());
        throw e;
    }
    return !hasError;
}

// 检查 location 是否存在当前集群中
function checkLocationExist(location, nodeInfo) {
    if (null == location || "" == location) {
        return false;
    }

    var isFind = false;
    for (let i in nodeInfo) {
        if (location == nodeInfo[i].Location) {
            isFind = true;
            break;
        }
    }
    return isFind;
}

// 检查 host 是否存在当前集群中
function checkHostExist(host, nodeInfo) {
    if (null == host || "" == host) {
        return false;
    }

    var isFind = false;
    for (let i in nodeInfo) {
        if (host == nodeInfo[i].HostName) {
            isFind = true;
            break;
        }
    }
    return isFind;
}

// 读取节点信息文件
function readNodeFileOnInit(filePath) {
    if (!filePath) {
        throw new Error("[ERROR] File can not be empty found: " + filePath);
    }

    try {
        if (!File.exist(filePath)) {    
            println("[ERROR] File not found: " + filePath);
            throw new Error("[ERROR] File not found: " + filePath);
        }
        var file = new File(filePath, 0644);
    } catch (e) {
        println("[ERROR] Failed to open file \""+ filePath +"\"");
        throw e;
    }


    var info = {};
    var currentLocation = "";
    var currentDomain = "";
    var content = "";
    var index = "";
    var hostObj = {};
    var curActiveLocation = "";
    var flag = false;
    
    try {
        while (content = file.readLine()) {
            var line = content.trim();
            if (line === "") continue;
            if (0 == line.indexOf("[domains") && line[line.length - 1] == "]") {
                // 读取 domains 信息
                if (line == "[domains(1)]") {
                    info.domainMode = 1;
                } else if (line == "[domains(0)]") {
                    info.domainMode = 0;
                } else {
                    // 非 0 | 1, 异常
                    println("\n[ERROR] The domainMode in file \"" + filePath + "\" can only be [domains(0)] or [domains(1)]");
                    throw new Error("[ERROR] The domainMode in file \"" + filePath + "\" can only be [domains(0)] or [domains(1)]");
                }
                currentDomain = "domains";
                currentLocation = "";
            } else if (line[0] == "[" && line[line.length - 1] == "]") {
                line = line.substring(1, line.length - 1);
                index = line.indexOf("(active)");
                if (-1 != index) {
                    line = line.substring(0, index);
                    if ("" == curActiveLocation) {
                        curActiveLocation = line;
                        activeLocation = line;
                    } else {
                        // 存在多个 activeLocation，文件有误
                        println("\n[ERROR] ActiveLocation duplicate in file: " + filePath);
                        throw new Error("[ERROR] ActiveLocation duplicate in file: " + filePath);
                    }
                }
                currentLocation = line;
                currentDomain = "";
                if (null == info[line]) {
                    info[line] = [];
                }
            } else if ("" != currentDomain) {
                if (null == info["domains"]) {
                    info["domains"] = [line];
                } else {
                    info["domains"].push(line);
                }
            } else if ("" != currentLocation) {
                // 去掉末尾的状态
                index = line.indexOf("(Critical)");
                index = index == -1 ? line.indexOf("(PartialCritical)") : index;
                index = index == -1 ? line.indexOf("(Maintenance)") : index;
                index = index == -1 ? line.indexOf("(PartialMaintenance)") : index;
                index = index == -1 ? line.indexOf("(Critical-Maintenance)") : index;
                index = index == -1 ? line.indexOf("(Partial-Critical-Maintenance)") : index;
                if (-1 != index) {
                    line = line.substring(0, index);
                }
                if (null == hostObj[line]) {
                    info[currentLocation].push(line);
                    hostObj[line] = 1;
                    flag = true;
                } else {
                    println("\n[ERROR] Host " + line + " duplicate in file: " + filePath);
                    throw new Error("[ERROR] Host " + line + " duplicate in file: " + filePath);
                }
            }
        }
    } catch (e) {
        if (SDB_EOF != e) {
            println("[ERROR] Failed to read file \""+ filePath +"\", error: " + getLastErrObj());
            throw e;
        }
    } finally {
        file.close();
    }

    if ("" == curActiveLocation) {
        // 文件中未设置 activeLocation
        activeLocation = "";
    }

    if (flag) {
        return info;
    } else {
        // 文件没有有效内容
        return null;
    }
}

// 读取节点信息文件
function readNodeFileOnMode(filePath) {
    if (!filePath) {
        throw new Error("[ERROR] File can not be empty found");
    }

    try {
        if (!File.exist(filePath)) {
            println("[ERROR] File not found: " + filePath);
            throw new Error("[ERROR] File not found: " + filePath);
        }
        var file = new File(filePath, 0644);
    } catch (e) {
        println("[ERROR] Failed to open file \""+ filePath +"\"");
        throw e;
    }

    var info = {
        locations: [],
        hostnames: [],
        nodenames: [],
        domains: [],
        count: 0
    };

    var currentSection = "";
    var content = "";
    var flag = false;

    try {
        while (content = file.readLine()) {
            var line = content.trim();
            if (line === "") continue;

            if (/\[location\]/i.test(line)) {
                currentSection = "locations";
            } else if (/\[hostname\]/i.test(line)) {
                currentSection = "hostnames";
            } else if (/\[domain\]/i.test(line)) {
                currentSection = "domains";
            } else if (currentSection) {
                if (currentSection === "locations") {
                    if (-1 != info.locations.indexOf(line)) {
                        println("[ERROR] Location " + line + " duplicate in file");
                        throw new Error("[ERROR] Location " + line + " duplicate in file");
                    }
                    info.locations.push(line);
                } else if (currentSection === "hostnames") {
                    if (-1 != info.hostnames.indexOf(line)) {
                        println("[ERROR] Host " + line + " duplicate in file");
                        throw new Error("[ERROR] Host " + line + " duplicate in file");
                    }
                    info.hostnames.push(line);
                } else if (currentSection === "domains") {
                    if (-1 != info.domains.indexOf(line)) {
                        println("[ERROR] Domain " + line + " duplicate in file");
                        throw new Error("[ERROR] Domain " + line + " duplicate in file");
                    }
                    info.domains.push(line);
                }
                info.count++;
                flag = true;
            }
        }
    } catch (e) {
        if (SDB_EOF != e) {
            println("[ERROR] Failed to read file \""+ filePath +"\"");
            throw e;
        }
    } finally {
        file.close();
    }

    if (flag) {
        return info;
    } else {
        // 文件没有有效内容（location/hostname）
        return null;
    }
}

// 检查 location 是否开启了 MaintenanceMode
function checkLocationInMaintenance(checkLocation, groupModeInfo, nodeInfo) {
    for (let i in groupModeInfo) {
        var properties = groupModeInfo[i].Properties;
        if (0 == properties.length || "maintenance" != groupModeInfo[i].GroupMode) {
            continue;
        }
        for (let j in properties) {
            var nodeName = properties[j].NodeName;
            var location = properties[j].Location;
            if (null != nodeName) {
                var hostName = nodeName.split(':')[0];
                for (let k in nodeInfo) {
                    if (hostName == nodeInfo[k].HostName && checkLocation == nodeInfo[k].Location) {
                        return true;
                    }
                }
            } else if (location == checkLocation){
                return true;
            }
        }
    }
    return false;
}

// 检查 host 是否开启了 MaintenanceMode
function checkHostInMaintenance(checkHost, groupModeInfo, nodeInfo) {
    for (let i in groupModeInfo) {
        var properties = groupModeInfo[i].Properties;
        if (0 == properties.length || "maintenance" != groupModeInfo[i].GroupMode) {
            continue;
        }
        for (let j in properties) {
            var nodeName = properties[j].NodeName;
            var location = properties[j].Location;
            if (null != nodeName) {
                var hostName = nodeName.split(':')[0];
                if (hostName == checkHost) {
                    return true;
                }
            } else {
                for (let k in nodeInfo) {
                    if (location == nodeInfo[k].Location && checkHost == nodeInfo[k].HostName) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// 检查 location 是否开启了 CriticalMode
function checkLocationInCritical(checkLocation, groupModeInfo, nodeInfo) {
    for (let i in groupModeInfo) {
        var properties = groupModeInfo[i].Properties;
        if (0 == properties.length || "critical" != groupModeInfo[i].GroupMode) {
            continue;
        }
        for (let j in properties) {
            var nodeName = properties[j].NodeName;
            var location = properties[j].Location;
            if (null != nodeName) {
                // var hostName = nodeName.split(':')[0];
                // for (let k in nodeInfo) {
                //     if (hostName == nodeInfo[k].HostName && checkLocation == nodeInfo[k].Location) {
                //         return true;
                //     }
                // }
                // 针对 hostname 开启的 critical，指定这个 host 所在的 location 为目标无法关闭 critical，所以不能返回 true
                continue ;
            } else if (location == checkLocation){
                return true;
            }
        }
    }
    return false;
}

// 检查 host 是否开启了 CriticalMode
function checkHostInCritical(checkHost, groupModeInfo, nodeInfo) {
    for (let i in groupModeInfo) {
        var properties = groupModeInfo[i].Properties;
        if (0 == properties.length || "critical" != groupModeInfo[i].GroupMode) {
            continue;
        }
        for (let j in properties) {
            var nodeName = properties[j].NodeName;
            var location = properties[j].Location;
            if (null != nodeName) {
                var hostName = nodeName.split(':')[0];
                if (hostName == checkHost) {
                    return true;
                }
            } else {
                for (let k in nodeInfo) {
                    if (location == nodeInfo[k].Location && checkHost == nodeInfo[k].HostName) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// 主函数
function main() {
    try {
        println(separator);
        println("SequoiaDB Location Disaster Recovery Tool");
        println(separator);

        // 先验证所有参数
        validateParameters();

        // 获取 sdb 连接
        try {
            connectToSdb();
        } catch (e) {
            if (-104 == e) {
                // 除 init 和 restore 操作外，确保 catalog 编目组可用
                if ('init' == mode || 'restore' == mode) {
                    println("[ERROR] SYSCatalogGroup does not have primary node, cannot " + mode);
                    throw new Error("SYSCatalogGroup does not have primary node, cannot " + mode);
                } else {
                    // 编目已经故障，且未设置 auth = false
                    print('[INFO] Try to force set up catalog node ... ');
                    forceSetupCataLog(maxKeepTime, true);
                    println('ok');
                    try {
                        connectToSdb();
                    } catch (e) {
                        throw e;
                    }
                }
            } else {
                throw e;
            }
        }

        // 除 init 和 restore 操作外，确保 catalog 编目组可用
        if ('init' == mode || 'restore' == mode) {
            if (!checkCataLogPrimary()) {
                println("[ERROR] SYSCatalogGroup does not have primary node, cannot " + mode);
                throw new Error("SYSCatalogGroup does not have primary node, cannot " + mode);
            }
        }

        // 检查是否为 standalone 节点，不支持
        if (checkStandAlone()) {
            println("[ERROR] Current tool cannot run on the standalone node");
            throw new Error("Current tool cannot run on the standalone node");
        }

        // 获取 dc
        getDc();

        switch (mode) {
            case 'show':
                executeShow();
                break;
            case 'check':
                executeCheck();
                break;
            case 'init':
                executeInit();
                reelect(reelectLevel);
                break;
            case 'start_maintenance':
                executeStartMaintenance();
                reelect(reelectLevel);
                break;
            case 'stop_maintenance':
                executeStopMaintenance();
                reelect(reelectLevel);
                break;
            case 'start_critical':
                executeStartCritical();
                reelect(reelectLevel);
                break;
            case 'stop_critical':
                executeStopCritical();
                reelect(reelectLevel);
                break;
            case 'restore':
                executeRestore();
                reelect(reelectLevel);
                break;
        }
    } catch (e) {
        println("Error" + "              " + getLastErrObj());
        if (e.stack) {
            println("Stack trace:");
            var stackLines = e.stack.split('\n');
            for (var i = 0; i < stackLines.length; i++) {
                println("  " + stackLines[i]);
            }
        }
        throw e;
    } finally {
        // 统一在函数结束时释放连接
        disconnectFromSdb();
    }
}


// 验证所有参数
function validateParameters() {
    print("[INFO] Validating parameters ... ");
    var errors = [];

    try {
        // 检查必需的参数
        switch (mode) {
            case 'show':
            case 'restore':
                // 这些命令不需要 file 参数
                break;
            case 'check':
            case 'init':
                // 如果提供了 locationFile 参数，验证文件存在并读取文件内容
                if (typeof locationFile !== 'undefined' && locationFile) {
                    initLocationObject = readNodeFileOnInit(locationFile);
                    if (!initLocationObject || typeof initLocationObject != 'object') {
                        errors.push("[ERROR] Location information file '" + locationFile + "' not found or invalid");
                    }
                }
                break;
            case 'start_maintenance':
            case 'stop_maintenance':
            case 'start_critical':
            case 'stop_critical':
                // 如果提供了 targetFile 参数，验证文件存在并读取文件内容
                if (typeof targetFile !== 'undefined' && targetFile) {
                    nodeTargetInfo = readNodeFileOnMode(targetFile);
                    if (!nodeTargetInfo || typeof nodeTargetInfo != 'object') {
                        errors.push("[ERROR] Node information file '" + targetFile + "' not found or invalid");
                    } else {
                        if (nodeTargetInfo.locations && nodeTargetInfo.hostnames) {
                            if (nodeTargetInfo.locations.length != 0 && nodeTargetInfo.hostnames.length != 0) {
                                errors.push("[ERROR] [location] and [hostname] cannot be used at the same time in the file \"" + targetFile + "\"");
                            }
                            if (nodeTargetInfo.domains && nodeTargetInfo.locations.length == 0 && nodeTargetInfo.hostnames.length == 0 && nodeTargetInfo.domains.length != 0) {
                                errors.push("[ERROR] [domain] cannot be used without [location] or [hostname] in the file \"" + targetFile + "\"");
                            }
                        }
                    }
                }
                break;
            default:
                println("Please provide a command using: sdb -f bin/main.js -e \"var mode='command'\"");
                throw new Error("[ERROR] No command specified");
        }
    } catch (e) {
        throw e;
    }

    // 检查各参数
    if (typeof sdbCoord === 'undefined' || typeof sdbCoord !== 'string') {
        errors.push("Parameter sdbCoord must be string");
    }

    if (typeof sdbUser === 'undefined' || typeof sdbUser !== 'string') {
        errors.push("Parameter sdbUser must be string");
    }

    if (typeof sdbPassword === 'undefined' || typeof sdbPassword !== 'string') {
        errors.push("Parameter sdbPassword must be string");
    }

    if (typeof sdbToken === 'undefined' || typeof sdbToken !== 'string') {
        errors.push("Parameter sdbToken must be string");
    }

    if (typeof sdbCipherFile === 'undefined' || typeof sdbCipherFile !== 'string') {
        errors.push("Parameter sdbCipherFile must be string");
    }

    if ('' != sdbCipherFile && !File.exist(sdbCipherFile)) {
        errors.push("Parameter sdbCipherFile '" + sdbCipherFile + "' does not exist");
    }

    if (typeof sdbCm === 'undefined' || typeof sdbCm !== 'number') {
        errors.push("Parameter sdbCm must be number");
    }

    // 检查并提取里面的 domains 和 domainMode
    if (null == initLocationObject || typeof initLocationObject === 'undefined' || typeof initLocationObject !== 'object') {
        errors.push("Parameter initLocationObject must be object");
    } else {
        if (Array.isArray(initLocationObject.domains)) {
            domainList = initLocationObject.domains;
        } else if (undefined !== initLocationObject.domains) {
            errors.push("Parameter initLocationObject.domains must be array");
        }
        // domainMode 仅接受 0 | 1；未配置时保持默认值 1
        if (undefined !== initLocationObject.domainMode) {
            if (0 === initLocationObject.domainMode || 1 === initLocationObject.domainMode) {
                domainMode = initLocationObject.domainMode;
            } else {
                errors.push("Parameter initLocationObject.domainMode must be in [0, 1]");
            }
        }
        // domains 不为空时，domainMode 必须为 0 | 1（默认已是 1）
        if (0 < domainList.length && 0 != domainMode && 1 != domainMode) {
            errors.push("Parameter initLocationObject.domainMode must be in [0, 1] when initLocationObject.domains are not empty");
        }

        if (0 < domainList.length) {
            delete initLocationObject.domains;
            delete initLocationObject.domainMode;

            // 必须包含其他 location 配置
            let keys = Object.keys(initLocationObject);
            if (0 == keys.length) {
                errors.push("Parameter initLocationObject must be included location configure");
            }
        }
    }

    if (typeof activeLocation === 'undefined' || typeof activeLocation !== 'string') {
        errors.push("Parameter activeLocation must be string");
    }

    if (typeof minKeepTime === 'undefined' || typeof minKeepTime !== 'number') {
        errors.push("Parameter minKeepTime must be number");
    }

    if (minKeepTime < 1 || minKeepTime > 10080) {
        errors.push("Parameter minKeepTime must be in [1, 10080]");
    }

    if (typeof maxKeepTime === 'undefined' || typeof maxKeepTime !== 'number') {
        errors.push("Parameter maxKeepTime must be number");
    }

    if (maxKeepTime < 1 || maxKeepTime > 10080) {
        errors.push("Parameter maxKeepTime must be in [1, 10080]");
    }

    if (maxKeepTime <= minKeepTime) {
        errors.push("Parameter maxKeepTime must be greater than minKeepTime");
    }

    if (typeof enforce === 'undefined' || typeof enforce !== 'boolean') {
        errors.push("Parameter enforce must be boolean");
    }

    // 检查 stop_maintenance 和 stop_critical 的 --check 选项
    if (typeof check !== 'undefined' && typeof check !== 'boolean') {
        errors.push("Parameter check must be boolean");
    }

    // 不允许同时指定 -l,--location 和 -H,--hostname 参数
    if (typeof locations !== 'undefined' && typeof hostnames !== 'undefined') {
        errors.push("Parameter \"-l,--location\" and \"-H,--hostname\" cannot be used at the same time");
    }

    // 检查 show 的 quiet 参数
    if (typeof quiet === 'undefined') {
        quiet = false;
    } else if (typeof quiet !== 'boolean') {
        errors.push("Parameter \"-q,--quiet\" must be boolean");
    }

    if (errors.length > 0) {
        println("Parameter validation errors:");
        for (var i = 0; i < errors.length; i++) {
            println("  - " + errors[i]);
        }
        throw new Error("Invalid parameters");
    }

    println("ok");
}

function outputLocationToFile(locationInfoObj, locationFile) {
    try {
        if (File.exist(locationFile)) {
            File.remove(locationFile);
        }
        var file = new File(locationFile, 0644);
    } catch (e) {
        println("[ERROR] Failed to open file " + locationFile);
        throw e;
    }

    try {
        for (let i in locationInfoObj) {
            file.write("[" + i + "]\n");
            for (let j in locationInfoObj[i]) {
                file.write(locationInfoObj[i][j] + "\n");
            }
            file.write("\n");
        }
        file.close();
    } catch (e) {
        println("[ERROR] Failed to write file " + locationFile);
        throw e;
    }
}

// 检查 domain 是否在 listDomains 中出现
function checkDomain(checkDomainList, domainInfo) {
    for (let i in checkDomainList) {
        let isFind = false;
        for (let j in domainInfo) {
            if (checkDomainList[i] == domainInfo[j].Name) {
                isFind = true;
            }
        }
        if (!isFind) {
            // 未找到目标 domain，异常
            println("[ERROR] Cannot find domain \"" + checkDomainList[i] + "\" in db.listDomains()");
            throw new Error("[ERROR] Cannot find domain \"" + checkDomainList[i] + "\" in db.listDomains()");
        }
    }
}

// 排除模式处理 domains
function excludeDomain(excludeDomainList, domainInfo) {
    var result = [];
    // 排除模式选择其他 domain
    for (let i in domainInfo) {
        let domain = domainInfo[i].Name;
        let isFind = false;
        for (let j in excludeDomainList) {
            let checkDomain = excludeDomainList[j];
            if (domain == checkDomain) {
                isFind = true;
                break;
            }
        }
        if (!isFind) {
            result.push(domain);
        }
    }
    return result;
}

// 按 domain 过滤 show 的信息
function filterShowInfoWithDomain(outputArray, nodeInfo, domainList, domainInfo) {
    // 根据域统计涉及的组
    var groupArray = [];
    for (let i in domainList) {
        for (let j in domainInfo) {
            if (domainList[i] == domainInfo[j].Name) {
                let groups = domainInfo[j].Groups;
                if (!Array.isArray(groups)) {
                    continue;
                }
                for (let k in groups) {
                    if (-1 == groupArray.indexOf(groups[k].GroupName)) {
                        groupArray.push(groups[k].GroupName);
                    }
                }
            }
        }
    }

    // 根据组统计涉及的节点
    var nodeArray = [];
    for (let i in groupArray) {
        for (let j in nodeInfo) {
            let group = nodeInfo[j].GroupName;
            let node = nodeInfo[j].NodeName;
            if (-1 != nodeArray.indexOf(node)) {
                continue;
            }

            if (group == groupArray[i]) {
                nodeArray.push(node);
                continue;
            }
        }
    }

    // 根据组统计涉及的机器
    var hostArray = [];
    for (let i in groupArray) {
        for (let j in nodeInfo) {
            let group = nodeInfo[j].GroupName;
            let host = nodeInfo[j].HostName;
            if (-1 != hostArray.indexOf(host)) {
                continue;
            }

            if (group == groupArray[i]) {
                hostArray.push(host);
                continue;
            }
        }
    }

    // 根据组统计涉及的 location
    var locationArray = [];
    for (let i in groupArray) {
        for (let j in nodeInfo) {
            let group = nodeInfo[j].GroupName;
            let location = nodeInfo[j].Location;
            if (-1 != locationArray.indexOf(location)) {
                continue;
            }

            if (group == groupArray[i]) {
                locationArray.push(location);
                continue;
            }
        }
    }

    // 根据机器列表，过滤打印表格
    for (let i = 0; i < outputArray.length; i++) {
        let title = outputArray[i].title;
        let head = outputArray[i].head;
        let content = outputArray[i].content;
        if ("[Location Layout Info]" == title) {
            let array = [];
            for (let j = 0; j < head[0].length; j++) {
                array[j] = [];
            }
            for (let j in content) {
                for (let k = 0; k < content[j].length; k++) {
                    // 去掉末尾的状态
                    let target = content[j][k];
                    let index = target.indexOf("(Critical)");
                    index = index == -1 ? target.indexOf("(PartialCritical)") : index;
                    index = index == -1 ? target.indexOf("(Maintenance)") : index;
                    index = index == -1 ? target.indexOf("(PartialMaintenance)") : index;
                    index = index == -1 ? target.indexOf("(Critical-Maintenance)") : index;
                    index = index == -1 ? target.indexOf("(Partial-Critical-Maintenance)") : index;
                    if (-1 != index) {
                        target = content[j][k].substring(0, index);
                    }
                    if (-1 != hostArray.indexOf(target)) {
                        array[k].push(content[j][k]);
                    }
                }
            }
            content.splice(0, content.length);
            for (let j in array) {
                for (let k in array[j]) {
                    if (null == content[k]) {
                        content[k] = [array[j][k]];
                    } else {
                        content[k].push(array[j][k]); 
                    }
                }
            }
        } else if ("[GroupMode Runtime Info]" == title) {
            let indexArray = [];
            for (let j in content) {
                let target = content[j][0];
                if (0 == target.indexOf("Node: ")) {
                    let sub_target = target.substring(6, target.length);
                    if (-1 == nodeArray.indexOf(sub_target)) {
                        indexArray.push(j);
                    }
                } else if (0 == target.indexOf("Host: ")) {
                    let sub_target = target.substring(6, target.length);
                    if (-1 == hostArray.indexOf(sub_target)) {
                        indexArray.push(j);
                    }
                } else if (0 == target.indexOf("Location: ")) {
                    let sub_target = target.substring(10, target.length);
                    if (-1 == locationArray.indexOf(sub_target)) {
                        indexArray.push(j);
                    }
                } else {
                    throw new Error("[ERROR] Unknown target from outputArray: " + target);
                }
            }
            for (let j = indexArray.length - 1; j >= 0; j--) {
                content.splice(indexArray[j], 1);
            }
            if (0 == content.length) {
                outputArray.splice(i, 1);
                i--;
            }
        } else if ("[Abnormal: Mixed Location Info]" == title) {
            let indexArray = [];
            for (let j in content) {
                if (-1 == nodeArray.indexOf(content[j][0])) {
                    indexArray.push(j);
                }
            }
            for (let j = indexArray.length - 1; j >= 0; j--) {
                content.splice(indexArray[j], 1);
            }
            if (0 == content.length) {
                outputArray.splice(i, 1);
                i--;
            }
        } else if ("[Abnormal: Primary Node Not In ActiveLocation]" == title) {
            let indexArray = [];
            for (let j in content) {
                if (-1 == nodeArray.indexOf(content[j][0])) {
                    indexArray.push(j);
                }
            }
            for (let j = indexArray.length - 1; j >= 0; j--) {
                content.splice(indexArray[j], 1);
            }
            if (0 == content.length) {
                outputArray.splice(i, 1);
                i--;
            }
        } else if ("[Abnormal: Node Not In GroupMode]" == title) {
            let indexArray = [];
            for (let j in content) {
                if (-1 == hostArray.indexOf(content[j][0])) {
                    indexArray.push(j);
                }
            }
            for (let j = indexArray.length - 1; j >= 0; j--) {
                content.splice(indexArray[j], 1);
            }
            if (0 == content.length) {
                outputArray.splice(i, 1);
                i--;
            }
        } else if ("[Abnormal: Group Not In GroupMode]" == title) {
            let array = content[0][1].split(',');
            for (let j = array.length - 1; j >=0; j--) {
                if (-1 == groupArray.indexOf(array[j])) {
                    array.splice(j, 1);
                }
            }
            if (0 < array.length) {
                outputArray[i].content[0][1] = "" + array;
            } else {
                outputArray.splice(i, 1);
                i--;
            }
        } else if ("[Abnormal: Empty Location Host]" == title) {
            if ("ClusterLocation" == head[0][0]) {
                let array = content[0][1].split(',');
                for (let j = array.length - 1; j >=0; j--) {
                    if (-1 == hostArray.indexOf(array[j])) {
                        array.splice(j, 1);
                    }
                }
                if (0 < array.length) {
                    outputArray[i].content[0][1] = "" + array;
                } else {
                    outputArray.splice(i, 1);
                    i--;
                }
            }
        } else {
            throw new Error("[ERROR] Unknown title from outputArray: " + title);
        }
    }
    return outputArray;
}

// 处理 show 信息成可打印的数组
function handleShowInfoToOutputArray(locationAnalyzeInfo, groupModeInfo, reelectAnalyzeInfo, nodeInfo) {
    var outputArray = [];
    if (null == locationAnalyzeInfo) {
        return null;
    }

    // 处理主机与节点关系，后面用于判断主机上所有节点是否都处于某种状态
    var hostNodeObj = handleNodeInfo(nodeInfo, true);

    // ------------- 处理 Location Info 表格 -------------
    var locationInfosheet = {"title": "[Location Layout Info]", "head": [[]], "content": []};
    var activeLocation = locationAnalyzeInfo.ActiveLocation;
    var locationHostObj = {};
    var nodeLocationObj = {};
    var abnormalHostLocationObj = {};

    // 根据 dc.locationAnalyze() 处理 hostname 和 location 信息
    for (let i = 0; i < locationAnalyzeInfo.LocationInfo.length; i++) {
        var hostArray = [];
        var locationInfo = locationAnalyzeInfo.LocationInfo[i];
        var location = locationInfo.LocationName;
        var groupStatus = locationInfo.GroupStatus;
        var isActive = false;

        if (location == activeLocation) {
            isActive = true;
        }

        if (Array.isArray(locationInfo.PartialHost)) {
            // 存在 PartialHost，填入的 hostname，后续在异常信息表格中处理不一致的节点
            for (let j = 0; j < locationInfo.PartialHost.length; j++) {
                var hostname = locationInfo.PartialHost[j].HostName;
                var node = locationInfo.PartialHost[j].Node;
                // 记录异常 hostname 在当前 location 中的节点占比，记录 location 占比更多的作为主要 location
                if (abnormalHostLocationObj[hostname]) {
                    abnormalHostLocationObj[hostname].push({
                        "Location":location,
                        "Node":node,
                        "GroupStatus":groupStatus,
                        "IsActive":isActive
                    });
                } else {
                    abnormalHostLocationObj[hostname] = [{
                        "Location":location,
                        "Node":node,
                        "GroupStatus":groupStatus,
                        "IsActive":isActive
                    }];
                }

                for (let k = 0; k < node.length; k++) {
                    nodeLocationObj[node[k]] = location;
                }
            }
        }

        // 填入 WholeHost 信息
        for (let j = 0; j < locationInfo.WholeHost.length; j++) {
            var hostname = locationInfo.WholeHost[j];
            if ('' != groupStatus) {
                hostArray.push(hostname + '(' + groupStatus + ')');
            } else {
                hostArray.push(hostname);
            }
            nodeLocationObj[hostname] = locationInfo.LocationName;
        }
        if (0 < hostArray.length) {
            if (isActive) {
                location = location + "(active)";
            }
            locationHostObj[location] = hostArray;
        }
    }

    // 按照host数量排序
    var sortArray = [];
    for (let i in locationHostObj) {
        sortArray.push({location: i, len: locationHostObj[i].length});
    }
    sortArray.sort(function(a,b) {return b.len - a.len});

    // 把结果转换为打印数组
    var keys = Object.keys(locationHostObj);
    var locationInfoObj = {};
    for (let j in sortArray) {
        for (let i = 0; i < keys.length; i++) {
            var location = keys[i];
            if (location != sortArray[j].location) {
                continue;
            }
            var hostArray = locationHostObj[location];
            locationInfosheet.head[0].push(location);
            locationInfoObj[location] = [];
            for (let j = 0; j < hostArray.length; j++) {
                if (null == locationInfosheet.content[j]) {
                    locationInfosheet.content[j] = [];
                }
                locationInfosheet.content[j].push(hostArray[j]);
                locationInfoObj[location].push(hostArray[j]);
            }
        }
    }


    // 处理同一 hostname 多个 location
    var keys = Object.keys(abnormalHostLocationObj);
    for (let i = 0; i < keys.length; i++) {
        var hostname = keys[i];
        var hostLocationArray = abnormalHostLocationObj[hostname];
        var maxLength = 0;
        var index = 0;
        for (let j = 0; j < hostLocationArray.length; j++) {
            // 记录节点最多的 location 为主要 location
            if (hostLocationArray[j].Node.length > maxLength) {
                maxLength = hostLocationArray[j].Node.length;
                index = j;
            }
        }

        var location = hostLocationArray[index].Location;
        var groupStatus = hostLocationArray[index].GroupStatus;
        if (hostLocationArray[index].IsActive) {
            location = location + "(active)";
        }
        index = locationInfosheet.head[0].indexOf(location);
        if (-1 == index) {
            // 新增一列 location
            if ('Maintenance' == groupStatus) {
                locationInfosheet.content[index].push(hostname + '(Maintenance)');
            } else if ('Critical' == groupStatus) {
                locationInfosheet.content[index].push(hostname + '(Critical)');
            } else if (null == locationInfosheet.content[index]) {
                locationInfosheet.content[index] = [hostname];
            } else {
                locationInfosheet.content[index].push(hostname);
            }
        } else {
            // 追加 hostname
            if ('Maintenance' == groupStatus) {
                locationInfosheet.content.push(createArrayByIndex(index, hostname + '(Maintenance)'));
            } else if ('Critical' == groupStatus) {
                locationInfosheet.content.push(createArrayByIndex(index, hostname + '(Critical)'));
            } else {
                locationInfosheet.content.push(createArrayByIndex(index, hostname));
            }
        }
    }
    
    if (0 < locationInfosheet.content.length) {
        // 输出到文件
        outputArray.push(locationInfosheet);
        try {
            outputLocationToFile(locationInfoObj, defaultLocationFile);
        } catch (e) {
            throw e;
        }
    } else {
        // 无 Location 布局信息时仍继续输出 GroupMode / 异常表，避免直接 return 丢掉运行中的 GroupMode
        outputArray.push({"title": "[Abnormal: Empty Location Host]", "head": [["Empty Location"]], "content": [["All Host"]]});
    }

    // ------------- 处理 Mode Info 表格 -------------
    var modeInfosheet = {"title": "[GroupMode Runtime Info]", "head": [["Target", "GroupMode","StartTime","Duration","RemainingTime","MaxEndTime"]], "content": []};
    var nodeModeInfo = [];
    var curHostNodeObj = {};
    var locationMode = [];
    // 获取节点、groupMode、最小时间、最大时间和启动时间，然后根据信息算出持续时间，剩余时间和最晚结束时间
    for (let i = 0; i < groupModeInfo.length; i++){
        // 需要计算一台机器上所有节点是否都处于 groupMode，如果是则合并展示
        var groupMode = groupModeInfo[i].GroupMode;
        var properties = groupModeInfo[i].Properties;
        var currentTime = getCurrentTimestamp();
        for (let j = 0; j < properties.length; j++){
            // 可能是 nodename 或者 location
            var nodeName = properties[j].NodeName;
            var location = properties[j].Location;
            var minKeepTime = properties[j].MinKeepTime;
            var maxKeepTime = properties[j].MaxKeepTime;
            var startTime = properties[j].UpdateTime;
            var duration = Math.floor(getTimeDiff(currentTime, startTime)/60);
            var remainingTime = Math.floor(getTimeDiff(minKeepTime, currentTime)/60);
            if (0 > remainingTime) {
                // 超过了最小持续时间，按最大持续时间计算
                remainingTime = Math.floor(getTimeDiff(maxKeepTime, currentTime)/60);
            }
            if (null != nodeName) {
                var hostName = nodeName.split(':')[0];
                // 记录 node 信息，后面统计是否合并为 host 展示
                nodeModeInfo.push({"HostName": hostName, "NodeName": nodeName, "GroupMode": groupMode, "StartTime": startTime, "Duration": duration, "RemainingTime": remainingTime, "MaxKeepTime": maxKeepTime});
                if (curHostNodeObj[hostName]) {
                    curHostNodeObj[hostName]++;
                } else {
                    curHostNodeObj[hostName] = 1;
                }
            } else {
                // 记录 location
                if (-1 == locationMode.indexOf(location)) {
                    locationMode.push(location);
                    modeInfosheet.content.push(["Location: " + location, groupMode, startTime, "" + duration, "" + remainingTime, maxKeepTime]);
                }
            }
        }
    }

    var mergeHost = [];
    for (let i = 0; i < nodeModeInfo.length; i++){
        var hostName = nodeModeInfo[i].HostName;

        if (-1 != mergeHost.indexOf(hostName)) {
            // 已合并 host 填入
            continue;
        }

        if (curHostNodeObj[hostName] >= Math.round(hostNodeObj[hostName].length / 2)) {
            // 一台机器上超过一半的节点处于 groupmode，合并为 hostname 填入
            mergeHost.push(hostName);
            modeInfosheet.content.push(["Host: " + hostName, nodeModeInfo[i].GroupMode, nodeModeInfo[i].StartTime, "" + nodeModeInfo[i].Duration, "" + nodeModeInfo[i].RemainingTime, nodeModeInfo[i].MaxKeepTime]);
        } else {
            // 单独填入
            modeInfosheet.content.push(["Node: " + nodeModeInfo[i].NodeName, nodeModeInfo[i].GroupMode, nodeModeInfo[i].StartTime, "" + nodeModeInfo[i].Duration, "" + nodeModeInfo[i].RemainingTime, nodeModeInfo[i].MaxKeepTime]);
        }
    }
    if (0 < modeInfosheet.content.length) {
        outputArray.push(modeInfosheet);
    }

    // ------------- 处理 Abnormal Info 表格 -------------
    // 每个主机异常的 location（节点 location 与主机大多数派不同）
    var abnormalInfosheet = {"title": "[Abnormal: Mixed Location Info]", "head": [["NodeName", "NodeLocation", "HostMajorLocation"]], "content": []};
    var keys = Object.keys(abnormalHostLocationObj);
    for (let i = 0; i < keys.length; i++) {
        // 找出少数派作为异常信息填入
        var hostname = keys[i];
        var majorLocation = "";
        var hostLocationArray = abnormalHostLocationObj[hostname];
        var maxLength = 0;
        var index = 0;
        for (let j = 0; j < hostLocationArray.length; j++) {
            if (hostLocationArray[j].Node.length > maxLength) {
                majorLocation = hostLocationArray[j].Location;
                maxLength = hostLocationArray[j].Node.length;
                index = j;
            }
        }

        for (let j = 0; j < hostLocationArray.length; j++) {
            if (j == index) {
                continue;
            }
            var location = hostLocationArray[j].Location;
            for (let k = 0; k < hostLocationArray[j].Node.length; k++) {
                abnormalInfosheet.content.push([hostLocationArray[j].Node[k], location, majorLocation])
            }
        }
    }
    if (0 < abnormalInfosheet.content.length) {
        outputArray.push(abnormalInfosheet);
    }

    // 主节点不在 activeLocation
    var detail = reelectAnalyzeInfo.Detail;
    if (Array.isArray(detail) && 0 < detail.length) {
        abnormalInfosheet = {"title": "[Abnormal: Primary Node Not In ActiveLocation]", "head": [["PrimaryNode", "NodeLocation", "ActiveLocation"]], "content": []};
        for (let i = 0; i < detail.length; i++) {
            if ("ActiveLocation" != detail[i].CausedBy) {
                continue;
            }
            var oldPrimary = detail[i].OldPrimary;
            var curLocation = nodeLocationObj[oldPrimary];
            if (null == curLocation) {
                curLocation = nodeLocationObj[oldPrimary.split(":")[0]];
            }
            abnormalInfosheet.content.push([oldPrimary, curLocation, "" + activeLocation]);
        }
        if (0 < abnormalInfosheet.content.length) {
            outputArray.push(abnormalInfosheet);
        }
    }

    // 节点异常 mode，主机中仅个别节点开启了 mode
    abnormalInfosheet = {"title": "[Abnormal: Node Not In GroupMode]", "head": [["HostName", "ServiceName"]], "content": []};
    var maintenanceArray = [];
    var criticalArray = [];
    // 如果部分节点未设置 location，并且开启了 mode ，在 dc.locationAnalyze() 中 GroupStatus 字段不包含这部分节点信息，需要单独处理
    for (let i = 0; i < groupModeInfo.length; i++){
        var groupMode = groupModeInfo[i].GroupMode;
        var properties = groupModeInfo[i].Properties;
        for (let j = 0; j < properties.length; j++){
            if (null != properties[j].Location) {
                continue;
            }
            var nodeName = properties[j].NodeName;
            if ("maintenance" == groupMode) {
                maintenanceArray.push(nodeName);
            } else {
                criticalArray.push(nodeName);
            }
        }
    }

    var curHostNodeObj = {};
    for (let i = 0; i < maintenanceArray.length; i++){
        var nodeName = maintenanceArray[i];
        var hostName = nodeName.split(':')[0];
        if (curHostNodeObj[hostName]) {
            curHostNodeObj[hostName].maintenanceNode.push(nodeName);
        } else {
            curHostNodeObj[hostName] = {"maintenanceNode": [nodeName]};
        }
    }
    for (let i = 0; i < criticalArray.length; i++){
        var nodeName = criticalArray[i];
        var hostName = nodeName.split(':')[0];
        if (curHostNodeObj[hostName]) {
            curHostNodeObj[hostName].criticalNode.push(nodeName);
        } else {
            curHostNodeObj[hostName] = {"criticalNode": [nodeName]};
        }
    }

    // 计算处于 mode 的节点与所有节点的占比，若占比相同，normal > maintenance > critical
    var abnromalObj = {};
    for (let i in curHostNodeObj){
        var hostName = i;
        var maintenanceNode = curHostNodeObj[hostName].maintenanceNode;
        var criticalNode = curHostNodeObj[hostName].criticalNode;
        var allNormalNode = hostNodeObj[hostName];
        var maintenanceCount = maintenanceNode ? maintenanceNode.length : 0;
        var criticalCount = criticalNode ? criticalNode.length : 0;
        var index = -1;
        if (0 != maintenanceCount && maintenanceCount >= criticalCount) {
            // 多数节点处于 maintenance
            // 填入 critical 节点
            for (let j = 0; criticalCount != 0 && j < criticalNode.length; j++){
                if (null == abnromalObj[hostName]) {
                    abnromalObj[hostName] = [criticalNode[j].split(':')[1]];
                } else {
                    abnromalObj[hostName].push(criticalNode[j].split(':')[1]);
                }
                index = allNormalNode.indexOf(criticalNode[j]);
                if (-1 != index) {
                    allNormalNode.splice(index, 1);
                }
            }
            // 填入剩下的普通节点
            for (let j in allNormalNode) {
                if (-1 == maintenanceNode.indexOf(allNormalNode[j])) {
                    if (null == abnromalObj[hostName]) {
                        abnromalObj[hostName] = [allNormalNode[j].split(':')[1]];
                    } else {
                        abnromalObj[hostName].push(allNormalNode[j].split(':')[1]);
                    }
                }
            }
        } else if (0 != criticalCount && criticalCount >= maintenanceCount) {
            // 多数节点处于 critical
            // 填入 maintenance 节点
            for (let j = 0; maintenanceCount != 0 && j < maintenanceNode.length; j++){
                if (null == abnromalObj[hostName]) {
                    abnromalObj[hostName] = [maintenanceNode[j].split(':')[1]];
                } else {
                    abnromalObj[hostName].push(maintenanceNode[j].split(':')[1]);
                }
                index = allNormalNode.indexOf(maintenanceNode[j]);
                if (-1 != index) {
                    allNormalNode.splice(index, 1);
                }
            }
            // 填入剩下的普通节点
            for (let j in allNormalNode) {
                if (-1 == criticalNode.indexOf(allNormalNode[j])) {
                    if (null == abnromalObj[hostName]) {
                        abnromalObj[hostName] = [allNormalNode[j].split(':')[1]];
                    } else {
                        abnromalObj[hostName].push(allNormalNode[j].split(':')[1]);
                    }
                }
            }
        }
    }

    for (let i in abnromalObj) {
        abnormalInfosheet.content.push([i, "" + abnromalObj[i]]);
    }

    if (0 < abnormalInfosheet.content.length) {
        outputArray.push(abnormalInfosheet);
    }

    abnormalInfosheet = {"title": "[Abnormal: Group Not In GroupMode]", "head": [["Location", "GroupName"]], "content": []};
    // critical 异常 mode，开启 critical 的 location 中是否缺少了部分 group
    abnromalObj = {};
    var criticalObj = {};
    for (let i in groupModeInfo){
        var groupMode = groupModeInfo[i].GroupMode;
        var properties = groupModeInfo[i].Properties;
        var groupName = groupModeInfo[i].GroupName;
        for (let j in properties){
            var location = properties[j].Location;
            if (null == location) {
                continue;
            }
            if ("critical" == groupMode) {
                if (null == criticalObj[location]) {
                    criticalObj[location] = [groupName];
                } else {
                    criticalObj[location].push(groupName);
                }
            }
        }
    }

    for (let i in criticalObj) {
        var location = i;
        var allGroup = hostNodeObj[location];
        var curGroup = criticalObj[location];
        // 对比 group 是否齐全
        if (allGroup.length == curGroup.length) {
            continue;
        }

        for (let j in allGroup) {
            if (-1 == curGroup.indexOf(allGroup[j])) {
                if (null == abnromalObj[location]) {
                    abnromalObj[location] = [allGroup[j]];
                } else {
                    abnromalObj[location].push(allGroup[j]);
                }            
            }
        }
    }

    for (let i in abnromalObj) {
        abnormalInfosheet.content.push([i, "" + abnromalObj[i]]);
    }

    if (0 < abnormalInfosheet.content.length) {
        outputArray.push(abnormalInfosheet);
    }

    // 整个主机未配置 location
    abnormalInfosheet = {"title": "[Abnormal: Empty Location Host]", "head": [["ClusterLocation", "HostName"]], "content": []};
    var noLocationHost = locationAnalyzeInfo.ExceptionHostInfo;
    if (noLocationHost && 0 < noLocationHost.NoLocationHost.length) {
        abnormalInfosheet.content.push(["" + Object.keys(locationHostObj), "" + noLocationHost.NoLocationHost]);
    }
    if (0 < abnormalInfosheet.content.length) {
        outputArray.push(abnormalInfosheet);
    }

    return outputArray;
}

// 处理 check 信息成可打印的数组
function handleCheckInfoToOutputArray(locationAnalyzeInfo, listGroupInfo, nodeInfo) {
    var outputArray = [];
    var emptyLocationHost = [];
    var unknownHost = [];

    // ------------- 处理异常主机 location（与配置不同） 表格 -------------
    var checkInfosheet = {"title": "[Diff: Host Location]", "head": [["HostName", "CurrentLocation", "ExpectLocation"]], "content": []};
    var currentLocation = {};
    var configLocation = {};

    for (let i in nodeInfo) {
        var hostname = nodeInfo[i].HostName;
        var location = nodeInfo[i].Location;
        if (null == currentLocation[hostname] || "ErrNodes" == currentLocation[hostname]) {
            currentLocation[hostname] = location;
        }
    }

    for (let i in initLocationObject) {
        var hostnames = initLocationObject[i];
        var location = i;
        for (let j in hostnames) {
            if (null == configLocation[hostnames[j]]) {
                configLocation[hostnames[j]] = location;
            }
        }
    }

    for (let i in configLocation) {
        if (null == currentLocation[i]) {
            // 集群不存在目标主机
            unknownHost.push(i);
        }
    }

    for (let i in currentLocation) {
        // 记录配置与当前集群不一致的 location
        if (null == configLocation[i] && "" == currentLocation[i]) {
            // 记录当前未配置 location，并且配置文件中也未提及的 hostname
            emptyLocationHost.push(i);
        } else if (configLocation[i] != currentLocation[i]) {
            if ("ErrNodes" == currentLocation[i]) {
                checkInfosheet.content.push([i, "Unknown", configLocation[i]]);
                // 整台机器故障，不清楚具体 location
            } else {
                checkInfosheet.content.push([i, currentLocation[i], configLocation[i]]);
            }
        } 
    }
    if (0 < checkInfosheet.content.length) {
        outputArray.push(checkInfosheet);
    }

    if (0 < unknownHost.length) {
        println("[ERROR] Host " + JSON.stringify(unknownHost) + " does not in current cluster, please check config file");
        throw new Error("[ERROR] Host " + JSON.stringify(unknownHost) + " does not in current cluster, please check config file");
    }

    // ------------- 处理异常 Host 没有 location，配置中也未设置 location 表格 -------------
    checkInfosheet = {"title": "[Diff: Empty Location]", "head": [["EmptyLocationHostName"]], "content": []};
    for (let i in emptyLocationHost) {
        checkInfosheet.content.push([emptyLocationHost[i]]);
    }
    if (0 < checkInfosheet.content.length) {
        outputArray.push(checkInfosheet);
    }

    // ------------- 处理异常 activeLocation（与配置不同） 表格 -------------
    var currentActiveLocation = locationAnalyzeInfo.ActiveLocation;
    var configActiveLocation = activeLocation;

    if ("" != configActiveLocation && null != currentActiveLocation && 'string' === typeof currentActiveLocation) {
        checkInfosheet = {"title": "[Diff: ActiveLocation]", "head": [["", "CurrentActiveLoation", "ExpectActiveLocation"]], "content": []};
        if (currentActiveLocation != configActiveLocation) {
            // 集群 activeLocation 与配置不一致
            checkInfosheet.content.push(["cluster", currentActiveLocation, configActiveLocation]);
        } else {
            // 一致
        }
    } else if ("" != configActiveLocation && null != currentActiveLocation && Array.isArray(currentActiveLocation)) {
        // 集群中存在多个 activeLocation
        checkInfosheet = {"title": "[Diff: ActiveLocation]", "head": [["GroupName", "CurrentActiveLoation", "ExpectActiveLocation"]], "content": []};
        var checkGroup = {};
        var flag = true;
        for (let i in currentActiveLocation) {
            for (let j in listGroupInfo) {
                var groupName = listGroupInfo[j].GroupName;
                if (true != checkGroup[groupName] && 'SYSCoord' != groupName) {
                    checkGroup[groupName] = true;
                    if (listGroupInfo[j].ActiveLocation != configActiveLocation) {
                        checkInfosheet.content.push([groupName, listGroupInfo[j].ActiveLocation, configActiveLocation]);
                    } else {
                        flag = false;
                    }
                }
            }
        }
        if (flag) {
            // 集群 activeLocation 与配置不一致
            checkInfosheet = {"title": "[Diff: ActiveLocation]", "head": [["", "CurrentActiveLoation", "ExpectActiveLocation"]], "content": []};
            checkInfosheet.content.push(["cluster", "" + currentActiveLocation, configActiveLocation]);
        }
    }
    if (0 < checkInfosheet.content.length) {
        outputArray.push(checkInfosheet);
    }
    return outputArray;
}

// 通过本机 coord 节点配置文件获取 catalog 节点，逐个尝试连接，直到连接上一个
function connCatalog() {
    var coord = sdbCoord.split(':')[1];
    try {
        var oma = new Oma("localhost", sdbCm);
        var catalogaddr = JSON.parse(oma.getNodeConfigs(coord)).catalogaddr;
    } catch (e) {
        println("[ERROR] Failed to get catalogaddr from " + coord + " config file: " + getLastErrObj());
        throw e;
    }

    var addrArray = catalogaddr.split(",");
    var cata;
    var nodeName = "";
    if (0 == addrArray.length) {
        println("[ERROR] Failed to get catalogaddr from " + coord + " config file");
        throw new Error("[ERROR] Failed to get catalogaddr from " + coord + " config file");
    }
    for (let i in addrArray) {
        try {
            var cipherUser = "";
            nodeName = addrArray[i];
            var cataHost = nodeName.split(':')[0];
            var cataPort = parseInt(nodeName.split(':')[1]) - 3;
            if ('' != sdbCipherFile) {
                if ('' != sdbToken) {
                    cipherUser = new CipherUser(sdbUser).token(sdbToken).cipherFile(sdbCipherFile);
                } else {
                    cipherUser = new CipherUser(sdbUser).cipherFile(sdbCipherFile);
                }
                cata = new Sdb(cataHost, cataPort, cipherUser);
            } else {
                cata = new Sdb(cataHost, cataPort, sdbUser, sdbPassword);
            }
        } catch (e) {
            if (-79 != e) {
                println("[ERROR] Failed to connect catalog node " + nodeName + " -> " + cataHost + ":" + cataPort + ": " + getLastErrObj());
                throw e;
            }
        }
    }

    if ("" == nodeName && null != cata) {
        println("[ERROR] Failed to connect catalog in " + catalogaddr);
        throw new Error("[ERROR] Failed to connect catalog in " + catalogaddr);
    }
    return cata;
}

// 对没有主节点的编目组进行强制升主，持续时间与 maxKeepTime 相同
function forceSetupCataLog(keepTime, flag) {
    var needSetup = flag;

    if (!flag) {
        try {
            var cursor = db.exec('select * from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup"');
            while (cursor.next()) {
                var current = cursor.current().toObj();
                if (null == current.ErrNodes) {
                    if (current.IsPrimary) {
                        needSetup = false;
                        break;
                    }
                }
                needSetup = true;
            }
        } catch (e) {
            println("Error getting catalog primary node info from $SNAPSHOT_DB: " + getLastErrObj());
            throw e;
        } finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    if (!needSetup) {
        // 不需要升主
        return false;
    }

    // 通过本机 coord 节点配置文件获取 catalog 节点，逐个尝试连接，将第一个连接上的节点强制升主
    var coord = sdbCoord.split(':')[1];
    try {
        var oma = new Oma("localhost", sdbCm);
        var catalogaddr = JSON.parse(oma.getNodeConfigs(coord)).catalogaddr;
    } catch (e) {
        println("[ERROR] Failed to get catalogaddr from " + coord + " config file: " + getLastErrObj());
        throw e;
    }

    var addrArray = catalogaddr.split(",");
    var cata;
    var nodeName = "";
    if (0 == addrArray.length) {
        println("[ERROR] Failed to get catalogaddr from " + coord + " config file");
        throw new Error("[ERROR] Failed to get catalogaddr from " + coord + " config file");
    }
    for (let i in addrArray) {
        try {
            var cipherUser = "";
            nodeName = addrArray[i];
            var cataHost = nodeName.split(':')[0];
            var cataPort = parseInt(nodeName.split(':')[1]) - 3;
            if ('' != sdbCipherFile) {
                if ('' != sdbToken) {
                    cipherUser = new CipherUser(sdbUser).token(sdbToken).cipherFile(sdbCipherFile);
                } else {
                    cipherUser = new CipherUser(sdbUser).cipherFile(sdbCipherFile);
                }
                cata = new Sdb(cataHost, cataPort, cipherUser);
            } else {
                cata = new Sdb(cataHost, cataPort, sdbUser, sdbPassword);
            }
        } catch (e) {
            if (-79 != e) {
                println("[ERROR] Failed to connect catalog node " + nodeName + " -> " + cataHost + ":" + cataPort + ": " + getLastErrObj());
                throw e;
            }
        }
    }

    if ("" == nodeName && null != cata) {
        println("[ERROR] Failed to connect catalog in " + catalogaddr);
        throw new Error("[ERROR] Failed to connect catalog in " + catalogaddr);
    }

    // 强制升主
    try {
        cata.forceStepUp({Seconds: 60 * keepTime, Enforced: enforce});
    } catch (e) {
        println("[ERROR] Failed to force step up catalog node " + nodeName + ": " + getLastErrObj());
        throw e;
    } finally {
        if (null != cata) {
            cata.close();
        }
    }

    return true;
}

// 检查目标是否存在 catalog，如果存在，则将主节点切换到目标上，超时时间 10 分钟
function reelectCatalogPrimary(location, hostname) {
    var reelectTarget = "";
    var needCheck = true;
    var target = "";
    var onlyErrNodes = false;
    try {
        if ("" != location) {
            var cursor = db.exec('select * from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup" and Location = "' + location +'"');
            target = location;
        } else if ("" != hostname) {
            var cursor = db.exec('select * from $SNAPSHOT_DB where GroupName = "SYSCatalogGroup" and HostName = "' + hostname +'"');
            target = hostname;
        }
        while (cursor.next()) {
            var current = cursor.current().toObj();
            if (null != current.ErrNodes) {
                onlyErrNodes = true;
                continue;
            }
            onlyErrNodes = false;
            if ("" != location) {
                if (!current.IsPrimary && needCheck) {
                    reelectTarget = current.HostName;
                } else {
                    needCheck = false;
                    reelectTarget = "";
                }
            } else if ("" != hostname && !current.IsPrimary) {
                reelectTarget = current.HostName;
            }
        }
    } catch (e) {
        println("Error in get $SNAPSHOT_DB: " + getLastErrObj());
        throw e;
    } finally {
        if (null != cursor) {
            cursor.close();
        }
    }

    if (onlyErrNodes) {
        println("[ERROR] Cannot reelect catalog primary node to error node: " + target);
        throw new Error("[ERROR] Cannot reelect catalog primary node to error node: " + target);
    }

    // 切主
    if ("" != reelectTarget) {
        println('  [WARNING] Reelect catalog primary node to ' + reelectTarget);
        try {
            db.getRG('SYSCatalogGroup').reelect({Seconds: 600, HostName: reelectTarget});
        } catch (e) {
            println("Error in reelect catalog primary node to " + reelectTarget + ": " + getLastErrObj());
            throw e;
        }
    }
}

// 执行 show 命令
function executeShow() {
    try {
        println("[INFO] Executing show command");
    
        print("   Running locationAnalyze ... ");
        var locationAnalyzeInfo = locationAnalyze();
        println("ok");

        print("   Running list SDB_LIST_GROUPMODES ... ");
        var groupModeInfo = listGroupModes();
        println("ok");

        print("   Running reelectAnalyze ... ");
        var reelectAnalyzeInfo = reelectAnalyze();
        println("ok");

        print("   Running snapshot SDB_SNAP_DATABASE ... ");
        var nodeInfo = getNodeInfo();
        println("ok");

        // 处理 Location 信息
        print("   Processing show information ... ");
        var outputArray = handleShowInfoToOutputArray(locationAnalyzeInfo, groupModeInfo, reelectAnalyzeInfo, nodeInfo);
        println("ok");

        // 按照 domain 过滤 show 的结果（只有机器上任意一个组在域中，整个机器的信息都会展示）
        var appliedDomainFilter = false;
        var domainsScope = [];
        if (showFilterWithDomain && 0 < domainList.length) {
            print("   Filter show information with domains ... ");
            appliedDomainFilter = true;
            domainsScope = [].concat(domainList);
            var domainInfo = listDomains();
            // 检查 domains 是否存在
            checkDomain(domainList, domainInfo);
            if (0 == domainMode) {
                // 排除模式
                domainList = excludeDomain(domainList, domainInfo);
            }
            // 过滤
            outputArray = filterShowInfoWithDomain(outputArray, nodeInfo, domainList, domainInfo);
            println("ok");
        }

        // 打印 Location 信息
        outputSheet(outputArray, showFile);
        println("   Output location file: \"" + defaultLocationFile + "\"");
        println("   Output show file: \"" + showFile + "\"");
        if (!quiet) {
            try {
                var cmd = new Cmd();
                var result = cmd.run("cat " + showFile);
                println(separator);
                if (appliedDomainFilter) {
                    println("[INFO] Apply domain mode: " + domainMode);
                    println("[INFO] Apply domains scope: " + JSON.stringify(domainsScope));
                } else {
                    println("[INFO] Apply scope: cluster");
                }
                println("[INFO] Show file content:");
                println(result);
                println(separator);
            } catch (e) {
                // 忽略错误
            }
        }

        println("[INFO] Show information successfully");
        println(separator);
    } catch (e) {
        println();
        println("locationAnalyzeInfo:" + JSON.stringify(locationAnalyzeInfo));
        println("groupModes:" + JSON.stringify(groupModeInfo));
        println("reelectAnalyzeInfo:" + JSON.stringify(reelectAnalyzeInfo));
        println("nodeInfo:" + JSON.stringify(nodeInfo));
        println("outputArray: " + JSON.stringify(outputArray));
        println(e);
        throw e;
    }
}

// 执行 check 命令
function executeCheck() {
    try {
        println("[INFO] Executing check command");

        print("   Running locationAnalyze ... ");
        var locationAnalyzeInfo = locationAnalyze();
        println("ok");
    
        print("   Running list SDB_LIST_GROUPS ... ");
        var listGroupInfo = listGroups();
        println("ok");
    
        print("   Running snapshot SDB_SNAP_DATABASE ... ");
        var nodeInfo = getNodeInfo();
        println("ok");
    
        // 处理 Location 信息，生成对比表格数组
        print("   Processing check information ... ");
        var outputArray = handleCheckInfoToOutputArray(locationAnalyzeInfo, listGroupInfo, nodeInfo);
        println("ok");

        // activeLocation 发生改变，并且切主参数不是 1 或者 2，打屏提醒用户
        if ("" != activeLocation && locationAnalyzeInfo.ActiveLocation != activeLocation && (1 != reelectLevel && 2 != reelectLevel)) {
            println("   [WARNING] Cluster activeLocation will be changed: \"" + locationAnalyzeInfo.ActiveLocation + "\" -> \"" + activeLocation + "\", but reelectLevel does not in [1, 2], it will not reelect caused by activeLocation after \"init\"");
        }

        // 打印对比表格
        if (0 < outputArray.length) {
            outputSheet(outputArray, checkFile);
            println("   Output check file: \"" + checkFile + "\"");
        } else {
            println("   All configurations are the same, check passed");
        }

        println("[INFO] Check completed successfully");
        println(separator);
    } catch (e) {
        println();
        println("locationAnalyzeInfo:" + JSON.stringify(locationAnalyzeInfo));
        println("listGroups:" + JSON.stringify(listGroupInfo));
        println("nodeInfo:" + JSON.stringify(nodeInfo));
        println("outputArray: " + JSON.stringify(outputArray));
        println(e);
        throw e;
    }
}

// 执行 init 命令
function executeInit() {
    println("[INFO] Executing init command");

    var locations = initLocationObject;
    if (!locations || Object.keys(locations).length === 0) {
        throw new Error("initLocationObject is not configured in config.js");
    }

    // 记录是否配置了 domain 过滤；排除模式可能使生效 domain 列表为空
    var useDomainFilter = 0 < domainList.length;
    var configuredDomains = [].concat(domainList);
    if (useDomainFilter) {
        var domainInfo = listDomains();
        checkDomain(domainList, domainInfo);
        println("   Domains: " + JSON.stringify(domainList));
        println("   DomainMode: " + domainMode);
        // 处理排除模式下的 domainList
        if (0 == domainMode) {
            domainList = excludeDomain(domainList, domainInfo);
        }
        if (0 == domainList.length) {
            println("[WARNING] No domains selected after applying domainMode, skip Domain-scoped location update");
        }
    }

    println("Configuration Summary:");
    println("   Total Locations: " + Object.keys(locations).length);
    for (var location in locations) {
        var hosts = locations[location];
        println("   - " + location + ": " + hosts.length + " host(s)");
    }

    if (activeLocation && activeLocation !== "") {
        println("   Active Location: \"" + activeLocation + "\"");
    } else {
        println("   Active Location: Not set");
    }

    println("[INFO] Initializing location configuration");
    var count = 0;
    for (var location in locations) {
        var hosts = locations[location];
        if (Array.isArray(hosts) && hosts.length > 0) {
            for (var i in hosts) {
                var host = hosts[i];
                println("   Setting location for host \"" + host + "\" -> \"" + location + "\"");
                try {
                    // 配置了 domain 过滤但生效列表为空时，不回退为集群级 setLocation
                    if (useDomainFilter) {
                        if (0 == domainList.length) {
                            continue;
                        }
                        dc.setLocation(host, location, {Domain: domainList});
                    } else {
                        dc.setLocation(host, location);
                    }
                    count++;
                } catch (e) {
                    let errObj = JSON.parse(getLastErrObj());
                    if (useDomainFilter) {
                        // 忽略不在主机不包含域中组的-6错误
                        if (-6 != errObj.errno || 0 != errObj.detail.indexOf("No group matches the speicfied host(")) {
                            println("[ERROR] Failed to set location for " + host + ": " + JSON.stringify(errObj));
                            println("[ERROR] Domain: " + JSON.stringify(domainList));
                            println("[ERROR] Configured domains: " + JSON.stringify(configuredDomains));
                        }
                    } else {
                        println("[ERROR] Failed to set location for " + host + ": " + JSON.stringify(errObj));
                    }
                }
            }
        }
    }

    if (count > 0) {
        println("   Successfully set locations for " + count + " host(s)");
    } else {
        println("No locations were set");
    }

    if (activeLocation && activeLocation !== "") {
        println("[INFO] Setting ActiveLocation: \"" + activeLocation + "\"");
        try {
            var result;
            // 与 setLocation 一致：按 domain 范围设置 ActiveLocation，避免影响域外复制组
            if (useDomainFilter) {
                if (0 == domainList.length) {
                    println("[WARNING] No domains selected after applying domainMode, skip Domain-scoped ActiveLocation update");
                } else {
                    result = dc.setActiveLocation(activeLocation, {Domain: domainList});
                }
            } else {
                result = dc.setActiveLocation(activeLocation);
            }
        } catch (e) {
            println("[ERROR] Failed to set active location: " + getLastErrObj());
            if (useDomainFilter) {
                println("[ERROR] Domain: " + JSON.stringify(domainList));
            }
        }

        // 设置 ActiveLocation 成功，但 reelectLevel 不为 1 或者 2 ，提醒用户
        try {
            result = JSON.parse(result);
            if (0 != result.SucceedNum && (1 != reelectLevel && 2 != reelectLevel)) {
                println("[WARNING] Cluster activeLocation changed to \"" + activeLocation + "\", but reelectLevel does not in [1, 2], it will not reelect caused by activeLocation");
            }
        } catch (e) {
            // JSON.parse 可能失败，不影响使用，忽略错误
        }

    }

    println("[INFO] Initialization completed successfully");
    println(separator);
}

// 执行 start_maintenance 命令
function executeStartMaintenance() {
    println("Executing start_maintenance command ... ");

    println("Configuration:");
    println("  MinKeepTime: " + minKeepTime + " minutes");
    println("  MaxKeepTime: " + maxKeepTime + " minutes");
    println("  Enforce: " + enforce);

    var domainArray = [];
    var locationsCount = 0;
    var hostnamesCount = 0;

    if (nodeTargetInfo && nodeTargetInfo.domains && nodeTargetInfo.domains.length > 0) {
        println("  Starting MaintenanceMode in Domain: " + JSON.stringify(nodeTargetInfo.domains));
        domainArray = nodeTargetInfo.domains;
    } else if (typeof domains !== 'undefined' && Array.isArray(domains)) {
        println("  Starting MaintenanceMode in Domain: " + JSON.stringify(domains));
        domainArray = domains;
    }

    try {
        var nodeInfo = getNodeInfo();
    } catch (e) {
        throw e;
    }

    if (nodeTargetInfo && nodeTargetInfo.locations && nodeTargetInfo.locations.length > 0) {
        for (var i = 0; i < nodeTargetInfo.locations.length; i++) {
            var location = nodeTargetInfo.locations[i];
            var rc = checkLocationExist(location, nodeInfo);
            if (rc) {
                println("  Starting MaintenanceMode for Location: " + location);
                try {
                    if (0 < domainArray.length) {
                        dc.startMaintenanceMode({
                            Location: location,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startMaintenanceMode({
                            Location: location,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start MaintenanceMode for location " + location + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown location: " + location);
            }
        }
    } else if (typeof locations !== 'undefined' && Array.isArray(locations)) {
        println("  Starting MaintenanceMode Location target: " + JSON.stringify(locations));
        for (let i in locations) {
            var rc = checkLocationExist(locations[i], nodeInfo);
            if (rc) {
                try {
                    println("  Starting MaintenanceMode for Location: " + locations[i]);
                    if (0 < domainArray.length) {
                        dc.startMaintenanceMode({
                            Location: locations[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startMaintenanceMode({
                            Location: locations[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start MaintenanceMode for location " + locations[i] + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown location: " + location);
            }
        }
    }

    if (nodeTargetInfo && nodeTargetInfo.hostnames && nodeTargetInfo.hostnames.length > 0) {
        for (var i = 0; i < nodeTargetInfo.hostnames.length; i++) {
            var host = nodeTargetInfo.hostnames[i];
            var rc = checkHostExist(host, nodeInfo);
            if (rc) {
                println("  Starting MaintenanceMode for Host: " + host);
                try {
                    if (0 < domainArray.length) {
                        dc.startMaintenanceMode({
                            HostName: host,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startMaintenanceMode({
                            HostName: host,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start MaintenanceMode for host " + host + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown host: " + host);
            }
        }
    } else if (typeof hostnames !== 'undefined' && Array.isArray(hostnames)) {
        println("  Starting MaintenanceMode Host target: " + JSON.stringify(hostnames));
        for (let i in hostnames) {
            var rc = checkHostExist(hostnames[i], nodeInfo);
            if (rc) {
                try {
                    println("  Starting MaintenanceMode for Host: " + hostnames[i]);
                    if (0 < domainArray.length) {
                        dc.startMaintenanceMode({
                            HostName: hostnames[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startMaintenanceMode({
                            HostName: hostnames[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start MaintenanceMode for host " + hostnames[i] + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown host: " + hostnames[i]);
            }
        }
    }

    println(separator);
    println("MaintenanceMode Started");
    println(separator);
    println("  Locations:  " + locationsCount);
    println("  Hostnames:  " + hostnamesCount);
    println("  Total:      " + (locationsCount + hostnamesCount));
    println(separator);
}

// 执行 stop_maintenance 命令
function executeStopMaintenance() {
    println("Executing stop_maintenance command ... ");

    var domainArray = [];
    var locationsCount = 0;
    var hostnamesCount = 0;
    var haveTarget = false;

    // 获取节点状态（如果指定了 --check）
    if (typeof check === 'boolean' && check) {
        print("Collecting node health info before stopping MaintenanceMode ... ");
        var healthInfo = getNodeHealthInfo();
        println("ok");
    } else {
        healthInfo = {};
    }

    if (nodeTargetInfo && nodeTargetInfo.domains && nodeTargetInfo.domains.length > 0) {
        println("  Stopping MaintenanceMode in Domain: " + JSON.stringify(nodeTargetInfo.domains));
        domainArray = nodeTargetInfo.domains;
    } else if (typeof domains !== 'undefined' && Array.isArray(domains)) {
        println("  Stopping MaintenanceMode in Domain: " + JSON.stringify(domains));
        domainArray = domains;
    }

    try {
        var groupModeInfo = listGroupModes();
        var nodeInfo = getNodeInfo();
    } catch (e) {
        throw e;
    }

    if (nodeTargetInfo && nodeTargetInfo.locations && nodeTargetInfo.locations.length > 0) {
        for (var i = 0; i < nodeTargetInfo.locations.length; i++) {
            var location = nodeTargetInfo.locations[i];
            // 检查该 location 是否存在开启 maintenancemode 的目标，有才关闭，没有则跳过
            var rc = checkLocationInMaintenance(location, groupModeInfo, nodeInfo);
            if (rc) {
                if (healthInfo._location && healthInfo._location[location]) {
                    // 存在故障节点，报错退出
                    println("[ERROR] There are faulty hosts in \"" + location + "\": " + JSON.stringify(healthInfo._location[location]));
                    throw new Error("[ERROR] Some nodes are not ok, cannot not stop MaintenanceMode");
                }
                println("  Stopping MaintenanceMode for Location: " + location);
                try {
                    if (0 < domainArray.length) {
                        dc.stopMaintenanceMode({ Location: location, Domain: domainArray});
                    } else {
                        dc.stopMaintenanceMode({ Location: location });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop MaintenanceMode for location " + location + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Location not in MaintenanceMode: " + location);
            }
            haveTarget = true;
        }
    } else if (typeof locations !== 'undefined' && Array.isArray(locations)) {
        println("  Stopping MaintenanceMode Location target: " + JSON.stringify(locations));
        for (let i in locations) {
            var rc = checkLocationInMaintenance(locations[i], groupModeInfo, nodeInfo);
            if (rc) {
                if (healthInfo._location && healthInfo._location[locations[i]]) {
                    // 存在故障节点，报错退出
                    println("[ERROR] There are faulty hosts in \"" + locations[i] + "\": " + JSON.stringify(healthInfo._location[locations[i]]));
                    throw new Error("[ERROR] Some nodes are not ok, cannot not stop MaintenanceMode");
                }
                try {
                    println("  Stopping MaintenanceMode for Location: " + locations[i]);
                    if (0 < domainArray.length) {
                        dc.stopMaintenanceMode({ Location: locations[i], Domain: domainArray});
                    } else {
                        dc.stopMaintenanceMode({ Location: locations[i] });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop MaintenanceMode for location " + locations[i] + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Location not in MaintenanceMode: " + locations[i]);
            }
            haveTarget = true;
        }
    }

    if (nodeTargetInfo && nodeTargetInfo.hostnames && nodeTargetInfo.hostnames.length > 0) {
        for (var i = 0; i < nodeTargetInfo.hostnames.length; i++) {
            var host = nodeTargetInfo.hostnames[i];
            var rc = checkHostInMaintenance(host, groupModeInfo, nodeInfo);
            if (rc) {
                if (healthInfo[host] && 0 < healthInfo[host].length) {
                    // 存在故障节点，报错退出
                    println("[ERROR] There are faulty nodes in \"" + host + "\": " + JSON.stringify(healthInfo[host]));
                    throw new Error("[ERROR] Some nodes are not ok, cannot not stop MaintenanceMode");
                }
                println("  Stopping MaintenanceMode for Host: " + host);
                try {
                    if (0 < domainArray.length) {
                        dc.stopMaintenanceMode({ HostName: host, Domain: domainArray});
                    } else {
                        dc.stopMaintenanceMode({ HostName: host });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop MaintenanceMode for host " + host + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Host not in MaintenanceMode: " + host);
            }
            haveTarget = true;
        }
    } else if (typeof hostnames !== 'undefined' && Array.isArray(hostnames)) {
        println("  Stopping MaintenanceMode Host target: " + JSON.stringify(hostnames));
        for (let i in hostnames) {
            var rc = checkHostInMaintenance(hostnames[i], groupModeInfo, nodeInfo);
            if (rc) {
                if (healthInfo[hostnames[i]] && 0 < healthInfo[hostnames[i]].length) {
                    // 存在故障节点，报错退出
                    println("[ERROR] There are faulty nodes in \"" + hostnames[i] + "\": " + JSON.stringify(healthInfo[hostnames[i]]));
                    throw new Error("[ERROR] Some nodes are not ok, cannot not stop MaintenanceMode");
                }
                println("  Stopping MaintenanceMode for Host: " + hostnames[i]);
                try {
                    if (0 < domainArray.length) {
                        dc.stopMaintenanceMode({ HostName: hostnames[i], Domain: domainArray});
                    } else {
                        dc.stopMaintenanceMode({ HostName: hostnames[i] });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop MaintenanceMode for host " + hostnames[i] + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Host not in MaintenanceMode: " + hostnames[i]);
            }
            haveTarget = true;
        }
    }

    var maintenanceCount = 0;
    if (!haveTarget && 0 == domainArray.length) {
        // 没有指定目标，检查如果集群存在 MaintenanceMode，则全部停止
        println("  Stopping MaintenanceMode for cluster");
        for (let i in healthInfo) {
            // 存在故障节点，报错退出
            println("[ERROR] There are faulty nodes in cluster: " + JSON.stringify(healthInfo));
            throw new Error("[ERROR] Some nodes are not ok, cannot not stop MaintenanceMode");
        }
        try {
            maintenanceCount = stopAllMaintenanceMode();
            if (maintenanceCount > 0) {
                println("  Stopped " + maintenanceCount + " MaintenanceMode instance(s)");
            } else {
                println("  No MaintenanceMode instances found, skip");
            }
        } catch (e) {
            println("[ERROR] Failed to stop MaintenanceMode for cluster: " + getLastErrObj());
            throw e;
        }
    }

    println(separator);
    println("MaintenanceMode Stopped");
    println(separator);
    println("  Locations:  " + locationsCount);
    println("  Hostnames:  " + hostnamesCount);
    println("  Total:      " + (locationsCount + hostnamesCount + maintenanceCount));
    println(separator);
}

// 执行 start_critical 命令
function executeStartCritical() {
    println("Executing start_critical command ... ");

    println("Configuration:");
    println("  MinKeepTime: " + minKeepTime + " minutes");
    println("  MaxKeepTime: " + maxKeepTime + " minutes");
    println("  Enforce: " + enforce);

    var domainArray = [];
    var locationsCount = 0;
    var hostnamesCount = 0;

    if (nodeTargetInfo && nodeTargetInfo.domains && nodeTargetInfo.domains.length > 0) {
        println("  Starting CriticalMode in Domain: " + JSON.stringify(nodeTargetInfo.domains));
        domainArray = nodeTargetInfo.domains;
    } else if (typeof domains !== 'undefined' && Array.isArray(domains)) {
        println("  Starting CriticalMode in Domain: " + JSON.stringify(domains));
        domainArray = domains;
    }

    try {
        var nodeInfo = getNodeInfo();
    } catch (e) {
        throw e;
    }

    if (nodeTargetInfo && nodeTargetInfo.locations && nodeTargetInfo.locations.length > 0) {
        for (var i = 0; i < nodeTargetInfo.locations.length; i++) {
            var location = nodeTargetInfo.locations[i];
            var rc = checkLocationExist(location, nodeInfo);
            if (rc) {
                println("  Starting CriticalMode for Location: " + location);
                try {
                    reelectCatalogPrimary(location, "");
                    if (0 < domainArray.length) {
                        dc.startCriticalMode({
                            Location: location,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startCriticalMode({
                            Location: location,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start CriticalMode for location " + location + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown location: " + location);
            }
        }
    } else if (typeof locations !== 'undefined' && Array.isArray(locations)) {
        println("  Starting CriticalMode Location target: " + JSON.stringify(locations));
        for (let i in locations) {
            var rc = checkLocationExist(locations[i], nodeInfo);
            if (rc) {
                try {
                    println("  Starting CriticalMode for Location: " + locations[i]);
                    reelectCatalogPrimary(locations[i], "");
                    if (0 < domainArray.length) {
                        dc.startCriticalMode({
                            Location: locations[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startCriticalMode({
                            Location: locations[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start CriticalMode for location " + locations[i] + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown location: " + locations[i]);
            }
        }
    }

    if (nodeTargetInfo && nodeTargetInfo.hostnames && nodeTargetInfo.hostnames.length > 0) {
        for (var i = 0; i < nodeTargetInfo.hostnames.length; i++) {
            var host = nodeTargetInfo.hostnames[i];
            var rc = checkHostExist(host, nodeInfo);
            if (rc) {
                println("  Starting CriticalMode for Host: " + host);
                try {
                    reelectCatalogPrimary("", host);
                    if (0 < domainArray.length) {
                        dc.startCriticalMode({
                            HostName: host,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startCriticalMode({
                            HostName: host,
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start CriticalMode for host " + host + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown host: " + host);
            }
        }
    } else if (typeof hostnames !== 'undefined' && Array.isArray(hostnames)) {
        println("  Starting CriticalMode Host target: " + JSON.stringify(hostnames));
        for (let i in hostnames) {
            var rc = checkHostExist(hostnames[i], nodeInfo);
            if (rc) {
                try {
                    println("  Starting CriticalMode for Host: " + hostnames[i]);
                    reelectCatalogPrimary("", hostnames[i]);
                    if (0 < domainArray.length) {
                        dc.startCriticalMode({
                            HostName: hostnames[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce,
                            Domain: domainArray
                        });
                    } else {
                        dc.startCriticalMode({
                            HostName: hostnames[i],
                            MinKeepTime: minKeepTime,
                            MaxKeepTime: maxKeepTime,
                            Enforced: enforce
                        });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to start CriticalMode for host " + hostnames[i] + ": " + getLastErrObj());
                    if (e == -334) {
                        println("  [INFO] If you need to force start GroupMode, try again after changing \"enforce = true\" in the configuration file");
                    }
                }
            } else {
                println("  Skip unknown host: " + hostnames[i]);
            }
        }
    }

    println(separator);
    println("CriticalMode Started");
    println(separator);
    println("  Locations:  " + locationsCount);
    println("  Hostnames:  " + hostnamesCount);
    println("  Total:      " + (locationsCount + hostnamesCount));
    println(separator);
}

// 执行 stop_critical 命令
function executeStopCritical() {
    println("Executing stop_critical command ... ");

    var domainArray = [];
    var locationsCount = 0;
    var hostnamesCount = 0;
    var haveTarget = false;

    // 获取节点状态（如果指定了 --check）
    if (typeof check === 'boolean' && check) {
        print("Collecting node health info before stopping CriticalMode ... ");
        var healthInfo = getNodeHealthInfo();
        for (let i in healthInfo) {
            // 存在故障节点，报错退出
            println("\n[ERROR] There are faulty nodes in cluster: " + JSON.stringify(healthInfo));
            throw new Error("[ERROR] Some nodes are not ok, cannot not stop CriticalMode");
        }
        println("ok");
    }

    if (nodeTargetInfo && nodeTargetInfo.domains && nodeTargetInfo.domains.length > 0) {
        println("  Stopping CriticalMode in Domain: " + JSON.stringify(nodeTargetInfo.domains));
        domainArray = nodeTargetInfo.domains;
    } else if (typeof domains !== 'undefined' && Array.isArray(domains)) {
        println("  Stopping CriticalMode in Domain: " + JSON.stringify(domains));
        domainArray = domains;
    }

    try {
        var groupModeInfo = listGroupModes();
        var nodeInfo = getNodeInfo();
    } catch (e) {
        throw e;
    }

    if (nodeTargetInfo && nodeTargetInfo.locations && nodeTargetInfo.locations.length > 0) {
        for (var i = 0; i < nodeTargetInfo.locations.length; i++) {
            var location = nodeTargetInfo.locations[i];
            // 检查该 location 是否存在开启 CriticalMode 的目标，有才关闭，没有则跳过
            var rc = checkLocationInCritical(location, groupModeInfo, nodeInfo);
            if (rc) {
                println("  Stopping CriticalMode for Location: " + location);
                try {
                    if (0 < domainArray.length) {
                        dc.stopCriticalMode({ Location: location, Domain: domainArray});
                    } else {
                        dc.stopCriticalMode({ Location: location });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop CriticalMode for location " + location + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Location not in CriticalMode: " + location);
            }
            haveTarget = true;
        }
    } else if (typeof locations !== 'undefined' && Array.isArray(locations)) {
        println("  Stopping CriticalMode Location target: " + JSON.stringify(locations));
        for (let i in locations) {
            var rc = checkLocationInCritical(locations[i], groupModeInfo, nodeInfo);
            if (rc) {
                try {
                    println("  Stopping CriticalMode for Location: " + locations[i]);
                    if (0 < domainArray.length) {
                        dc.stopCriticalMode({ Location: locations[i], Domain: domainArray});
                    } else {
                        dc.stopCriticalMode({ Location: locations[i] });
                    }
                    locationsCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop CriticalMode for location " + locations[i] + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Location not in CriticalMode: " + locations[i]);
            }
            haveTarget = true;
        }
    }

    if (nodeTargetInfo && nodeTargetInfo.hostnames && nodeTargetInfo.hostnames.length > 0) {
        for (var i = 0; i < nodeTargetInfo.hostnames.length; i++) {
            var host = nodeTargetInfo.hostnames[i];
            var rc = checkHostInCritical(host, groupModeInfo, nodeInfo);
            if (rc) {
                println("  Stopping CriticalMode for Host: " + host);
                try {
                    if (0 < domainArray.length) {
                        dc.stopCriticalMode({ HostName: host, Domain: domainArray});
                    } else {
                        dc.stopCriticalMode({ HostName: host });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop CriticalMode for host " + host + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Host not in CriticalMode: " + host);
            }
            haveTarget = true;
        }
    } else if (typeof hostnames !== 'undefined' && Array.isArray(hostnames)) {
        println("  Stopping CriticalMode Host target: " + JSON.stringify(hostnames));
        for (let i in hostnames) {
            var rc = checkHostInCritical(hostnames[i], groupModeInfo, nodeInfo);
            if (rc) {
                try {
                    println("  Stopping CriticalMode for Host: " + hostnames[i]);
                    if (0 < domainArray.length) {
                        dc.stopCriticalMode({ HostName: hostnames[i], Domain: domainArray});
                    } else {
                        dc.stopCriticalMode({ HostName: hostnames[i] });
                    }
                    hostnamesCount++;
                } catch (e) {
                    println("  [WARNING] Failed to stop CriticalMode for host " + hostnames[i] + ": " + getLastErrObj());
                }
            } else {
                println("  Skip Host not in CriticalMode: " + hostnames[i]);
            }
            haveTarget = true;
        }
    }

    var criticalCount = 0;
    if (!haveTarget && 0 == domainArray.length) {
        // 没有指定目标，检查如果集群存在 CriticalMode，则全部停止
        println("  Stopping CriticalMode for cluster");
        try {
            criticalCount = stopAllCriticalMode();
            if (criticalCount > 0) {
                println("  Stopped " + criticalCount + " CriticalMode instance(s)");
            } else {
                println("  No CriticalMode instances found, skip");
            }
        } catch (e) {
            println("[ERROR] Failed to stop CriticalMode for cluster: " + getLastErrObj());
            throw e;
        }
    }

    println(separator);
    println("CriticalMode Stopped");
    println(separator);
    println("  Locations:  " + locationsCount);
    println("  Hostnames:  " + hostnamesCount);
    println("  Total:      " + (locationsCount + hostnamesCount + criticalCount));
    println(separator);
}

// 执行 restore 命令
function executeRestore() {
    println("Executing restore command ... ");

    println(separator);
    println("Cluster Restore Operation");
    println(separator);

    if (!checkClusterHealth()) {
        println(separator);
        println("[ERROR] Some nodes are not OK, cannot restore cluster");
        println(separator);
        throw new Error("Some nodes are not OK, cannot restore cluster");
    } else {
        println("Cluster health status: OK - all nodes are healthy");
    }

    var maintenanceCount = stopAllMaintenanceMode();
    if (maintenanceCount > 0) {
        println("  Stopped " + maintenanceCount + " MaintenanceMode instance(s)");
    } else {
        println("  No MaintenanceMode instances found, skip");
    }

    var criticalCount = stopAllCriticalMode();
    if (criticalCount > 0) {
        println("  Stopped " + criticalCount + " CriticalMode instance(s)");
    } else {
        println("  No CriticalMode instances found, skip");
    }

    println(separator);
    println("Cluster Restore Completed");
    println("  MaintenanceMode stopped: " + maintenanceCount);
    println("  CriticalMode stopped: " + criticalCount);
    println(separator);
}

// 调用主函数
main();
println("");
