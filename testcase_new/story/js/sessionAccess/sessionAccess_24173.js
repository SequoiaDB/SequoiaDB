/**********************************************
 * @Description   : seqDB-24173 : PerferedInstance取值为数组，数组中存在多个相同实例ID
 * @Author        : wenjing wang 
 * @CreateTime    : 2020.05.13
 **********************************************/
testConf.skipStandAlone = true;

main(test_setSessionAttr_withDupId)
function test_setSessionAttr_withDupId()
{
   var ids = [1,1] ;
   db.setSessionAttr({"PreferedInstance": ids});
   var obj = db.getSessionAttr().toObj();
   if ( obj.PreferedInstance !== 1 ) 
   {
      throw new Error("real:" + JSON.stringify(obj.PreferedInstance) + "expect:" + 1) ;
   }

   ids = [1,1,2,2];
   db.setSessionAttr({"PreferedInstance": ids});
   var obj = db.getSessionAttr().toObj();
   if ( typeof( obj.PreferedInstance ) != Array && obj.PreferedInstance.length != 2 ) 
   {
      throw new Error("real:" + JSON.stringify(obj.PreferedInstance) + "expect:" + JSON.stringify([1,2]))
   }

   if (( obj.PreferedInstance[0] != 1 || obj.PreferedInstance[1] != 2))
   {
      throw new Error("real:" + JSON.stringify(obj.PreferedInstance) + "expect:" + JSON.stringify([1,2]))
   }
}
