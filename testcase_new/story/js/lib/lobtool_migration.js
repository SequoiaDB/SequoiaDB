import("lobtool_commlib.js");
LobToolTest.prototype.setUp = function (paras) {
    if (paras === undefined || paras.user === undefined) {
        var user = "";
    } else {
        var user = paras.user;
    }

    if (paras === undefined || paras.passwd === undefined) {
        var passwd = "";
    } else {
        var passwd = paras.passwd
    }

    if (paras === undefined || paras.useSSL === undefined) {
        var useSSL = false;
    } else {
        var useSSL = paras.useSSL;
    }

    this.otherCLName = CHANGEDPREFIX + "_newcl";
    this.conf = {
        srcFullCL: COMMCSNAME + "." + COMMCLNAME,       // 源集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
        dstFullCL: COMMCSNAME + "." + this.otherCLName,        // 目标集合
        srcUsr: user,
        srcPwd: passwd,
        destUsr: user,
        destPwd: passwd,
        useSsl: useSSL
    };

    // 创建包含大对象的导出集合
    this.lobfile = toolMakeLobfile();
    var expCl = commCreateCL(db, COMMCSNAME, COMMCLNAME);
    var lobNum = 2;
    this.oids = toolPutLobs(expCl, this.lobfile, lobNum);

    // 获取putLob文件的Md5值
    this.wMd5 = getFileMd5(cmd, this.lobfile);
}

LobToolTest.prototype.test = function () {
    // sdblobtool 迁移的选项参数
    var args = buildMigrationArgs(this.conf.srcFullCL, this.conf.dstFullCL, this.conf.srcUsr, this.conf.srcPwd,
        this.conf.destUsr, this.conf.destPwd, this.conf.useSsl);

    // 创建导入集合并将大对象导入
    this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);
    execLobToolCommand(args);
}

function main(db, paras) {
    println("call main");
    var test = new LobToolTest();
    if (!test.checkEnv(paras)) {
        return;
    }

    try {
        test.setUp(paras);
        test.test();
        test.checkResult();
    } finally {
        test.tearDown(paras);
    }
}
