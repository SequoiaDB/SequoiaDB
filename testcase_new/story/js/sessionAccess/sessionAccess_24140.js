/**********************************************
 * @Description   : seqDB-24140 : PerferedInstance取值为数组时，参数校验 
 * @Author        : wenjing wang 
 * @CreateTime    : 2020.04.29
 **********************************************/
testConf.skipStandAlone = true;

main(test_setSessionAttr)
function test_setSessionAttr()
{
   validObj=[];
   validObj.push([1]);
   validObj.push([255]);
   validObj.push("S")
   validObj.push("-S")
   validObj.push("s")
   validObj.push("-s")
   validObj.push("A")
   validObj.push("-A")
   validObj.push("a")
   validObj.push("-a")
   validObj.push("M")
   validObj.push("-M")
   validObj.push("m")
   validObj.push("-m")
   validObj.push([1,255,'S']);
   validObj.push([1,255,'s']);
   validObj.push([1,255,'A']);
   validObj.push([1,255,'a']);
   validObj.push([1,255,'M']);
   validObj.push([1,255,'m']);

   for ( var i = 0; i < validObj.length; ++i)
   {
      db.setSessionAttr({"PreferedInstance": validObj[i]});
      var obj = db.getSessionAttr().toObj();
      if ( !checkResult(validObj[i], obj.PreferedInstance)) 
      {
         throw new Error("real:" + JSON.stringify(obj.PreferedInstance) + "expect:" + JSON.stringify(validObj[i]))
      }
   }

   invalidObj=[]
   invalidObj.push([0]);
   invalidObj.push([256]);
   invalidObj.push(['O']);
   invalidObj.push(['o']);
   invalidObj.push(['-O']);
   invalidObj.push(['-o']);
   invalidObj.push([0,255]);
   invalidObj.push([1,256]);
   invalidObj.push([1,255,'t']);
   invalidObj.push([1,255,'T']);
   invalidObj.push(['t',1,255]);
   invalidObj.push(['T',1,255]);
   invalidObj.push([1,255,'s','t']);
   invalidObj.push([1,255,'S','T']);
   invalidObj.push(['s',1,255,'t']);
   invalidObj.push(['S',1,255,'T']);
   invalidObj.push([1,255, 'S', 'M']);
   invalidObj.push([1,255,'s', 'm']);
   for ( var i = 0; i < invalidObj.length; ++i)
   {
      assert.tryThrow( [ SDB_INVALIDARG ], function()
      {
         println( JSON.stringify( invalidObj[i] ));
         db.setSessionAttr({"PreferedInstance": invalidObj[i]});
      });
   
      var obj = db.getSessionAttr().toObj();
      if ( !checkResult(validObj[validObj.length -1], obj.PreferedInstance))
      {
         throw new Error("real:" + JSON.stringify(obj.PreferedInstance) + "expect:" + JSON.stringify(validObj[validObj.length -1 ]))
      }
   }
}

function checkResult(expect, real)
{
   if ( expect.length == 1 )
   {
      if (real == expect[0] || ""+real == (""+expect[0]).toUpperCase())
      {
         return true ;
      }
      else
      {
         return false ;
      }
   }

   if (expect.length != real.length)
   {
      return false ;
   }

   for ( var i = 0; i < expect.length; ++i)
   {
      if ( (""+expect[i]).toUpperCase() != ""+real[i] )
      {
         return false ;
      }
   }
   return true;
}

