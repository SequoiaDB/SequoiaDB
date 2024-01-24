/************************************
*@Description: decimal data use divide
*@author:      zhaoyu
*@createdate:  2016.4.27
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7776";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data for test
   var doc = [{ a: { $decimal: "92233720368547758071983" } },
   { a: { $decimal: "-922337203685477580719832056" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "-4.7E-360" } },
   { a: { $decimal: "5.7E-400" } },
   { a: { $decimal: "922337203685477580719837895", $precision: [30, 2] } },
   { a: { $decimal: "-922337203685477580754308", $precision: [30, 3] } },
   { a: 1589 }];
   dbcl.insert( doc );

   // divide positive int data,check result
   var expRecsDivPosInt = [{ a: { $decimal: "0.10337394593251813219" } },
   { a: { $decimal: "-1033.7394593251813219" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000" } },
   { a: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000052676780676464560" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000063884606352308083" } },
   { a: { $decimal: "1033.7394593251813219" } },
   { a: { $decimal: "-1.0337394593251813" } },
   { a: { $decimal: "0.0000000000000000000017809234998915358599" } }];
   checkResult( dbcl, {}, { a: { $divide: { $decimal: "892233720368547758084321" } } }, expRecsDivPosInt, { _id: 1 } );

   //divide negative int data,check result
   var expRecsDivNegInt = [{ a: { $decimal: "-0.10337394593251813219" } },
   { a: { $decimal: "1033.7394593251813219" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000052676780676464560" } },
   { a: { $decimal: "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000063884606352308083" } },
   { a: { $decimal: "-1033.7394593251813219" } },
   { a: { $decimal: "1.0337394593251813" } },
   { a: { $decimal: "-0.0000000000000000000017809234998915358599" } }];
   checkResult( dbcl, {}, { a: { $divide: { $decimal: "-892233720368547758081234" } } }, expRecsDivNegInt, { _id: 1 } );

   //divide positive float data,check result
   var expRecsDivPosFloat = [{ a: { $decimal: "-155275623516073666787850168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168.350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168350168" } },
   { a: { $decimal: "1552756235160736667878505144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781.144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781144781" } },
   { a: { $decimal: "0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "7912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912.457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912457912" } },
   { a: { $decimal: "-959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595.959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959595959596" } },
   { a: { $decimal: "-1552756235160736667878514974747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747.474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747474747" } },
   { a: { $decimal: "1552756235160736667936545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454.545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545454545455" } },
   { a: { $decimal: "-2675084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084.175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084175084" } }];
   checkResult( dbcl, {}, { a: { $divide: { $decimal: "-5.94E-700" } } }, expRecsDivPosFloat, { _id: 1 } );

   //divide negative float data,check result
   var expRecsDivNegFloat = [{ a: { $decimal: "1329016143639016686916181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080.6916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469741" } },
   { a: { $decimal: "-13290161436390166869161845187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305.4755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135447" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "-67723342939481268011527377521613832853025936599423631123919308357348703170028818443804034582132564841498559077809798270893371757925072046109.5100864553314121037463976945244956772334293948126801152737752161383285302593659942363112391930835734870317002881844380403458213256484149855907780979827089337175792507204610951008645533141210374639769452449567723342939481268011527377521613832853025936599423631123919308357348703170028818443804034582132564841498559077809798270893371757925072046109510086455331412103746397694524495677233429394812680115273775216138328530259365994236311239193083573487031700288184438040345821325648414985590778097982708934" } },
   { a: { $decimal: "8213256484149855907780979827089337175792507204610951008645533141210374639769452449567723342939481268.0115273775216138328530259365994236311239193083573487031700288184438040345821325648414985590778097982708933717579250720461095100864553314121037463976945244956772334293948126801152737752161383285302593659942363112391930835734870317002881844380403458213256484149855907780979827089337175792507204610951008645533141210374639769452449567723342939481268011527377521613832853025936599423631123919308357348703170028818443804034582132564841498559077809798270893371757925072046109510086455331412103746397694524496" } },
   { a: { $decimal: "13290161436390166869161929322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587.8962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279539" } },
   { a: { $decimal: "-13290161436390166869658616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051.8731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230548" } },
   { a: { $decimal: "22896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291066282420749279538904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953.8904899135446685878962536023054755043227665706051873198847262247838616714697406340057636887608069164265129682997118155619596541786743515850144092219020172910662824207492795389048991354466858789625360230547550432276657060518731988472622478386167146974063400576368876080691642651296829971181556195965417867435158501440922190201729106628242074927953890489913544668587896253602305475504322766570605187319884726224783861671469740634005763688760806916426512968299711815561959654178674351585014409221902017291" } }];
   checkResult( dbcl, {}, { a: { $divide: { $decimal: "6.94E-500" } } }, expRecsDivNegFloat, { _id: 1 } );

   //divide decimal data with precision,check result
   var expRecsDivWithPre = [{ a: { $decimal: "48597776683991652917426.1025" } },
   { a: { $decimal: "-485977766839916529174262108.6464" } },
   { a: { $decimal: "0.00000000000000000000" } },
   { a: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000024764213077612098" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000030033194583487012" } },
   { a: { $decimal: "485977766839916529174265185.2047" } },
   { a: { $decimal: "-485977766839916529192427.4198" } },
   { a: { $decimal: "837.2411612835238948" } }];
   checkResult( dbcl, {}, { a: { $divide: { $decimal: "1.8979", $precision: [5, 4] } } }, expRecsDivWithPre, { _id: 1 } );

   //divide int data,check result
   var expRecsDivInt = [{ a: { $decimal: "7480431497854643801458.48" } },
   { a: { $decimal: "-74804314978546438014584919.38" } },
   { a: { $decimal: "0.00000000000000000000" } },
   { a: { $decimal: "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000038118410381184103812" } },
   { a: { $decimal: "0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000046228710462287104623" } },
   { a: { $decimal: "74804314978546438014585392.94" } },
   { a: { $decimal: "-74804314978546438017381.022" } },
   { a: 128.8726682887267 }];
   checkResult( dbcl, {}, { a: { $divide: 12.33 } }, expRecsDivInt, { _id: 1 } );

   //divide invalid data,check result
   InvalidArgCheck( dbcl, {}, { a: { $divide: { $decimal: "0" } } }, -6 );
   InvalidArgCheck( dbcl, {}, { a: { $divide: "abc" } }, -6 );
   commDropCL( db, COMMCSNAME, clName );
}

