function generatePublicVar(file)
{
   if (undefined === file)
   {
      throw "invalid file";
   }

   file.write("if(undefined === coordHostName)\n");
   file.write("{\n");
   file.write("   var coordHostName = 'localhost';\n");
   file.write("}\n\n");

   file.write("if(undefined === coordPort)\n");
   file.write("{\n");
   file.write("   var coordPort = 11810;\n");
   file.write("}\n\n");

   file.write("if(undefined === needDeploy)\n");
   file.write("{\n");
   file.write("   var needDeploy = false;\n");
   file.write("}\n\n");

   file.write("if(undefined === needCreateDomain)\n");
   file.write("{\n");
   file.write("   var needCreateDomain = true;\n");
   file.write("}\n\n");
}

// Get data source name by data source id.
function getDSNameByID(dsSet, id)
{
   try
   {
      for (var i = 0; i < dsSet.length; ++i)
      {
         if (dsSet[i].ID === id)
         {
            return dsSet[i].Name;
         }
      }
      throw "getDSNameByID: Data source of id " + id + " dose not exist";
   }
   catch (e)
   {
      throw "getDSNameByID " + e;
   }
}

//获取组定义
function getGroupDefine(db, jsFile)
{
   try
   {
      var cursor = db.list(7);
      jsFile.write("var groups = [");
      var firsttime = true;
      while (cursor.next())
      {
         var obj = cursor.current().toObj();
         var group = {}
         group.name = obj.GroupName;
         group.nodes = [];

         for (var i = 0; i < obj.Group.length; ++i)
         {
            var node = {};
            node.HostName = obj.Group[i].HostName;
            node.dbpath = obj.Group[i].dbpath;
            node.serviceName = obj.Group[i].Service[0].Name;
            group.nodes.push(node);
         }

         if (firsttime)
         {
            jsFile.write(JSON.stringify(group) + ",");
            firsttime = false;
         }
         else
         {
            jsFile.write("\n");
            jsFile.write("              " + JSON.stringify(group) + ",");

         }
      }

      jsFile.write("];");
      jsFile.write("\n\n");
      println("getGroupDefine success");

   }
   catch(e)
   {
      throw "getGroupDefine " + e;
   }
}

function isContain(nameSet, name)
{
   if (undefined === nameSet ||
       !(nameSet instanceof Array) ||
       0 === nameSet.length)
   {
      return true;
   }

   for (var i = 0; i < nameSet.length; ++i)
   {
      if (nameSet[i] === name)
      {
         return true;
      }
   }

   return false;
}

//获取domain定义
function getDomainDefine(db, jsFile)
{
   try
   {
      var cursor = db.listDomains();

      jsFile.write("var domains = [")
      var firsttime = true;
      while (cursor.next())
      {
         var obj = cursor.current().toObj();
         // 该domain是否需要创建
         if (!isContain(inDomainNames, obj.Name))
         {
            continue;
         }

         var domain = {};
         domain.name = obj.Name;
         domain.groups = [];
         for (var i = 0; i < obj.Groups.length; ++i)
         {
            domain.groups.push(obj.Groups[i].GroupName);
         }
         if (undefined !== obj.AutoSplit)
         {
            domain.option = {};
            domain.option.AutoSplit = obj.AutoSplit;
         }
         if (true === firsttime)
         {
            jsFile.write(JSON.stringify(domain) + ",");
            firsttime = false;
         }
         else
         {
            jsFile.write("\n");
            jsFile.write("               " + JSON.stringify(domain) + ",");
         }
      }

      jsFile.write("];\n\n");
      println("getDomainDefine success");
   }
   catch(e)
   {
      throw "getDomainDefine " + e;
   }
}

// Get data source metadata.
function getDSDefine(db, jsFile, dsSet)
{
   try
   {
      var cursor = db.SYSCAT.SYSDATASOURCES.find();
      jsFile.write("var dsSet = [");
      var firsttime = true;
      while (cursor.next())
      {
         var ds = {};
         var obj = cursor.current().toObj();
         if (!isContain(inDSNames, obj.Name))
         {
            continue;
         }

         ds.Name = obj.Name;
         ds.Address = obj.Address;
         ds.User = obj.User;
         ds.Password = obj.Password;
         ds.Type = obj.Type;
         ds.option = {};
         ds.option.AccessMode = obj.AccessModeDesc;
         ds.option.ErrorFilterMask = obj.ErrorFilterMaskDesc;
         ds.option.ErrorControlLevel = obj.ErrorControlLevel;
         // These are new added properties in later version.
         if (undefined !== obj.TransPropagateMode)
         {
            ds.option.TransPropagateMode = obj.TransPropagateMode;
         }
         if (undefined !== obj.InheritSessionAttr)
         {
            ds.option.InheritSessionAttr = obj.InheritSessionAttr ;
         }

         if (true === firsttime)
         {
            jsFile.write(JSON.stringify(ds) + ",");
            firsttime = false;
         }
         else
         {
            jsFile.write("\n");
            jsFile.write("             " + JSON.stringify(ds) + ",");
         }
         // We don't want to write the data source id in the file. So set it
         // after writting other metadata.
         ds.ID = obj.ID;
         dsSet.push(ds);
      }
      jsFile.write("];\n\n");
      println("getDSDefine success");
   }
   catch (e)
   {
      // In old versions, data source is not supported.
      if (-23 != e)
      {
         throw "getDSDefine " + e;
      }
   }
}

// 获取CS定义
function getCSDefine(db, dbcata, jsFile, csSet, dsSet)
{
   try
   {
      var cursor = dbcata.SYSCAT.SYSCOLLECTIONSPACES.find();
      jsFile.write("var csSet = [");
      var firsttime = true;
      while(cursor.next())
      {
         var useDataSource = false;
         var cs = {};
         var obj = cursor.current().toObj();
         // 该CS是否需要创建
         if (!isContain(inCSNames, obj.name))
         {
            continue;
         }

         cs.name = obj.Name;
         cs.option = {};
         // First, let's check data source options. If data source is being
         // used, other options should not appear in the metadata record.
         // Otherwise it will fail when creating cs using the metadata.
         if (undefined !== obj.DataSourceID)
         {
            cs.option.DataSource = getDSNameByID(dsSet, obj.DataSourceID);
            useDataSource = true;
         }
         if (undefined !== obj.Mapping)
         {
            cs.option.Mapping = obj.Mapping;
         }

         if (false === useDataSource)
         {
            var curDomainName = "";
            if (undefined !== obj.Domain)
            {
               cs.option.Domain = obj.Domain;
               curDomainName = obj.Domain;
            }

            // 该domain是否需要创建，如果domain没有指定，则CS也排除
            if (!isContain(inDomainNames, curDomainName))
            {
               csSet.push(cs.name);
               continue;
            }

            cs.option.LobPageSize = obj.LobPageSize;
            cs.option.PageSize = obj.PageSize;
         }

         if (true === firsttime)
         {
            jsFile.write(JSON.stringify(cs) + ",");
            firsttime = false;
         }
         else
         {
            jsFile.write("\n");
            jsFile.write("             "  + JSON.stringify(cs) + ",");
         }
      }

      jsFile.write("];\n\n");
      println("getCSDefine success");
   }
   catch(e)
   {
      throw "getCSDefine " + e;
   }
}

// 获取CL定义
function getCLDefine(db, jsFile, csSet, dsSet, dsMappingCLSet)
{
   try
   {
      var cursor = db.snapshot(8);
      jsFile.write("var clSet = [");
      var firsttime = true;
      while (cursor.next())
      {
         var useDataSource = false;
         var cl = {};
         var obj = cursor.current().toObj();

         var names = obj.Name.split(".");
         // 该CL是否需要创建
         if (!isContain(inCLNames, names[1]))
         {
            continue;
         }

         // 该CL对应的CS是否需要创建
         if (!isContain(inCSNames, names[0]))
         {
            continue;
         }

         if (csSet.length !=0 &&
             isContain(csSet, names[0]))
         {
            continue;
         }

         cl.name = obj.Name;
         cl.option = {};
         if (undefined !== obj.DataSourceID)
         {
            cl.option.DataSource = getDSNameByID(dsSet, obj.DataSourceID);
            useDataSource = true ;
            dsMappingCLSet.push(obj.Name);
         }
         if (undefined !== obj.Mapping)
         {
            cl.option.Mapping = obj.Mapping;
         }

         if (false === useDataSource)
         {
            if (undefined !== obj.ShardingType)
            {
               cl.option.ShardingType = obj.ShardingType;
               cl.option.ShardingKey = obj.ShardingKey;
            }

            if (undefined !== obj.EnsureShardingIndex)
            {
               cl.option.EnsureShardingIndex = obj.EnsureShardingIndex
            }

            if (undefined === obj.AutoSplit &&
                1 === obj.CataInfo.length)
            {
               cl.option.Group = obj.CataInfo[0].GroupName;
            }

            if (undefined !== obj.IsMainCL)
            {
               cl.option.IsMainCL = true;

               cl.subcls = [];
               for (var i = 0; i< obj.CataInfo.length; ++i)
               {
                  var subcl = {};
                  subcl.name = obj.CataInfo[i].SubCLName;
                  subcl.attchOpt = {}

                  subcl.attchOpt.LowBound = {};
                  subcl.attchOpt.UpBound = {};
                  for (key in cl.option.ShardingKey)
                  {
                     subcl.attchOpt.LowBound[key] = obj.CataInfo[i].LowBound[key];
                     subcl.attchOpt.UpBound[key] = obj.CataInfo[i].UpBound[key];
                  }
                  cl.subcls.push(subcl);
               }
            }

            if (undefined !== obj.Partition)
            {
               cl.option.Partition = obj.Partition;
            }

            if (undefined !== obj.AutoIndexId)
            {
               cl.option.Partition = obj.AutoIndexId;
            }

            if (undefined !== obj.Attribute &&
                0 !== obj.Attribute)
            {
               cl.option.Compressed = true;

               if (undefined !== obj.CompressionType &&
                   1 === obj.CompressionType)
               {
                  cl.option.CompressionType = "lzw";
               }
            }

            if (undefined !== obj.ReplSize)
            {
               cl.option.ReplSize = obj.ReplSize;
            }

            cl.CataInfo = [];
            if (obj.CataInfo.length === 1)
            {
               cl.option.Group = obj.CataInfo[0].GroupName;
            }
            else
            {
               for(var i = 0; i< obj.CataInfo.length; ++i)
               {
                  var cataItem = {};
                  cataItem.group = obj.CataInfo[i].GroupName;
                  if (cl.option.ShardingType === "hash")
                  {
                     cataItem.LowBound = obj.CataInfo[i].LowBound;
                     for(var k in cataItem.LowBound)
                     {
                       if (k === "") cataItem.LowBound={'Partition':obj.CataInfo[i].LowBound[""]}
                     }
                     cataItem.UpBound = obj.CataInfo[i].UpBound;
                     for(var k in cataItem.UpBound)
                     {
                       if (k === "") cataItem.UpBound={'Partition':obj.CataInfo[i].UpBound[""]}
                     }
                  }
                  else
                  {
                     cataItem.LowBound = obj.CataInfo[i].LowBound;
                     cataItem.UpBound = obj.CataInfo[i].UpBound;
                  }
                  cl.CataInfo.push(cataItem);
               }
            }
         }

         if (true === firsttime)
         {
            jsFile.write(JSON.stringify(cl) + ",");
            firsttime = false;
         }
         else
         {
            jsFile.write("\n");
            jsFile.write("             " + JSON.stringify(cl) + ",");
         }
      }

      jsFile.write("];\n\n");
      println("getCLDefine success");
   }
   catch(e)
   {
      throw "getCLDefined" + e;
   }
}

// 获取索引定义
function getIndexDefine(db, jsFile, csSet, excludeCLSet)
{
   try
   {
      var clSet = db.list(4).toArray();
      jsFile.write("var indexDefs = [");
      var firsttime = true;
      for (var i = 0; i < clSet.length; ++i)
      {
         var obj = eval("(" + clSet[i] + ")");
         var fullname = obj.Name;
         if (isContain(excludeCLSet, fullname))
         {
            continue;
         }
         var names = fullname.split(".");
         // 该CL是否需要创建
         if (!isContain(inCLNames, names[1]))
         {
            continue;
         }

         // 该CL对应的CS是否需要创建
         if (!isContain(inCSNames, names[0]))
         {
            continue;
         }
         if (csSet.length !=0 &&
             isContain(csSet, names[0]))
         {
            continue;
         }
         if (2 === names.length )
         {
            var cl = db.getCS(names[0]).getCL(names[1]);
            var cursor = cl.listIndexes();
            var clIndexs = {}
            clIndexs.name = fullname;
            clIndexs.indexs = [];
            while (cursor.next())
            {
               var index = {};
               var obj = cursor.current().toObj();
               if ("$id" !== obj.IndexDef.name &&
                   "$shard" !== obj.IndexDef.name)
               {
                  index.name = obj.IndexDef.name;
                  index.key = obj.IndexDef.key;
                  index.unique = obj.IndexDef.unique;
                  index.enforced = obj.IndexDef.enforced;
                  clIndexs.indexs.push(index);
               }
            }

            if (true === firsttime &&
                0 !== clIndexs.indexs.length )
            {
               jsFile.write(JSON.stringify(clIndexs) + ",");
               firsttime = false;
            }
            else if(0 !== clIndexs.indexs.length)
            {
               jsFile.write("\n");
               jsFile.write("                " + JSON.stringify(clIndexs) + ",");
            }
         }
      }

      jsFile.write("];\n\n");
      println("getIndexDefine success");
   }
   catch(e)
   {
      throw "getIndexDefine " + e;
   }
}

// 生成部署函数
function generatDeployFunction(file)
{
var functionContext = "\n\
function deployCluster(db, groups) \n\
{\n\
   for (var i = 0; i < groups.length; ++i) \n\
   {\n\
      if ('SYSCatalogGroup' === groups[i].name)\n\
      {\n\
         for (var k = 0; k < groups[i].nodes.length; ++k)\n\
         {\n\
            var node = groups[i].nodes[k];\n\
            if (0 === k)\n\
            {\n\
               var catarg = db.createCataRG(node.HostName, node.serviceName, node.dbpath);\n\
            }\n\
            else\n\
            {\n\
               db.getRG(1).createNode(node.HostName, node.serviceName, node.dbpath);\n\
            }\n\
         }\n\
         db.getRG(1).start();\n\
         println('create catalog rg done');\n\
      }\n\
   }\n\
   \n\
   for (var i = 0; i < groups.length; ++i)\n\
   {\n\
      if ('SYSCatalogGroup' === groups[i].name)\n\
      {\n\
         continue;\n\
      }\n\
      \n\
      if ('SYSCoord' === groups[i].name)\n\
      {\n\
         var rg = db.createCoordRG();\n\
      }\n\
      else\n\
      {\n\
         var rg = db.createRG(groups[i].name);\n\
      }\n\
      \n\
      for (var k = 0; k < groups[i].nodes.length; ++k)\n\
      {\n\
         var node = groups[i].nodes[k];\n\
         rg.createNode(node.HostName, node.serviceName, node.dbpath);\n\
      }\n\
      rg.start();  \n\
      println('create group ' + groups[i].name + ' done');\n\
   }\n\
}"

   file.write(functionContext + "\n");
   println("generatDeployFunction success");
}

function generatCreateDSFunction(file)
{
var functionContext = "\n\
function createAllDS(db, dsSet)\n\
{\n\
   for (var i = 0; i < dsSet.length; ++i)\n\
   {\n\
      db.createDataSource(dsSet[i].Name, dsSet[i].Address, dsSet[i].User, dsSet[i].Password, dsSet[i].Type, dsSet[i].option);\n\
      println('createDataSource(' + dsSet[i].Name + ',' + JSON.stringify(dsSet[i].option) + ') success');\n\
   }\n\
}";
   file.write(functionContext + "\n");
   println("generatCreateDSFunction success");
}

// 生成CS创建函数
function generatCreateCSFunction(file)
{
var functionContext = "\n\
function createAllCS(db, csSet)\n\
{\n\
   for (var i = 0; i < csSet.length; ++i)\n\
   {\n\
      db.createCS(csSet[i].name, csSet[i].option);\n\
      println('createCS(' + csSet[i].name + ',' + JSON.stringify(csSet[i].option) + ') success');\n\
   }\n\
}";
   file.write(functionContext + "\n");
   println("generatCreateCSFunction success");
}

// 生成获取CL所在组的函数
function generatGetSrcGroup(file)
{
    var functionContext = "\n\
function getSrcGroup(db, fullName)\n\
{\n\
    var cursor = db.snapshot(8, {Name: fullName});\n\
    var doc = cursor.next().toObj();\n\
    return doc['CataInfo'][0]['GroupName'];\n\
}"
   file.write(functionContext + "\n");
   println("generatGetSrcGroup success");
}

// 生成切分函数
function generatSplitCL(file)
{
   var functionContext = "\n\
function splitCL(cl, srcGroup, cataInfo)\n\
{\n\
    for (var i = 0; i < cataInfo.length; ++i)\n\
    {\n\
        if (srcGroup !== cataInfo[i].group)\n\
        {\n\
           cl.split(srcGroup, cataInfo[i].group, cataInfo[i].LowBound, cataInfo[i].UpBound);\n\
        }\n\
    }\n\
}"
   file.write(functionContext + "\n");
   println("generatSplitCL success");
}

// 生成CL创建函数
function generatCreateCLFunction(file)
{
   var functionContext = "\n\
function createAllCL(db, clSet)\n\
{\n\
   var mainCLSet  = [];\n\
   for (var i = 0; i < clSet.length; ++i)\n\
   {\n\
      var names = clSet[i].name.split('.');\n\
      var cl = db.getCS(names[0]).createCL(names[1], clSet[i].option); \n\
      println('createCL(' + names[1] + ',' + JSON.stringify(clSet[i].option) + ') success');\n\
      \n\
      if (undefined !== clSet[i].option.IsMainCL && \n\
          true === clSet[i].option.IsMainCL)\n\
      {\n\
         mainCLSet.push(clSet[i]);\n\
      }\n\
      else if (undefined !== clSet[i].CataInfo)\n\
      {\n\
         var srcGroup = getSrcGroup(db, clSet[i].name);\n\
         splitCL(cl, srcGroup, clSet[i].CataInfo);\n\
      }\n\
   }\n\
   \n\
   for (var i = 0; i<mainCLSet.length; ++i)\n\
   {\n\
      name = mainCLSet[i].name.split('.');\n\
      for (var j = 0; j < mainCLSet[i].subcls.length; ++j)\n\
      {\n\
         var item = mainCLSet[i].subcls[j];\n\
         db.getCS(name[0]).getCL(name[1]).attachCL(item.name, item.attchOpt);\n\
         println('db.' + mainCLSet[i].name + '.attachCL(' + item.name + ',' + JSON.stringify(item.attchOpt) + ') success');\n\
      }\n\
   }\n\
}"
   file.write(functionContext + "\n");
   println("generatCreateCLFunction success");
}

// 生成修改domain属性的方法
function generatAlterAllDomains(file)
{
   var functionContext = "\n\
function alterAllDomains(db, domains)\n\
{\n\
   for (var i = 0; i < domains.length; ++i)\n\
   {\n\
      if (undefined !== domains[i].option && domains[i].option.AutoSplit)\n\
      {\n\
         var dm = db.getDomain(domains[i].name);\n\
         dm.alter({AutoSplit:true});\n\
         println('alterDomain ' + domains[i].name + ' success');\n\
      }\n\
   }\n\
}"
   file.write(functionContext + "\n");
   println("generatAlterAllDomains success");
}

// 生成main函数
function generatmainFunction(file)
{
   var functionContext = "\n\
function main()\n\
{\n\
   try\n\
   {\n\
      var db = new Sdb(coordHostName, coordPort);\n\
      \n\
      if (step === 0)\n\
      {\n\
         deployCluster(db, groups);\n\
      }\n\
      else if (step === 1)\n\
      {\n\
         createAllDomains(db, domains);\n\
      }\n\
      else if (step === 2)\n\
      {\n\
         createAllDS(db, dsSet);\n\
         createAllCS(db,csSet);\n\
         createAllCL(db, clSet);\n\
      }\n\
      else if (step ===3)\n\
      {\n\
         createAllIndexes(db,indexDefs);\n\
      }\n\
      else if (step === 4)\n\
      {\n\
         alterAllDomains(db, domains);\n\
      }\n\
   }\n\
   catch(e)\n\
   {\n\
      throw e;\n\
   }\n\
   finally\n\
   {\n\
      if (undefined !== db)\n\
      {\n\
         db.close();\n\
      }\n\
   }\n\
}\n\
println('Usage:')\n\
println('      -e \\\'var coordHostName = \\\"localhost\\\"; var step = 0/1/2/3/4\\\'');\n\
println('you can modify parameters!!!');\n\
println('use default parameters continue...');\n\
main();"
   file.write(functionContext + "\n");
}


// 生成创建domain的函数
function generatCreateDomainFunction(file)
{
   var functionContext = "\n\
function createAllDomains(db, domains)\n\
{\n\
   for (var i = 0; i < domains.length; ++i)\n\
   {\n\
      if (undefined === domains[i].option)\n\
      {\n\
         db.createDomain(domains[i].name, domains[i].groups);\n\
      }\n\
      else\n\
      {\n\
         db.createDomain(domains[i].name, domains[i].groups, domains[i].option);\n\
      }\n\
      println('createDomain ' + domains[i].name + ' success');\n\
   }\n\
}";
   file.write(functionContext + "\n");
   println("generatCreateDomainFunction success");
}

// 生成创建索引的函数
function generatCreateIdxFunction(file)
{
  var functionContext = "\n\
function createAllIndexes(db, indexes)\n\
{\n\
  for (var i = 0; i < indexes.length; ++i)\n\
  {\n\
     var names=indexes[i].name.split('.');\n\
     var cl = db.getCS(names[0]).getCL(names[1]);\n\
     for (var j = 0; j < indexes[i].indexs.length; ++j)\n\
     {\n\
        var item = indexes[i].indexs[j];\n\
        try{\n\
        cl.createIndex(item.name, item.key, item.unique, item.enforced);}\n\
        catch (e) { if ( -247 !=e ) {throw e}}\n\
        println(cl.toString() + 'createIndex(' + item.name + ',' + JSON.stringify(item.key) + ',' + item.unique + ',' + item.enforced + ') success');\n\
     }\n\
  }\n\
}"
  file.write(functionContext + "\n");
  println("generatCreateIdxFunction success");
}


function newDB(hostname, port)
{
   try
   {
      var db = new Sdb(hostname, port);
   }
   catch(e)
   {
      throw "new Sdb(" + hostname + "," + port +") ErrCode: " + e;
   }
   return db;
}

function createFile(filePath)
{
   try
   {
      var file = new File(filePath);
   }
   catch(e)
   {
      throw "new File(" + filePath + ") ErrCode: " + e;
   }
   return file;
}

if ( undefined === coordHostName)
{
   var coordHostName = "localhost";
}

if (undefined === coordPort)
{
   var coordPort = 11810;
}

if ( undefined === catalogHostName)
{
   var catalogHostName = "localhost";
}

if (undefined === catalogPort)
{
   var catalogPort = 11820;
}

if (undefined === generateFilePath)
{
   var generateFilePath = "./copyCluster.js";
}

if (undefined == inDSNames)
{
   var inDSNames = [];
}

if (undefined === inDomainNames)
{
   var inDomainNames = [];
}

if (undefined === inCSNames)
{
   var inCSNames = [];
}

if (undefined === inCLNames)
{
   var inCLNames = [];
}

function main()
{
   try
   {
      var db = newDB(coordHostName, coordPort);
      var catadb = newDB(catalogHostName, catalogPort);
      var file = createFile(generateFilePath);
      var exceptCSSet = [];
      var dsMappingCLSet = [];
      var dsSet = [];

      getDSDefine(catadb, file, dsSet);
      getGroupDefine(db,file);
      getDomainDefine(db,file);
      getCSDefine(db, catadb, file, exceptCSSet, dsSet);
      getCLDefine(db, file, exceptCSSet, dsSet, dsMappingCLSet);
      getIndexDefine(db, file, exceptCSSet, dsMappingCLSet);
      generatePublicVar(file);


      generatDeployFunction(file);
      generatCreateDomainFunction(file);
      generatCreateDSFunction(file);
      generatCreateCSFunction(file);
      generatGetSrcGroup(file);
      generatCreateCLFunction(file);
      generatSplitCL(file);
      generatCreateIdxFunction(file);
      generatAlterAllDomains(file);
      generatmainFunction(file);
   }
   catch(e)
   {
      throw e;
   }
   finally
   {
      if (undefined !== file)
      {
         file.close();
      }

      if (undefined !== db)
      {
         db.close();
      }

      if (undefined !== catadb)
      {
         catadb.close();
      }
   }
}
/*
function readInput()
{
   try
   {
      var cmd = new Cmd();
      var in = cmd.run("read input; echo $input");
      var pos = in.lastIndexOf('\n');

      var inContent = pos !== -1 ? in.substring(0, pos) : in;
      return inContent;
   }
   catch(e)
   {
      throw "readInput failure; ErrCode: " + e;
   }
}
*/
println("Usage:" );
println("     -e 'var coordHostName= \"localhost\";" +
                 "var coordPort=11810;" +
                 "var catalogHostName=\"localhost\" ;" +
                 "var catalogPort=11820 ;" +
                 "[ var inDomainNames=[\"name1\",\"name2\"];" +
                 "var inDSNames = [\"name1\",\"name2\"];" +
                 "var inCSNames = [\"name1\",\"name2\"];" +
                 "var inCLNames = [\"name1\",\"name2\"]; ]" +
                 "var generateFilePath=./copyCluster.js '");
println("you can modify parameters!!!");
println("use default parameters continue...");
main();

