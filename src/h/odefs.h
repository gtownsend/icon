/*
 * Operator definitions.
 *
 * Fields are:
 *    name
 *    number of arguments
 *    string representation
 *    dereference arguments flag: -1 = don't, 0 = do
 */

OpDef(asgn,2,":=",-1)
OpDef(bang,1,"!",-1)
OpDef(cater,2,"||",0)
OpDef(compl,1,"~",0)
OpDef(diff,2,"--",0)
OpDef(divide,2,"/",0)
OpDef(eqv,2,"===",0)
OpDef(inter,2,"**",0)
OpDef(lconcat,2,"|||",0)
OpDef(lexeq,2,"==",0)
OpDef(lexge,2,">>=",0)
OpDef(lexgt,2,">>",0)
OpDef(lexle,2,"<<=",0)
OpDef(lexlt,2,"<<",0)
OpDef(lexne,2,"~==",0)
OpDef(minus,2,"-",0)
OpDef(mod,2,"%",0)
OpDef(mult,2,"*",0)
OpDef(neg,1,"-",0)
OpDef(neqv,2,"~===",0)
OpDef(nonnull,1,"\\",-1)
OpDef(null,1,"/",-1)
OpDef(number,1,"+",0)
OpDef(numeq,2,"=",0)
OpDef(numge,2,">=",0)
OpDef(numgt,2,">",0)
OpDef(numle,2,"<=",0)
OpDef(numlt,2,"<",0)
OpDef(numne,2,"~=",0)
OpDef(plus,2,"+",0)
OpDef(powr,2,"^",0)
OpDef(random,1,"?",-1)
OpDef(rasgn,2,"<-",-1)
OpDef(refresh,1,"^",0)
OpDef(rswap,2,"<->",-1)
OpDef(sect,3,"[:]",-1)
OpDef(size,1,"*",0)
OpDef(subsc,2,"[]",-1)
OpDef(swap,2,":=:",-1)
OpDef(tabmat,1,"=",0)
OpDef(toby,3,"...",0)
OpDef(union,2,"++",0)
OpDef(value,1,".",0)
/* OpDef(llist,1,"[...]",0) */
