

#include "loadfuncpp.h"
#include "iload.h"

#define GPX 1	//enables polling for events when calling Icon from C++

namespace icall {
//remove interference with icon/src/h/rt.h
#undef D_Null
#undef D_Integer
#undef D_Lrgint
#undef D_Real
#undef D_File
#undef D_Proc
#undef D_External
#undef Fs_Read
#undef Fs_Write
#undef F_Nqual
#undef F_Var

#include "xinterp.cpp"

#ifdef __CYGWIN__
extern "C" {
	typedef int icallfunction(dptr procptr, dptr arglistptr, dptr result);
};
	extern icallfunction *icall2;
#endif //cywgin

};

#ifdef __CYGWIN__

//linking constraints make us do our own linking
class linkicall {
  public:
	linkicall() {	//assign our icall to a function pointer in iload.so
		icall::icall2 = &(icall::icall);
	}
};
static linkicall load;

#else //not cygwin
//call an Icon procedure that always returns a value and never suspends
value Value::call(const value& proc, const value& arglist) {
	value result;
	icall::icall( (icall::dptr)(&proc), (icall::dptr)(&arglist), (icall::dptr)(&result) );
	return result; 
}

#endif //not cywgin

//succeed if graphics are present, fail otherwise
extern "C" int iconx_graphics(value argv[]) {
	argv[0] = nullvalue;
	return SUCCEEDED;
}

//put Icon graphics keywords and functions here
//plus access to the event queue for new I/O events associated with sockets



