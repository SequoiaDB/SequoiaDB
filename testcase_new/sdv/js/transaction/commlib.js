/**************************************************
*@Description: commlib of transaction
*@author:      wuyan  2015/11/18
               TingYU 2015/11/25
**************************************************/
function insertData ( cl, insertNum )
{
   this.name = "insert";
   this.cl = cl;
   this.number = insertNum;
}
insertData.prototype.exec =
   function()
   {
      println( "--insert date" );
      if( undefined == this.number ) { this.number = 1000; }
      try
      {
         var docs = [];
         for( var i = 0; i < this.number; ++i )
         {
            var no = i;
            var user = "test" + i;
            var phone = 13700000000 + i;
            var time = new Date().getTime();
            var doc = { no: no, customerName: user, phone: phone, openDate: time };
            //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

            docs.push( doc );
         }
         this.cl.insert( docs );
      }
      catch( e )
      {
         throw buildException( "insertData.exec()", e );
      }
   }

function updateData ( cl )
{
   this.name = "update";
   this.cl = cl;
}
updateData.prototype.exec =
   function()
   {
      try
      {
         println( "--update data" );
         this.cl.update( { $inc: { "cardID": 5 }, $set: { "customerName": "updated_name" }, $unset: { "openDate": "" } } );
         this.number = parseInt( this.cl.count() );
      }
      catch( e )
      {
         throw buildException( "updateData.exec()", e );
      }
   }

//randomly delete 5 records
function removeData ( cl, delNum )
{
   if( delNum === undefined ) { delNum = 5; }
   this.name = "remove";
   this.cl = cl;
   this.delNum = delNum;
}
removeData.prototype.exec =
   function()
   {
      try
      {
         println( "--remove data" );
         var totalCnt = parseInt( this.cl.count() );

         var delBeginNo = Math.floor( Math.random() * ( totalCnt - this.delNum ) );
         for( var i = 0; i < this.delNum; i++ )
         {
            this.cl.remove( { "no": delBeginNo++ } );
         }

         this.leftNum = totalCnt - this.delNum;
      }
      catch( e )
      {
         throw buildException( "removeData.exec()", e );
      }
   }

function beginTrans ()
{
   try
   {
      println( "--transBegin" );
      db.transBegin();

   }
   catch( e )
   {
      throw buildException( "beginTrans()", e );
   }
}


function commitTrans ()
{
   try
   {
      println( "--transCommit" );
      db.transCommit();
   }
   catch( e )
   {
      throw buildException( "commitTrans()", e );
   }
}

function rollbackTrans ()
{
   try
   {
      println( "--transRollback" );
      db.transRollback();
   }
   catch( e )
   {
      throw buildException( "rollbackTrans()", e );
   }
}

function execTransaction ()
{
   var number_of_params = arguments.length;
   for( i = 0; i < number_of_params; i++ )
   {
      if( typeof ( arguments[i] ) === "function" )
      {
         arguments[i]();
      }
      else
      {
         arguments[i].exec();
      }
   }
}

//isSuccess: if operate succeed or not
function checkResult ( cl, isSuccess, operate )
{
   var operName = operate.name;
   db.setSessionAttr( { PreferedInstance: "M" } );

   if( isSuccess === true )
   {
      switch( operName )
      {
         case "insert": checkInsert( cl, operate );
            break;
         case "update": checkUpdate( cl, operate );
            break;
         case "remove": checkRemove( cl, operate );
            break;
         default:
            throw buildException( "checkResult()", "invalid parameter operate=" + operName );
      }
   }
   else if( isSuccess === false )
   {
      switch( operName )
      {
         case "insert": checkInsertRollback( cl, operate );
            break;
         case "update": checkUpdateRollback( cl, operate );
            break;
         case "remove": checkRemoveRollback( cl, operate );
            break;
      }
   }
   else
   {
      throw buildException( "checkResult()", "invalid parameter isSuccess=" + isSuccess );
   }
}

function checkInsert ( cl, insert )
{
   var cnt = parseInt( cl.count() );
   if( cnt !== insert.number )
   {
      throw buildException( "checkInsert()", "", "cl.count()", insert.number, cnt );
   }

   var cnt = parseInt( cl.find( { "no": ( insert.number - 1 ) } ).count() );
   if( cnt !== 1 )
   {
      throw buildException( "checkInsert()", "",
         "cl.find({no:" + ( insert.number - 1 ) + "}).count()",
         1, cnt );
   }
}

function checkUpdate ( cl, update )
{
   var cnt = parseInt( cl.find( { "customerName": "updated_name" } ).count() );
   if( cnt !== update.number )
   {
      throw buildException( "checkUpdate()", "",
         "cl.find({customerName:'updated_name'}).count()",
         update.number, cnt );
   }
}

function checkRemove ( cl, remove )
{
   var cnt = parseInt( cl.count() );
   if( cnt !== remove.leftNum )
   {
      throw buildException( "checkRemove()", "", "cl.count()", remove.leftNum, cnt );
   }
}

function checkInsertRollback ( cl, insert )
{
   var cnt = parseInt( cl.count() );
   if( cnt !== 0 )
   {
      throw buildException( "checkInsertRollback()", "", "cl.count()", 0, cnt );
   }
}

function checkUpdateRollback ( cl, update )
{
   var cnt = parseInt( cl.find( { "customerName": "updated_name" } ).count() );
   if( cnt !== 0 )
   {
      throw buildException( "checkUpdateRollback()", "",
         "cl.find({customerName:'updated_name'}).count()",
         0, cnt );
   }
}

function checkRemoveRollback ( cl, remove )
{
   var expTotalCnt = remove.leftNum + remove.delNum;
   var totalCnt = parseInt( cl.count() );
   if( totalCnt !== expTotalCnt )
   {
      throw buildException( "checkRemoveRollback()", "", "cl.count()", expTotalCnt, totalCnt );
   }
}

function readyCL ( csName, clName, option )
{
   println( "--ready cl" );

   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var cl = commCreateCL( db, csName, clName, option, true, false, "create cl in begin" );

   return cl;
}

function clean ( csName, clName )
{
   println( "--clean" );

   commDropCL( db, csName, clName, true, true, "drop cl in clean" );
}

function select2RG ()
{
   var dataRGInfo = commGetGroups( db );
   var rgsName = {};
   rgsName.srcRG = dataRGInfo[0][0]["GroupName"]; //source group
   rgsName.tgtRG = dataRGInfo[1][0]["GroupName"]; //target group

   return rgsName;
}