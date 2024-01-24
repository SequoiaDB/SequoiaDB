/*******************************************************************************
*@Description : 7528:shell_使用strict格式运算
*@Modify List : 2016-3-28  Ting YU  Init
*******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_7528";

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   testEqual( cl );
   testArithmetic( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function testEqual ( cl )
{
   cl.remove();

   var val = 9007199254740992; //2^53
   var rec = { a: { $numberLong: val.toString() } };
   cl.insert( rec );

   var rc = cl.find();
   var queryVal = rc.current().toObj().a;

   if( queryVal != val )
   {
      throw new Error( "val: " + val + "\nqueryVal: " + JSON.stringify( queryVal ) );
   }

   if( queryVal == ( val - 1 ) )
   {
      throw new Error( "val-1: " + ( val - 1 ) + "\nqueryVal: " + JSON.stringify( queryVal ) );
   }
}

function testArithmetic ( cl )
{
   var val = 9007199254740991;

   cl.remove();
   cl.insert( { a: { $numberLong: val.toString() } } );
   var queryVal = cl.find().current().toObj().a;

   var actVal = ( queryVal + 1 ) / 2;
   var expVal = ( val + 1 ) / 2;
   if( actVal !== expVal )
      throw new Error( "actVal: " + actVal + "\nexpVal: " + expVal );

   var actVal = ( queryVal - 9007199254740990 ) * 2;
   var expVal = ( val - 9007199254740990 ) * 2;
   if( actVal !== expVal )
      throw new Error( "actVal: " + actVal + "\nexpVal: " + expVal );

   var actVal = queryVal % 2;
   var expVal = val % 2;
   if( actVal !== expVal )
      throw new Error( "actVal: " + actVal + "\nexpVal: " + expVal );

   var actVal = Math.abs( queryVal * -0.5 );
   var expVal = Math.abs( val * -0.5 );
   if( actVal !== expVal )
      throw new Error( "actVal: " + actVal + "\nexpVal: " + expVal );
}
