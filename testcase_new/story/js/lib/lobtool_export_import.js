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
        var passwd = paras.passwd;
    }

    if (paras === undefined || paras.useSSL === undefined) {
        var useSSL = false;
    } else {
        var useSSL = paras.useSSL;
    }
    var filename = CHANGEDPREFIX + "_test.file";	      // CHANGEDPREFIX = "local_test" 文件名
    this.otherCLName = CHANGEDPREFIX + "_newcl";
    this.conf = {
        exportFile: LocalPath + "/" + filename,         // 导出文件，导入文件	
        expFullCL: COMMCSNAME + "." + COMMCLNAME,       // 导出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
        impFullCL: COMMCSNAME + "." + this.otherCLName,         // 导入集合}
        usr: user,
        pwd: passwd,
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
    var args = buildImportOrExprtArgs("export", this.conf.expFullCL, this.conf.exportFile, this.conf.usr, this.conf.pwd, this.conf.useSsl);
    // 导出大对象到文件exportFile
    execLobToolCommand(args);
    // 创建导入集合并将大对象导入
    this.otherCl = commCreateCL(db, COMMCSNAME, this.otherCLName);
    args["operation"] = "import";
    args["collection"] = this.conf.impFullCL;
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
