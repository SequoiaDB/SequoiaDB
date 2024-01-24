/******************************************************************************
@description   错误处理类
@author  lyy
******************************************************************************/
function Assert ()
{
   /******************************************************************************
   @parameter
      actual         必填项，实际结果
      expected       必填项，预期结果
      message        选填项，出错时期望打印的 msg
   @usage
      e.g:
         if( actual != expected ){
            throw new Error("erro msg");
         }
         等价于
         assert.equal( actual, expected );
   ******************************************************************************/
   this.equal =
      function( actual, expected, message )
      {
         if( arguments.length < 2 ) { throw new Error( "actual and expected number less than 2" ); }
         message = message || "";

         if( !commCompareObject( actual, expected ) )
         {
            throw new Error( "equal error\nactual:" + JSON.stringify( actual, "", 1 ) + "\nexpected:" + JSON.stringify( expected, "", 1 ) + "\n" + message );
         }
      }

   this.notEqual =
      function( actual, expected, message )
      {
         if( arguments.length < 2 ) { throw new Error( "actual and expected number less than 2" ); }
         message = message || "";

         if( commCompareObject( actual, expected ) )
         {
            throw new Error( "notEqual error\nactual:" + JSON.stringify( actual, "", 1 ) + "\nexpected:" + JSON.stringify( expected, "", 1 ) + "\n" + message );
         }
      }

   /******************************************************************************
   @description    期望失败实际却执行成功
   @author  lyy
   @parameter
      errno         {array|string|number}    :     必填项，错误码
      func          {function}               :     必填项，执行函数
   @usage
      e.g:
         try{
            db.dropCS();
            throw new Error("should error but success");
         }catch(e){
            // SDB_INVALIDARG -6
            if( SDB_INVALIDARG != e ){
               throw e;
            }
         }
         等价于
         assert.tryThrow( SDB_INVALIDARG, function(){
            db.dropCS();
         })
   ******************************************************************************/
   this.tryThrow =
      function( errno, func )
      {
         if( !Array.isArray( errno ) ) { errno = [Number( errno )]; }
         commCheckType( func, "function" );

         try
         {
            func();
            throw new Error( "should error but success" );
         } catch( e )
         {
            var err = e.message || e;
            if( errno.indexOf( Number( err ) ) === -1 )
            {
               throw e;
            }
         }
      }
}
