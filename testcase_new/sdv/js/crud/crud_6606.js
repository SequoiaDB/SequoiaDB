/******************************************************************************
 * @Description   : seqDB-6606:对date和timestamp类型的数据进行排序
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.07.12
 * @LastEditTime  : 2021.09.14
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_6606";

main( test );

function test ( args )
{
   var varCL = args.testCL;
   varCL.createIndex( "indexA", { a: 1 } );
   insertData( varCL );
   var expData = [];
   expectedDate( expData );
   var actCursor = varCL.find( {}, { a: 1 } ).sort( { a: 1 } );
   commCompareResults( actCursor, expData );
}

function expectedDate ( docs )
{
   docs.push( { 'a': { $date: '2000-04-03' } } );
   docs.push( { 'a': { $date: '2000-04-04' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-11.11.12.123456' } } );
   docs.push( { 'a': { $date: '2001-04-02' } } );
   docs.push( { 'a': { $date: '2001-04-03' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-11.11.12.123456' } } );
   docs.push( { 'a': { $date: '2001-05-03' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-11.11.12.123456' } } );
   docs.push( { 'a': { $date: '2002-04-03' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-11.11.12.123456' } } );
   docs.push( { 'a': { $date: '2002-04-04' } } );
}


function insertData ( cl )
{
   var docs = [];
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-11.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2001-04-03-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-11.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-03-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-11.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2002-04-02-10.11.12.123457' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.12.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.13.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-11.11.12.123456' } } );
   docs.push( { 'a': { $timestamp: '2000-04-04-10.11.12.123457' } } );
   docs.push( { 'a': { $date: '2001-04-03' } } );
   docs.push( { 'a': { $date: '2001-04-02' } } );
   docs.push( { 'a': { $date: '2001-05-03' } } );
   docs.push( { 'a': { $date: '2000-04-03' } } );
   docs.push( { 'a': { $date: '2000-04-04' } } );
   docs.push( { 'a': { $date: '2002-04-03' } } );
   docs.push( { 'a': { $date: '2002-04-04' } } );
   cl.insert( docs );
}