

/* C++ support for easy extensions to icon via loadfunc,
 * without garbage collection difficulties.
 * Include loadfuncpp.h and link dynamically to
 * this, which contains the necessary glue.
 * See iexample.cpp for typical use.
 * Carl Sturtivant, 2008/3/17
 */

#include <cstdio>
#include <cstring>

#include "loadfuncpp.h"
#include "iload.h"


/*
 * References to the part of loadfuncpp written in Icon
 */

//variables to refer to the Icon procedures in loadfuncpp.icn
static value _loadfuncpp_pathfind;
static value _loadfuncpp_reduce;
static value _loadfuncpp_create;
static value _loadfuncpp_activate;
static value _loadfuncpp_kcollections;
static value _loadfuncpp_kfeatures;
static value _loadfuncpp_kregions;
static value _loadfuncpp_kstorage;
static value _loadfuncpp_function;
static value _loadfuncpp_key;
static value _loadfuncpp_bang;
static value _loadfuncpp_any;
static value _loadfuncpp_many;
static value _loadfuncpp_upto;
static value _loadfuncpp_find;
static value _loadfuncpp_match;
static value _loadfuncpp_bal;
static value _loadfuncpp_move;
static value _loadfuncpp_tab;
static value _loadfuncpp_apply;

static void initialize_procs() { //called below, on load
	_loadfuncpp_pathfind = Value::libproc("_loadfuncpp_pathfind");
	_loadfuncpp_reduce = Value::libproc("_loadfuncpp_reduce");
	_loadfuncpp_create = Value::libproc("_loadfuncpp_create");
	_loadfuncpp_activate = Value::libproc("_loadfuncpp_activate");
	_loadfuncpp_kcollections = Value::libproc("_loadfuncpp_kcollections");
	_loadfuncpp_kfeatures = Value::libproc("_loadfuncpp_kfeatures");
	_loadfuncpp_kregions = Value::libproc("_loadfuncpp_kregions");
	_loadfuncpp_kstorage = Value::libproc("_loadfuncpp_kstorage");
	_loadfuncpp_function = Value::libproc("_loadfuncpp_function");
	_loadfuncpp_key = Value::libproc("_loadfuncpp_key");
	_loadfuncpp_bang = Value::libproc("_loadfuncpp_bang");
	_loadfuncpp_any = Value::libproc("_loadfuncpp_any");
	_loadfuncpp_many = Value::libproc("_loadfuncpp_many");
	_loadfuncpp_upto = Value::libproc("_loadfuncpp_upto");
	_loadfuncpp_find = Value::libproc("_loadfuncpp_find");
	_loadfuncpp_match = Value::libproc("_loadfuncpp_match");
	_loadfuncpp_bal = Value::libproc("_loadfuncpp_bal");
	_loadfuncpp_move = Value::libproc("_loadfuncpp_move");
	_loadfuncpp_tab = Value::libproc("_loadfuncpp_tab");
	_loadfuncpp_apply = Value::libproc("_loadfuncpp_apply");
}

//callbacks to Icon for generative keywords and functions
static int K_collections(value* argv) {
	argv[0] = _loadfuncpp_kcollections.apply(Value::list());
	return SUCCEEDED;
}

static int K_features(value* argv) {
	argv[0] = _loadfuncpp_kfeatures.apply(Value::list());
	return SUCCEEDED;
}

static int K_regions(value* argv) {
	argv[0] = _loadfuncpp_kregions.apply(Value::list());
	return SUCCEEDED;
}

static int K_storage(value* argv) {
	argv[0] = _loadfuncpp_kstorage.apply(Value::list());
	return SUCCEEDED;
}

static int Z_function(value* argv) {
	argv[0] = _loadfuncpp_function.apply(Value::list());
	return SUCCEEDED;
}

static int Z_key(value* argv) {
	value arg(1,argv);
	argv[0] = _loadfuncpp_key.apply(arg);
	return SUCCEEDED;
}

static int Z_any(value* argv) {
	value arg(4,argv);
	argv[0] = _loadfuncpp_any.apply(arg);
	return SUCCEEDED;
}

static int Z_many(value* argv) {
	value arg(4,argv);
	argv[0] = _loadfuncpp_many.apply(arg);
	return SUCCEEDED;
}

static int Z_upto(value* argv) {
	value arg(4,argv);
	argv[0] = _loadfuncpp_upto.apply(arg);
	return SUCCEEDED;
}

static int Z_find(value* argv) {
	value arg(4,argv);
	argv[0] = _loadfuncpp_find.apply(arg);
	return SUCCEEDED;
}

static int Z_match(value* argv) {
	value arg(4,argv);
	argv[0] = _loadfuncpp_match.apply(arg);
	return SUCCEEDED;
}

static int Z_bal(value* argv) {
	value arg(6,argv);
	argv[0] = _loadfuncpp_bal.apply(arg);
	return SUCCEEDED;
}

static int Z_move(value* argv) {
	value arg(1,argv);
	argv[0] = _loadfuncpp_move.apply(arg);
	return SUCCEEDED;
}

static int Z_tab(value* argv) {
	value arg(1,argv);
	argv[0] = _loadfuncpp_tab.apply(arg);
	return SUCCEEDED;
}



/*
 * Keywords and their initialization
 */

namespace Icon {
//all non-graphics keywords excepting &fail, &cset (name collision with function cset)
keyword allocated;
keyword ascii;
keyword clock;
keyword collections;
keyword current;
keyword date;
keyword dateline;
keyword digits;
keyword dump;
keyword e;
keyword error;
keyword errornumber;
keyword errortext;
keyword errorvalue;
keyword errout;
keyword features;
keyword file;
keyword host;
keyword input;
keyword lcase;
keyword letters;
keyword level;
keyword line;
keyword main;
keyword null;
keyword output;
keyword phi;
keyword pi;
keyword pos;
keyword progname;
keyword random;
keyword regions;
keyword source;
keyword storage;
keyword subject;
keyword time;
keyword trace;
keyword ucase;
keyword version;
}; //namespace Icon


static void initialize_keywords() {
    Icon::allocated.f = Kallocated;
    Icon::ascii.f = Kascii;
    Icon::clock.f = Kclock;
    Icon::collections.f = K_collections;	//generative: K_
    Icon::current.f = Kcurrent;
    Icon::date.f = Kdate;
    Icon::dateline.f = Kdateline;
    Icon::digits.f = Kdigits;
    Icon::dump.f = Kdump;
    Icon::e.f = Ke;
    Icon::error.f = Kerror;
    Icon::errornumber.f = Kerrornumber;
    Icon::errortext.f = Kerrortext;
    Icon::errorvalue.f = Kerrorvalue;
    Icon::errout.f = Kerrout;
    Icon::features.f = K_features;	//generative: K_
    Icon::file.f = Kfile;
    Icon::host.f = Khost;
    Icon::input.f = Kinput;
    Icon::lcase.f = Klcase;
    Icon::letters.f = Kletters;
    Icon::level.f = Klevel;
    Icon::line.f = Kline;
    Icon::main.f = Kmain;
    Icon::null.f = Knull;
    Icon::output.f = Koutput;
    Icon::phi.f = Kphi;
    Icon::pi.f = Kpi;
    Icon::pos.f = Kpos;
    Icon::progname.f = Kprogname;
    Icon::random.f = Krandom;
    Icon::regions.f = K_regions;	//generative: K_
    Icon::source.f = Ksource;
    Icon::storage.f = K_storage;	//generative: K_
    Icon::subject.f = Ksubject;
    Icon::time.f = Ktime;
    Icon::trace.f = Ktrace;
    Icon::ucase.f = Kucase;
    Icon::version.f = Kversion;
}

safe keyword::operator&() {
	value result;
	safecall_0(*f, result);
	return result;
}

/*
 * Implementation of the value class.
 */

const value nullstring(NullString);
const value nullvalue; //statically initialized by default to &null
const value nullchar(NullChar);
const value illegal(Illegal);

value::value() {
//default initialization is to &null
    dword = D_Null;
    vword = 0;
}

value::value(special_value sv, const char *text) {
	switch( sv ) {
		case NullString:
    		dword = 0;
    		vword = (long)"";
    		break;
    	case StringLiteral:
    		dword = strlen(text);
    		vword = (long)text;
    		break;
    	case NewString:
    		dword = strlen(text);
    		vword = (long)alcstr((char*)text, dword);
    		break;
		case NullChar:
			dword = 1;
			vword = (long)"\0";
			break;
		case Illegal:
			dword = D_Illegal;
			vword = 0;
			break;
		default:
    		dword = D_Null;
    		vword = 0;
	}
}

value::value(int argc, value* argv) { 	//assumes these are passed in from Icon
    safe argv0 = argv[0];               //which guarantees their GC safety
	Ollist(argc, argv);
	*this = argv[0];
	argv[0] = argv0;
}

value::value(int n) {
    dword = D_Integer;
    vword = n;
}

value::value(long n) {
    dword = D_Integer;
    vword = n;
}

value::value(float x) {
    dword = D_Real;
    vword = (long)alcreal(x);
}

value::value(double x) {
    dword = D_Real;
    vword = (long)alcreal(x);
}

value::value(char* s) {
    dword = strlen(s);
    vword = (long)alcstr(s, dword);
}

value::value(const char* s) {
    dword = strlen(s);
    vword = (long)alcstr((char*)s, dword);
}

value::value(const char* s, long len) {
	dword = len;
	vword = (long)alcstr((char*)s, dword);
}

value::value(proc_block& pb) {
	dword = D_Proc;
	vword = (long)&pb;
}

value::value(proc_block* pbp) {
	dword = D_Proc;
	vword = (long)pbp;
}

value::value(external* ep) {
	char* ptr = (char*)ep - sizeof(external_block)/sizeof(char);
	dword = D_External;
	vword = (long)ptr;
}

value::operator int() {
    if( this->type() != Integer )
        syserror("loadfuncpp: int cannot be produced from non-Integer");
    return vword;
}

value::operator long() {
    if( this->type() != Integer )
        syserror("loadfuncpp: long cannot be produced from non-Integer");
    return vword;
}

value::operator float() {
	if( this->type() != Real )
	    syserror("loadfuncpp: double cannot be produced from non-Real");
	return getdbl(this);
}

value::operator double() {
	if( this->type() != Real )
	    syserror("loadfuncpp: double cannot be produced from non-Real");
	return getdbl(this);
}

value::operator char*() {
    if( this->type() != String )
        syserror("loadfuncpp: char* cannot be produced from non-String");
    return (char*)vword;
}

value::operator external*() {
	if( dword != D_External ) return 0;	//too ruthless
	return (external*)((external_block*)vword + 1);
}

value::operator proc_block*() const {
	if( dword != D_Proc ) return 0; //too ruthless
	return (proc_block*)vword;
}

void value::dump() const {
	fprintf(stderr, "\n%lx\n%lx\n", dword, vword);
	fflush(stderr);
}

bool value::operator==(const value& v) const {
	return dword==v.dword && vword==v.vword;
}

value& value::dereference() {
    deref(this, this); //dereference in place
    return *this;
}

value value::intify() { //integer representation of vword pointer
    switch( this->type() ) {
        default:
            return vword;
        case Null: case Integer: case Real:
            return nullvalue;
    }
}

bool value::isNull() {
    return (dword & TypeMask) == T_Null;
}

bool value::notNull() {
    return (dword & TypeMask) != T_Null;
}

value value::size() const {
	value result;
	safecall_1(&Osize, result, *this);
	return result;
}

kind value::type() {
    if( !( dword & F_Nqual ) ) return String;
    if( dword & F_Var ) return Variable;
    return kind(dword & TypeMask);
}

bool value::toCset() {
    return safecall_1(&Zcset, *this, *this) == SUCCEEDED;
}

bool value::toInteger() {
    return safecall_1(&Zinteger, *this, *this) == SUCCEEDED;
}

bool value::toReal() {
    return safecall_1(&Zreal, *this, *this) == SUCCEEDED;
}

bool value::toNumeric() {
    return safecall_1(&Znumeric, *this, *this) == SUCCEEDED;
}

bool value::toString() {
    return safecall_1(&Zstring, *this, *this) == SUCCEEDED;
}

value value::subscript(const value& v) const {
	value result;
	safecall_2(&Osubsc, result, *this, v);
	return result;
}

value& value::assign(const value& v) {
    if( dword & F_Var ) //lhs value is an Icon 'Variable'
    	safecall_2(&Oasgn, *this, *this, v);
    else {
    	dword = v.dword;
    	vword = v.vword;
    	deref(this,this); //in case rhs is an Icon 'Variable'
    }
    return *this;
}

value value::put(value x) {
	value result;
	safecall_v2(&Zput, result, *this, x);
	return result;
}

value value::push(value x) {
	value result;
	safecall_v2(&Zpush, result, *this, x);
	return result;
}

void value::printimage() const {
    value result;
    safecall_1(&Zimage, result, *this);
    safecall_v1(&Zwrites, result, result);
}

int value::compare(const value& x) const {
	return anycmp(this, &x);
}

value value::negative() const {
	value result;
	if( safecall_1(&Oneg, result, *this) == FAILED )
		return nullvalue;
	return result;
}

value value::complement() const {
	value result;
	if( safecall_1(&Ocompl, result, *this) == FAILED )
		return nullvalue;
	return result;
}

value value::refreshed() const {
	value result;
	if( safecall_1(&Orefresh, result, *this) == FAILED )
		return nullvalue;
	return result;
}

value value::random() const {
	value result;
	if( safecall_1(&Orandom, result, *this) == FAILED )
		return nullvalue;
	return result;
}

value value::plus(const value& x) const {
	value result;
	if( safecall_2(&Oplus, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::minus(const value& x) const {
	value result;
	if( safecall_2(&Ominus, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::multiply(const value& x) const {
	value result;
	if( safecall_2(&Omult, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::divide(const value& x) const {
	value result;
	if( safecall_2(&Odivide, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::remainder(const value& x) const {
	value result;
	if( safecall_2(&Omod, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::power(const value& x) const {
	value result;
	if( safecall_2(&Opowr, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::union_(const value& x) const {
	value result;
	if( safecall_2(&Ounion, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::intersection(const value& x) const {
	value result;
	if( safecall_2(&Ointer, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::difference(const value& x) const {
	value result;
	if( safecall_2(&Odiff, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::concatenate(const value& x) const {
	value result;
	if( safecall_2(&Ocater, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::listconcatenate(const value& x) const {
	value result;
	if( safecall_2(&Olconcat, result, *this, x) == FAILED )
		return nullvalue;
	return result;
}

value value::slice(const value& x, const value& y) const {
	value result;
	if( safecall_3(&Osect, result, *this, x, y) == FAILED )
		return nullvalue;
	return result;
}

value& value::swap(value& x) {
	safecall_2(&Oswap, *this, *this, x);
	return *this;
}

value value::activate(const value& x) const {
	value arg = Value::pair(*this, x);
	return _loadfuncpp_activate.apply(arg);
}

value value::apply(const value& x) const {
	return Value::call(*this, x);
}



/*
 * Implementation of the generator class
 */

int generator::generate(value argv[]) {
//suspend all values generated and return the eventual signal
	int signal = FAILED;
	while( this->hasNext() && signal == FAILED ) {
		argv[0] = this->giveNext();
		signal = interp(SUSPEND, argv);
	}
	return signal;
}

bool generator::hasNext() { return false; } //empty sequence for the root class
value generator::giveNext() { return nullvalue; }



/*
 * Implementation of class iterate
 */

class wrap: public external {	//an iterate object as Icon data
  public:
	iterate* data;
	wrap(iterate* ip): data(ip) {}
};

extern "C" int update_iteration(value argv[]) {
	external* ep = argv[1];
	iterate* ip = ((wrap*)ep)->data;
	argv[0] = nullvalue;
	if( ip->wantNext(argv[2]) ) {
		ip->takeNext(argv[2]);
		return SUCCEEDED;
	}
	else return FAILED;
}

static proc_block updatepb("update_iteration", &update_iteration, 2);
static value update(updatepb);

void iterate::every(const value& g, const value& arg) {
	value nullary(new wrap(this));
	variadic v(nullary);
	_loadfuncpp_reduce.apply((v,update,g,arg));
}

void iterate::bang(const value& x) {
	value nullary(new wrap(this));
	variadic v(nullary);
	_loadfuncpp_bang.apply((v,update,x));
}

bool iterate::wantNext(const value& v) { return true; } //use whole sequence
void iterate::takeNext(const value& v) {}



/*
 * Implementation of the safe_variable class
 */
safe_variable::safe_variable() : val() {};

safe_variable::safe_variable(int n) : val(n) {};

safe_variable::safe_variable(long n) : val(n) {};

safe_variable::safe_variable(double x) : val(x) {};

safe_variable::safe_variable(value v) : val(v) {};

safe_variable::safe_variable(proc_block& pb) : val(pb) {};

safe_variable::safe_variable(proc_block* pbp) : val(pbp) {};

safe_variable::safe_variable(int argc, value* argv) : val(argc, argv) {};

inline void safe_variable::push(safe_variable*& tendlist, int numvalues) {
	previous = tendlist;
	num = numvalues;
	tendlist = this;
}

inline void safe_variable::pop(safe_variable*& tendlist) {
    if(  tendlist == this ) { //we are at the head of the tend list
        tendlist = tendlist->previous; //pop us off
        return;
    }
#if 0
    if( tendlist == tend ) //warning is for safe tend list only
    {
        fprintf(stderr, "loadfuncpp warning: pop needed from interior of tended list\n");
        fflush(stderr);
    }
#endif
    safe_variable *last = 0, *current = tendlist;
    do { //search tendlist
        last = current;
        current = current->previous;
    } while( current != this && current != 0);
    if( current == 0 )
    	syserror("loadfuncpp bug: failed to find variable on tended list so as to remove it.");
    last->previous = current->previous; //slice us out
}



/*
 * Implementation of the variadic class (variable length argument list)
 */

variadic::variadic(int n) {
	value v(n);
	val = Value::list(1, v);
	push(global_tend);
}

variadic::variadic(long n) {
	value v(n);
	val = Value::list(1, v);
	push(global_tend);
}

variadic::variadic(float x) {
	value v(x);
	val = Value::list(1, v);
	push(global_tend);
}

variadic::variadic(double x) {
	value v(x);
	val = Value::list(1, v);
	push(global_tend);
}

variadic::variadic(char* s) {
	value v(s);
	val = Value::list(1, v);
	push(global_tend);
}
 
variadic::variadic(value v) {
	val = Value::list(1, v);
	push(global_tend);
}
 
variadic::variadic(const safe& x) {
	val = Value::list(1, x.val);
	push(global_tend);
}

variadic::variadic(const safe& x, const safe& y) {
	val = Value::pair(x, y);
	push(global_tend);
}

variadic& variadic::operator,(const safe& x) {
	val.put(x.val);
	return *this;
}

variadic::operator value() {
	return val;
}

variadic::~variadic() { pop(global_tend); }


/*
 * Implementation of the safe class
 */

safe::safe() : safe_variable() { push(global_tend); }

safe::safe(const safe& x) : safe_variable(x.val) { push(global_tend); }

safe::safe(int n) : safe_variable(n) { push(global_tend); }

safe::safe(long n) : safe_variable(n) { push(global_tend); }

safe::safe(float x) : safe_variable(x) { push(global_tend); }

safe::safe(double x) : safe_variable(x) { push(global_tend); }

safe::safe(char* s) : safe_variable(s) { push(global_tend); }

safe::safe(const value& v) : safe_variable(v) { push(global_tend); }

safe::safe(const variadic& v) : safe_variable(v) { push(global_tend); }

safe::safe(proc_block& pb) : safe_variable(pb) { push(global_tend); }

safe::safe(proc_block* pbp) : safe_variable(pbp) { push(global_tend); }

safe::safe(int argc, value* argv) : safe_variable(argc, argv) { push(global_tend); }

safe::~safe() { pop(global_tend); }

safe& safe::operator=(const safe& x) {
	val.assign(x.val); //Icon style assignment
    return *this;
}

safe& safe::operator^=(const safe& x) {
	*this = *this ^ x;
    return *this;
}

safe& safe::operator+=(const safe& x) {
	*this = *this + x;
    return *this;
}

safe& safe::operator-=(const safe& x) {
	*this = *this - x;
    return *this;
}

safe& safe::operator*=(const safe& x) {
	*this = *this * x;
    return *this;
}

safe& safe::operator/=(const safe& x) {
	*this = *this / x;
    return *this;
}

safe& safe::operator%=(const safe& x) {
	*this = *this % x;
    return *this;
}

safe& safe::operator&=(const safe& x) {
	*this = *this & x;
    return *this;
}

safe& safe::operator|=(const safe& x) {
	*this = *this | x;
    return *this;
}

safe& safe::operator++() {
    *this -= 1;
	return *this;
}

safe& safe::operator--() {
    *this += 1;
	return *this;
}

safe safe::operator++(int) {
    safe temp(*this);
    *this += 1;
	return temp;
}

safe safe::operator--(int) {
    safe temp(*this);
    *this -= 1;
	return temp;
}

safe::operator value() const {
	return val;	//low-level copy
}

safe safe::operator() () {
	value empty = Value::list();
	return this->apply(empty);
}

safe safe::operator() (const safe& x) {
	value singleton = Value::list(1, x);
	return this->apply(singleton);
}

safe safe::operator()(const safe& x1, const safe& x2,
                        const safe& x3, const safe& x4,
                        const safe& x5, const safe& x6,
                        const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return this->apply( (x1,x2) );
	if( x4.isIllegal() )
		return this->apply( (x1,x2,x3) );
	if( x5.isIllegal() )
		return this->apply( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return this->apply( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return this->apply( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return this->apply( (x1,x2,x3,x4,x5,x6,x7) );		
	return this->apply( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe safe::operator[](const safe& x) {
	return val.subscript(x.val);
}

safe operator*(const safe& x){
	return x.val.size();
}

safe operator-(const safe& x){
	return x.val.negative();
}

safe operator~(const safe& x){ //set complement
	return x.val.complement();
}

safe operator+(const safe& x, const safe& y){
	return x.val.plus(y.val);
}

safe operator-(const safe& x, const safe& y){
	return x.val.minus(y.val);
}

safe operator*(const safe& x, const safe& y){
	return x.val.multiply(y.val);
}

safe operator/(const safe& x, const safe& y){
	return x.val.divide(y.val);
}

safe operator%(const safe& x, const safe& y){
	return x.val.remainder(y.val);
}

safe operator^(const safe& x, const safe& y){     //exponentiation
	return x.val.power(y.val);
}

safe operator|(const safe& x, const safe& y){     //union
	return x.val.union_(y.val);
}

safe operator&(const safe& x, const safe& y){     //intersection
	return x.val.intersection(y.val);
}

safe operator&&(const safe& x, const safe& y){    //set or cset difference
	return x.val.difference(y.val);
}

safe operator||(const safe& x, const safe& y){    //string concatenation
	return x.val.concatenate(y.val);
}

bool operator<(const safe& x, const safe& y){
	return x.val.compare(y.val) < 0;
}

bool operator>(const safe& x, const safe& y){
	return x.val.compare(y.val) > 0;
}

bool operator<=(const safe& x, const safe& y){
	return x.val.compare(y.val) <= 0;
}

bool operator>=(const safe& x, const safe& y){
	return x.val.compare(y.val) >= 0;
}

bool operator==(const safe& x, const safe& y){
	return x.val.compare(y.val) == 0;
}

bool operator!=(const safe& x, const safe& y){
	return x.val.compare(y.val) != 0;
}

variadic operator,(const safe& x, const safe& y){  //variadic argument list construction
	return variadic(x.val, y.val);
}

safe safe::slice(const safe& y, const safe& z){      // x[y:z]
	return this->val.slice(y, z);
}

safe safe::apply(const safe& y){          // x ! y
    safe result;
	result = _loadfuncpp_apply.apply( (this->val, y.val) );
	return result;
}

safe safe::listcat(const safe& y){        // x ||| y
	value x(*this);
	return x.listconcatenate(y);
}

safe& safe::swap(safe& y){                // x :=: y
	value& x(this->val);
	value& yv(y.val);
	x.swap(yv);
	return *this;
}

safe safe::create(){                  // create !x
	return _loadfuncpp_create.apply(Value::list(1, *this));
}

safe safe::create(const safe& y){        // create x!y
	return _loadfuncpp_create.apply(Value::pair(*this, y));
}

safe safe::activate(const safe& y){  // y@x
	return _loadfuncpp_activate.apply(Value::pair(*this, y));
}

safe safe::refresh(){                  // ^x
	return this->val.refreshed();
}

safe safe::random(){                       // ?x
	return this->val.random();
}

safe safe::dereference(){               // .x
	value var(this->val);
	var.dereference();
	return var;
}

bool safe::isIllegal() const {
	return this->val == illegal;
}



/*
 * iconx callback support
 */

inline int safecall_0(iconfunc *F, value& out) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[1];
    } vars;
    vars.stack[0] = nullvalue;
    vars.tend.push(tend,2);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_1(iconfunc *F, value& out, const value& x1) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[2];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.tend.push(tend,3);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_2(iconfunc *F, value& out, const value& x1, const value& x2) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[3];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.tend.push(tend,4);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_3(iconfunc *F, value& out, const value& x1, const value& x2, const value& x3) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[4];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.stack[3] = x3;
    vars.tend.push(tend,5);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_4(iconfunc *F, value& out, const value& x1, const value& x2, const value& x3, const value& x4) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[5];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.stack[3] = x3;
    vars.stack[4] = x4;
    vars.tend.push(tend,6);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_5(iconfunc *F, value& out, const value& x1, const value& x2, const value& x3, const value& x4, const value& x5) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[6];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.stack[3] = x3;
    vars.stack[4] = x4;
    vars.stack[5] = x5;
    vars.tend.push(tend,7);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_6(iconfunc *F, value& out, const value& x1, const value& x2, const value& x3, const value& x4, const value& x5, const value& x6) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[7];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.stack[3] = x3;
    vars.stack[4] = x4;
    vars.stack[5] = x5;
    vars.stack[6] = x6;
    vars.tend.push(tend,8);
    int result = F(vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_v0(iconfvbl *F, value& out) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[1];
    } vars;
    vars.stack[0] = nullvalue;
    vars.tend.push(tend,2);
    int result = F(0, vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_v1(iconfvbl *F, value& out, const value& x1) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[2];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1]= x1;
    vars.tend.push(tend,3);
    int result = F(1, vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_v2(iconfvbl *F, value& out, const value& x1, const value& x2) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[3];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.tend.push(tend,4);
    int result = F(2, vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_v3(iconfvbl *F, value& out, const value& x1, const value& x2, const value& x3) {
    struct {
        safe_variable tend; //contains an additional unused value
        value stack[4];
    } vars;
    vars.stack[0] = nullvalue;
    vars.stack[1] = x1;
    vars.stack[2] = x2;
    vars.stack[3] = x3;
    vars.tend.push(tend,5);
    int result = F(3, vars.stack);
    if( result == SUCCEEDED )
        out = vars.stack[0];
    vars.tend.pop(tend);
    return result;
}

inline int safecall_vbl(iconfvbl* F, safe& out, const variadic& arg) {
	int argc = arg.val.size();
	//C++ makes allocating trailing variable sized arrays
	//inside structs difficult, so do this C-style
	safe_variable* pvars = (safe_variable*)malloc(sizeof(safe_variable)+(argc+1)*sizeof(value));
	value* stack = (value*)(pvars + 1); //get past the safe_variable at the start of the block
	stack[0] = nullvalue;
	for(int i=1; i<=argc; ++i)
		stack[i] = arg.val.subscript(i).dereference();
	pvars->push(tend, argc+2);
	int result = F(argc, stack);
	if( result == SUCCEEDED )
		out = stack[0];
	pvars->pop(tend);
	free(pvars);
}



/*
 * Procedure related
 */

//Icon procedure block: used to make new Icon procedures as values to return

proc_block::proc_block(value procname, iconfvbl *function) {
   	init(procname);
   	nparam = -1; //a variable number of arguments
   	entryp = function;
}

proc_block::proc_block(value procname,  iconfunc *function, int arity) {
   	init(procname);
   	nparam = arity;
   	entryp = (iconfvbl*)function;
}

proc_block::proc_block(value procname,  iconfvbl *function, int arity) {
   	init(procname);
   	nparam = -1; //a variable number of arguments
   	entryp = function;
}

long proc_block::extra_bytes = 0;

extern long extl_ser; //serial number counter for alcexternal

static void* alcproc(long nbytes) {
	proc_block* pbp = (proc_block*)alcexternal(nbytes, 0, 0);	//a hack for now
	--extl_ser;
	pbp->title = T_Proc;
	pbp->blksize = nbytes;
	return (void*)pbp;
}

void* proc_block::operator new(size_t nbytes) { //allocated in Icon's block region
	return alcproc(nbytes + extra_bytes);
}

void proc_block::operator delete(void*) {
	return;						//do nothing
}

proc_block::proc_block(proc_block* pbp) {
	*this = *pbp; //copy the C++ legitimate part
}

proc_block* proc_block::bind(proc_block* pbp, const value& rec) {
	extra_bytes = pbp->blksize - sizeof(proc_block) + sizeof(value); //one more slot
	proc_block* ans = new proc_block(pbp); // copies the C++ legitimate part
	ans->blksize = sizeof(proc_block) + extra_bytes;
	extra_bytes = 0;
    int nsafe = ans->ndynam + ans->nparam;
    for( int pos=1; pos<nsafe; pos++)   //copy the remainder
		ans->lnames[pos] = pbp->lnames[pos];
    ans->lnames[nsafe] = rec;   //set the last array slot to rec
	ans->pname = "bound to record";	//improve this to use the proc name and rec image
	return ans;
}

extern "C" int bindself(value argv[]) {
	if( argv[1].type() != Procedure ||
		argv[2].type() != Record ) {
		argv[0] = nullvalue;
		return FAILED;
	}
	argv[0] = proc_block::bind(argv[1], argv[2]);
	return SUCCEEDED;
}



/*
 * External values related
 */

extern "C" { //these call virtual functions, so only one function list needed
	static int extcmp(int argc, value argv[]) {
		external *ep = argv[1], *ep2 = argv[2];
		argv[0] = ep->compare(ep2);
		return 0;
	}
	static int extcopy(int argc, value argv[]) {
		external* ep = argv[1];
		argv[0] = ep->copy();
		return 0;
	}
	static int extname(int argc, value argv[]) {
		external* ep = argv[1];
		argv[0] = ep->name();
		return 0;
	}
	static int extimage(int argc, value argv[]) {
		external* ep = argv[1];
		argv[0] = ep->image();
		return 0;
	}
}; //end extern "C"

static void initialize_ftable(); //just below

static struct external_ftable { //C callback table for all C++ made external values
	iconfvbl* cmp;
	iconfvbl* copy;
	iconfvbl* name;
	iconfvbl* image;
	external_ftable() { initialize_ftable(); }
} ftable;

static void initialize_ftable() {
	ftable.cmp = &extcmp;
	ftable.copy = &extcopy;
	ftable.name = &extname;
	ftable.image = &extimage;
}

long external_block::extra_bytes;	//silent extra parameter to external_block::new

static void* external_block::operator new(size_t nbytes) {
	return alcexternal(nbytes + extra_bytes, &ftable, 0); //extra_bytes for C++ external
}

static void external_block::operator delete(void* p) {
	return;	//don't delete
}

external_block::external_block() {
	//val = (external*)((long*)&val + 1); //add a trashable pointer to the (to be appended) external
	val = 0;
}

external_block* external::blockptr; //silent extra result of external::new for external()

static void* external::operator new(size_t nbytes) {
	external_block::extra_bytes = nbytes; //pass our requirements to external_block::new
	blockptr = new external_block(); //with extra_bytes; pass our requirements to external()
	char* ptr = (char*)blockptr + sizeof(external_block)/sizeof(char); //beginning of extra_bytes
	return (void*)ptr; //where the external will be appended
}

static void external::operator delete(void* p) {
	return; //don't delete
}

external::external() {
	id = blockptr->id; //set by new
}

external* external::copy() {
	return this;
}

value external::image() { //need new string every time!
	char sbuf[100];
	long vptr = *((long*)this);
	sprintf(sbuf, "external_%ld(%lX)", id, vptr);
	return value(NewString, sbuf);
}

value external::name() {
	return value(StringLiteral, "external");
}

long external::compare(external* ep) {
	return this->id - ep->id;
}

bool value::isExternal(const value& type) { //needs external_block declaration
	if( dword != D_External ) return false;
	value result;
	external_block* ebp = (external_block*)vword;
	iconfvbl* name = (ebp->funcs)->name;
	value stack[2];
	stack[1] = *this;
	name(1, stack);
	return !stack[0].compare(type);
}



/*
 * Startup code (on load)
 */

//new variant of loadfunc sidestepping loadfunc's glue, a three argument function

extern "C" int loadfuncpp(value argv[]) {	//three arguments
	if( argv[3].isNull() ) argv[3]=-1;
	//assumption: a path is specified iff a slash or backslash is in the filename,	
	if( argv[1].toString() ) {
		safe fname(argv[1]), fullname;
		int ispath = value( *(Icon::cset(fname) & Icon::cset((char*)"\\/")) );
		if( !ispath ) { //search FPATH for the file
			fullname = _loadfuncpp_pathfind.apply((fname, Icon::getenv((char*)"FPATH")));
			if( fullname == nullvalue ) {
				Icon::runerr(216, argv[1]);
				return FAILED;
			}
			argv[1] = value(fullname);
		}
	}
	return rawloadfuncpp(argv);
}

static void replace_loadfunc() {
	static proc_block pb("loadfuncpp", loadfuncpp, 3); //three arguments
	value proc(pb), var = Value::variable("loadfunc");
	var.assign(proc);
}

//set up a tend list for global variables on the tail of &main's
struct safe_tend { //struct with isomorphic data footprint to a safe_variable
	safe_variable *previous;
	int num;
	value val;
} sentinel;

safe_variable*& global_tend = sentinel.previous;

static void add_to_end(safe_variable*& tend_list) {
	safe_tend *last = 0, *current = (safe_tend*)tend_list;
	while( current != 0 ) {
		last = current;
		current = (safe_tend*)(current->previous);
	}
	if( last == 0 ) tend_list = (safe_variable*)&sentinel;
	else last->previous = (safe_variable*)&sentinel;
}

static void make_global_tend_list() {
	sentinel.previous = 0;
	sentinel.num = 1;
	sentinel.val = nullvalue;
	if( k_current == k_main ) add_to_end(tend); //add to the active tend list
	else add_to_end( ((coexp_block*)(long(k_main)))->es_tend   );
}

struct load {
	load() {	//startup code here
		replace_loadfunc();	//store loadfuncpp in global loadfunc temporarily
		make_global_tend_list();
		initialize_procs();
		initialize_keywords();
//fprintf(stderr, "\nStartup code ran!\n");fflush(stderr);
	}
};
static load startup; //force static initialization so as to run startup code



/*
 * Useful helper functions
 */

namespace Value {

value pair(value x, value y) {
	value newlist;
	if( safecall_v2(&Ollist, newlist, x, y) == FAILED )
		return nullvalue;
	return newlist;
}

value list(value n, value init) {
    value newlist;
    if( safecall_2(&Zlist, newlist, n, init) == FAILED )
        return nullvalue;
    return newlist;
}

void runerr(value n, value x) {
    value v;
   	safecall_v2(&Zrunerr, v, n, x);
}

value set(value list) {
    value newset;
    if(  safecall_1(&Zset, newset, list) == FAILED )
        return nullvalue;
    return newset;
}

value table(value init) {
    value newtable;
    if(  safecall_1(&Ztable, newtable, init) == FAILED )
        return nullvalue;
    return newtable;
}

value variable(value name) {
    value var;
    if(  safecall_1(&Zvariable, var, name) == FAILED )
        return nullvalue;
    return var;
}

value proc(value name, value arity) {
	value procedure;
	if( safecall_2(&Zproc, procedure, name, arity) == FAILED )
		return nullvalue;
	return procedure;
}

value libproc(value name, value arity) {
	value procedure;
	if( safecall_2(&Zproc, procedure, name, arity) == SUCCEEDED )
		return procedure;
	syserror("loadfuncpp: unable to find required Icon procedure through 'link loadfunc'\n");
	return nullvalue;
}

}; //namespace Value



/*
 * Built-in Icon functions
 */
namespace Icon {
safe abs(const safe& x1) {
	value result;
	safecall_1(&Zabs, result, x1);
	return result;
}

safe acos(const safe& x1) {
	value result;
	safecall_1(&Zacos, result, x1);
	return result;
}

safe args(const safe& x1) {
	value result;
	safecall_1(&Zargs, result, x1);
	return result;
}

safe asin(const safe& x1) {
	value result;
	safecall_1(&Zasin, result, x1);
	return result;
}

safe atan(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zatan, result, x1, x2);
	return result;
}

safe center(const safe& x1, const safe& x2, const safe& x3) {
	value result;
	safecall_3(&Zcenter, result, x1, x2, x3);
	return result;
}

safe char_(const safe& x1) {
	value result;
	safecall_1(&Zchar, result, x1);
	return result;
}

safe chdir(const safe& x1) {
	value result;
	safecall_1(&Zchdir, result, x1);
	return result;
}

safe close(const safe& x1) {
	value result;
	safecall_1(&Zclose, result, x1);
	return result;
}

safe collect() {
	value result;
	safecall_0(&Zcollect, result);
	return result;
}

safe copy(const safe& x1) {
	value result;
	safecall_1(&Zcopy, result, x1);
	return result;
}

safe cos(const safe& x1) {
	value result;
	safecall_1(&Zcos, result, x1);
	return result;
}

safe cset(const safe& x1) {
	value result;
	safecall_1(&Zcset, result, x1);
	return result;
}

safe delay(const safe& x1) {
	value result;
	safecall_1(&Zdelay, result, x1);
	return result;
}

safe delete_(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zdelete, result, x1, x2);
	return result;
}

safe detab(const variadic& x1) {
    safe result;
	safecall_vbl(&Zdetab, result, x1);
	return result;
}

safe detab(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return detab( (x1,x2) );
	if( x4.isIllegal() )
		return detab( (x1,x2,x3) );
	if( x5.isIllegal() )
		return detab( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return detab( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return detab( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return detab( (x1,x2,x3,x4,x5,x6,x7) );		
	return detab( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe display(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zdisplay, result, x1, x2);
	return result;
}

safe dtor(const safe& x1) {
	value result;
	safecall_1(&Zdtor, result, x1);
	return result;
}

safe entab(const variadic& x1) {
    safe result;
	safecall_vbl(&Zentab, result, x1);
	return result;
}

safe errorclear() {
	value result;
	safecall_0(&Zerrorclear, result);
	return result;
}

safe exit(const safe& x1) {
	value result;
	safecall_1(&Zexit, result, x1);
	return result;
}

safe exp(const safe& x1) {
	value result;
	safecall_1(&Zexp, result, x1);
	return result;
}

safe flush(const safe& x1) {
	value result;
	safecall_1(&Zflush, result, x1);
	return result;
}

safe function() {
	value result;
	safecall_0(&Z_function, result);	//generative: Z_
	return result;
}

safe get(const safe& x1) {
	value result;
	safecall_1(&Zget, result, x1);
	return result;
}

safe getch() {
	value result;
	safecall_0(&Zgetch, result);
	return result;
}

safe getche() {
	value result;
	safecall_0(&Zgetche, result);
	return result;
}

safe getenv(const safe& x1) {
	value result;
	safecall_1(&Zgetenv, result, x1);
	return result;
}

safe iand(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Ziand, result, x1, x2);
	return result;
}

safe icom(const safe& x1) {
	value result;
	safecall_1(&Zicom, result, x1);
	return result;
}

safe image(const safe& x1) {
	value result;
	safecall_1(&Zimage, result, x1);
	return result;
}

safe insert(const safe& x1, const safe& x2, const safe& x3) {
	value result;
	safecall_3(&Zinsert, result, x1, x2, x3);
	return result;
}

safe integer(const safe& x1) {
	value result;
	safecall_1(&Zinteger, result, x1);
	return result;
}

safe ior(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zior, result, x1, x2);
	return result;
}

safe ishift(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zishift, result, x1, x2);
	return result;
}

safe ixor(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zixor, result, x1, x2);
	return result;
}

safe kbhit() {
	value result;
	safecall_0(&Zkbhit, result);
	return result;
}

safe key(const safe& x1) {
	value result;
	safecall_1(&Z_key, result, x1);		//generative: Z_
	return result;
}

safe left(const safe& x1, const safe& x2, const safe& x3) {
	value result;
	safecall_3(&Zleft, result, x1, x2, x3);
	return result;
}

safe list(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zlist, result, x1, x2);
	return result;
}

safe loadfunc(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zloadfunc, result, x1, x2);
	return result;
}

safe log(const safe& x1) {
	value result;
	safecall_1(&Zlog, result, x1);
	return result;
}

safe map(const safe& x1, const safe& x2, const safe& x3) {
	value result;
	safecall_3(&Zmap, result, x1, x2, x3);
	return result;
}

safe member(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zmember, result, x1, x2);
	return result;
}

safe name(const safe& x1) {
	value result;
	safecall_1(&Zname, result, x1);
	return result;
}

safe numeric(const safe& x1) {
	value result;
	safecall_1(&Znumeric, result, x1);
	return result;
}

safe open(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zopen, result, x1, x2);
	return result;
}

safe ord(const safe& x1) {
	value result;
	safecall_1(&Zord, result, x1);
	return result;
}

safe pop(const safe& x1) {
	value result;
	safecall_1(&Zpop, result, x1);
	return result;
}

safe proc(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zproc, result, x1, x2);
	return result;
}

safe pull(const safe& x1) {
	value result;
	safecall_1(&Zpull, result, x1);
	return result;
}

safe push(const variadic& x1) {
    safe result;
	safecall_vbl(&Zpush, result, x1);
	return result;
}

safe push(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return push( (x1,x2) );
	if( x4.isIllegal() )
		return push( (x1,x2,x3) );
	if( x5.isIllegal() )
		return push( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return push( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return push( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return push( (x1,x2,x3,x4,x5,x6,x7) );		
	return push( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe put(const variadic& x1) {
    safe result;
	safecall_vbl(&Zput, result, x1);
	return result;
}

safe put(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return put( (x1,x2) );
	if( x4.isIllegal() )
		return put( (x1,x2,x3) );
	if( x5.isIllegal() )
		return put( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return put( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return put( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return put( (x1,x2,x3,x4,x5,x6,x7) );		
	return put( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe read(const safe& x1) {
	value result;
	safecall_1(&Zread, result, x1);
	return result;
}

safe reads(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zreads, result, x1, x2);
	return result;
}

safe real(const safe& x1) {
	value result;
	safecall_1(&Zreal, result, x1);
	return result;
}

safe remove(const safe& x1) {
	value result;
	safecall_1(&Zremove, result, x1);
	return result;
}

safe rename(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zrename, result, x1, x2);
	return result;
}

safe repl(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zrepl, result, x1, x2);
	return result;
}

safe reverse(const safe& x1) {
	value result;
	safecall_1(&Zreverse, result, x1);
	return result;
}

safe right(const safe& x1, const safe& x2, const safe& x3) {
	value result;
	safecall_3(&Zright, result, x1, x2, x3);
	return result;
}

safe rtod(const safe& x1) {
	value result;
	safecall_1(&Zrtod, result, x1);
	return result;
}

safe runerr(const safe& x1, const safe& x2) {
	value result;
	safecall_v2(&Zrunerr, result, x1, x2);
	return result;
}

safe runerr(const safe& x1) {
	value result;
	safecall_v1(&Zrunerr, result, x1);
	return result;
}

safe seek(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zseek, result, x1, x2);
	return result;
}

safe serial(const safe& x1) {
	value result;
	safecall_1(&Zserial, result, x1);
	return result;
}

safe set(const safe& x1) {
	value result;
	safecall_1(&Zset, result, x1);
	return result;
}

safe sin(const safe& x1) {
	value result;
	safecall_1(&Zsin, result, x1);
	return result;
}

safe sort(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zsort, result, x1, x2);
	return result;
}

safe sortf(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Zsortf, result, x1, x2);
	return result;
}

safe sqrt(const safe& x1) {
	value result;
	safecall_1(&Zsqrt, result, x1);
	return result;
}

safe stop() {
	safe result, nullarg;
	safecall_vbl(&Zstop, result, nullarg);
	return result;
}

safe stop(const variadic& x1) {
    safe result;
	safecall_vbl(&Zstop, result, x1);
	return result;
}

safe stop(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return stop( (x1,x2) );
	if( x4.isIllegal() )
		return stop( (x1,x2,x3) );
	if( x5.isIllegal() )
		return stop( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return stop( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return stop( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return stop( (x1,x2,x3,x4,x5,x6,x7) );		
	return stop( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe string(const safe& x1) {
	value result;
	safecall_1(&Zstring, result, x1);
	return result;
}

safe system(const safe& x1) {
	value result;
	safecall_1(&Zsystem, result, x1);
	return result;
}

safe table(const safe& x1) {
	value result;
	safecall_1(&Ztable, result, x1);
	return result;
}

safe tan(const safe& x1) {
	value result;
	safecall_1(&Ztan, result, x1);
	return result;
}

safe trim(const safe& x1, const safe& x2) {
	value result;
	safecall_2(&Ztrim, result, x1, x2);
	return result;
}

safe type(const safe& x1) {
	value result;
	safecall_1(&Ztype, result, x1);
	return result;
}

safe variable(const safe& x1) {
	value result;
	safecall_1(&Zvariable, result, x1);
	return result;
}

safe where(const safe& x1) {
	value result;
	safecall_1(&Zwhere, result, x1);
	return result;
}

safe write() {
	safe result, nullarg;
	safecall_vbl(&Zwrite, result, nullarg);
	return result;
}

safe write(const variadic& x1) {
    safe result;
	safecall_vbl(&Zwrite, result, x1);
	return result;
}

safe write(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return write( (x1,x2) );
	if( x4.isIllegal() )
		return write( (x1,x2,x3) );
	if( x5.isIllegal() )
		return write( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return write( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return write( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return write( (x1,x2,x3,x4,x5,x6,x7) );		
	return write( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

safe writes(const variadic& x1) {
    safe result;
	safecall_vbl(&Zwrites, result, x1);
	return result;
}

safe writes(	const safe& x1, const safe& x2,
			const safe& x3, const safe& x4,
			const safe& x5, const safe& x6,
			const safe& x7, const safe& x8 ) {
	if( x3.isIllegal() )
		return writes( (x1,x2) );
	if( x4.isIllegal() )
		return writes( (x1,x2,x3) );
	if( x5.isIllegal() )
		return writes( (x1,x2,x3,x4) );
	if( x6.isIllegal() )
		return writes( (x1,x2,x3,x4,x5) );
	if( x7.isIllegal() )
		return writes( (x1,x2,x3,x4,x5,x6) );
	if( x8.isIllegal() )
		return writes( (x1,x2,x3,x4,x5,x6,x7) );		
	return writes( (x1,x2,x3,x4,x5,x6,x7,x8) );
}

//generative functions crippled to return a single value follow

safe any(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue) {
	value result;
	safecall_4(&Z_any, result, x1, x2, x3, x4);
	return result;
}

safe many(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue) {
	value result;
	safecall_4(&Z_many, result, x1, x2, x3, x4);
	return result;
}

safe upto(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue) {
	value result;
	safecall_4(&Z_upto, result, x1, x2, x3, x4);
	return result;
}

safe find(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue) {
	value result;
	safecall_4(&Z_find, result, x1, x2, x3, x4);
	return result;
}

safe match(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue) {
	value result;
	safecall_4(&Z_match, result, x1, x2, x3, x4);
	return result;
}

safe bal(const safe& x1, const safe& x2=nullvalue, const safe& x3=nullvalue, const safe& x4=nullvalue, const safe& x5=nullvalue, const safe& x6=nullvalue) {
	value result;
	safecall_6(&Z_bal, result, x1, x2, x3, x4, x5, x6);
	return result;
}

safe move(const safe& x1) {
	value result;
	safecall_1(&Z_move, result, x1);
	return result;
}

safe tab(const safe& x1) {
	value result;
	safecall_1(&Z_tab, result, x1);
	return result;
}

}; //namespace Icon

/*
 * Useful functions
 */

//pass this on to external libraries, so they don't have to link against iconx (cygwin)
void syserror(const char* s) { syserr((char *)s); }

value IconFile(FILE* fd, int status, char* fname) {
	value answer, filename(NewString, fname);
	answer.dword = D_File;
	answer.vword = (long)alcfile(fd, status, &filename);
	return answer;
}

//large integer related and base64 related functions follow

struct bignum {	//after b_bignum in rstructs.h
   long title;
   long blksize;
   long msd, lsd;
   int sign;
   unsigned int digit[1];
};

//Endian/wordsize nonsense follows, to help get at bytes in the digits of Icon BigIntegers

//repair moves the non-zero bytes we care about in a DIGIT (see rlrgint.r)
//that are in the least significant half of the bytes of a uint
//into the left hand end (in RAM)  of the unint in big endian order

//for solaris that does not define this macro
#ifndef BYTE_ORDER
#define BYTE_ORDER 4321
#endif

#if BYTE_ORDER==1234 || BYTE_ORDER==4321
const int DIGITBYTES=2;

#if BYTE_ORDER==1234
inline unsigned int repair(unsigned int x) {
	return (x & 0x0000FF00) >> 8 | (x & 0x000000FF) << 8;
}
inline long bigendian(long n) {
	n = (n & 0xFFFF0000) >> 16 | (n & 0x0000FFFF) << 16;
	return (n & 0xFF00FF00) >> 8 | (n & 0x00FF00FF) << 8;
}
#endif

#if BYTE_ORDER==4321
inline unsigned int repair(unsigned int x) {
	return x << 2;
}
inline long bigendian(long n) {
	return n;
}
#endif

#endif

#if BYTE_ORDER==12345678 || BYTE_ORDER==87654321
const int DIGITBYTES=4;

#if BYTE_ORDER==12345678
inline unsigned int repair(unsigned int x) {
	x = (x & 0x00000000FFFF0000) >> 16 | (x & 0x000000000000FFFF) << 16;
	return (x & 0x00000000FF00FF00) >> 8 | (x & 0x0000000000FF00FF) << 8;
}
inline long bigendian(long n) {
	n = (n & 0xFFFFFFFF00000000) >> 32 | (n & 0x00000000FFFFFFFF) << 32;
	n = (n & 0xFFFF0000FFFF0000) >> 16 | (n & 0x0000FFFF0000FFFF) << 16;
	return (n & 0xFF00FF00FF00FF00) >> 8 | (n & 0x00FF00FF00FF00FF) << 8;
}
#endif

#if BYTE_ORDER==87654321
inline unsigned int repair(unsigned int x) {
	return x << 4;
}
inline long bigendian(long n) {
	return n;
}
#endif

#endif

value integertobytes(value bigint){	//get the bytes of an Icon long integer as an Icon string (ignore sign)
	safe n(bigint);
	if( n == 0 ) return nullchar;
	switch( bigint.type() ) {
		case Integer: {
			long x = bigint;
			x = bigendian(x);
			char *sbuf = (char *)&x;
			int len = sizeof(long);
			while( !*sbuf ) {  //skip leading zeros in base 256
				++sbuf;
				--len;
			}
			return value(sbuf, len);
			break;
		}
		case BigInteger: {
			bignum *bp = ((bignum*)(bigint.vword));
			unsigned int current;
			long pos = 0, len = (bp->lsd - bp->msd + 1) * DIGITBYTES;
			char *source, *buf = new char[len], *sbuf;
			sbuf = buf;
			for(long i = bp->msd; i <= bp->lsd; ++i) {
				current = repair(bp->digit[i]);
				source = (char *)&current;
				for(int b=0; b < DIGITBYTES; ++b)
					sbuf[pos++] = source[b];
			}
			while( !*sbuf ) {  //skip leading zeros in base 256
				++sbuf;
				--len;
			}
			value bytestring(sbuf, len);
			delete[] buf;
			return bytestring;
		}
		default:
			return nullvalue;
	} 
}

value bytestointeger(value bytestring){	//get the bytes of a new Icon long integer from an Icon string
	if( bytestring.type() != String ) return nullvalue;
	while( *(char*)bytestring.vword == 0 && bytestring.dword != 0 ) { //skip leading zeros
		--bytestring.dword;
		++bytestring.vword;
	}
	safe s(bytestring);
	long size = value(*s);
	if( size == 0 ) return 0;
	unsigned char *bytes = (unsigned char *)((char*)bytestring);
	long n = 0;
	if( size < sizeof(long) || //doesn't overflow a signed long
			(size == sizeof(long) && ( bytes[0] <= 0x7F ))  ) {
		for(int i = 0; i < size; ++i)
			n = (n << 8) + bytes[i];
		return n;
	}
	static const int RATIO = sizeof(unsigned int)/2;
	long len = (size + RATIO - 1)/RATIO; //number of digits
	bignum *bp = (bignum *)alcbignum(len);
	bytestring = s; //in case the allocation caused a garbage collection
	bytes = (unsigned char *)((char*)bytestring);
	long pos = 0;
	const int FIRST = len*RATIO==size ? RATIO : len*RATIO-size; //bytes in the first digit
	n = 0;
	for(int p=0; p < FIRST; ++p)
		n = (n << 8) + bytes[pos++];
	bp->digit[0] = n;		
	for(long i = bp->msd + 1; i <= bp->lsd; ++i) {
		n = 0; 
		for(int p=0; p < RATIO; ++p)
			n = (n << 8) + bytes[pos++];
		bp->digit[i] = n;
	}
	value answer;
	answer.dword = D_Lrgint;
	answer.vword = (long)bp;
	return answer;
}

//base64 utilities
typedef unsigned char uchar;
static char chr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//3 bytes -> four base64 chars
inline void threetofour(uchar *three, uchar* four) {
	unsigned long n = three[0];
	n = (((n << 8) + three[1]) << 8) + three[2];
	four[3] = chr[n & 0x3F];
	n = n >> 6;
	four[2] = chr[n & 0x3F];
	n = n >> 6;
	four[1] = chr[n & 0x3F];
	n = n >> 6;
	four[0] = chr[n & 0x3F];
}

//two trailing bytes -> four base64 chars
inline void twotofour(uchar *three, uchar* four) {
	unsigned long n = three[0];
	n = ((n << 8) + three[1]) << 2;
	four[3] = '=';
	four[2] = chr[n & 0x3F];
	n = n >> 6;
	four[1] = chr[n & 0x3F];
	n = n >> 6;
	four[0] = chr[n & 0x3F];
}

//one trailing byte -> four base64 chars
inline void onetofour(uchar *three, uchar* four) {
	unsigned long n = three[0];
	n = n << 4;
	four[3] = four[2] = '=';
	four[1] = chr[n & 0x3F];
	n = n >> 6;
	four[0] = chr[n & 0x3F];
}

//convert to base64, return the length of the encoded string
inline long b64(char *in, long len, char* out) {
	char *start = out;
	long num = len/3;
	int rem = len%3;
	for(long i = 0; i < num; ++i) {
		threetofour((uchar*)in, (uchar*)out);
		in += 3;
		out += 4;
	}
	switch( rem ) {
		case 1: 
			onetofour((uchar*)in, (uchar*)out);
			out += 4;
			break;
		case 2: 
			twotofour((uchar*)in, (uchar*)out); 
			out += 4;
			break;
	}
	return out - start;
}

//constant denoting an invalid character in a putative base64 encoding
static const int NONSENSE = -1;

//convert a base64 char into its corresponding 6 bits
inline int undo(uchar ch) {
	switch( ch ) {
		default: return NONSENSE;
		 case 'A': return  0; case 'B': return  1; case 'C': return  2; case 'D': return  3;
		 case 'E': return  4; case 'F': return  5; case 'G': return  6; case 'H': return  7;
		 case 'I': return  8; case 'J': return  9; case 'K': return 10; case 'L': return 11;
		 case 'M': return 12; case 'N': return 13; case 'O': return 14; case 'P': return 15;
		 case 'Q': return 16; case 'R': return 17; case 'S': return 18; case 'T': return 19;
		 case 'U': return 20; case 'V': return 21; case 'W': return 22; case 'X': return 23;
		 case 'Y': return 24; case 'Z': return 25; case 'a': return 26; case 'b': return 27;
		 case 'c': return 28; case 'd': return 29; case 'e': return 30; case 'f': return 31;
		 case 'g': return 32; case 'h': return 33; case 'i': return 34; case 'j': return 35;
		 case 'k': return 36; case 'l': return 37; case 'm': return 38; case 'n': return 39;
		 case 'o': return 40; case 'p': return 41; case 'q': return 42; case 'r': return 43;
		 case 's': return 44; case 't': return 45; case 'u': return 46; case 'v': return 47;
		 case 'w': return 48; case 'x': return 49; case 'y': return 50; case 'z': return 51;
		 case '0': return 52; case '1': return 53; case '2': return 54; case '3': return 55;
		 case '4': return 56; case '5': return 57; case '6': return 58; case '7': return 59;
		 case '8': return 60; case '9': return 61; case '+': return 62; case '/': return 63;
	}
}

//four base64 chars -> three bytes
inline long unfour(uchar* four, uchar* three) {
	int ch;
	if( (ch = undo(four[0])) == NONSENSE ) return NONSENSE;
	long n = ch;
	if( (ch = undo(four[1])) == NONSENSE ) return NONSENSE;
	n = (n << 6) + ch;
	if( (ch = undo(four[2])) == NONSENSE ) return NONSENSE;
	n = (n << 6) + ch;
	if( (ch = undo(four[3])) == NONSENSE ) return NONSENSE;
	n = (n << 6) + ch;
	three[2] = n & 0xFF;
	n = n >> 8;
	three[1] = n & 0xFF;
	three[0] = n >> 8;
}

//decode a base64 string; return NONSENSE if anything doesn't make strict sense
inline long unb64(char* in, long len, char* out) {
	char* start = out;
	if( len == 0 ) return 0;
	if( len%4 != 0 ) return NONSENSE;
	int last = 0;
	if( in[len-1] == '=' ) {
		last = 1;
		if( in[len-2] == '=' ) last = 2;
	}
	if( last ) len -= 4;

	for(long i = 0; i < len/4; ++i) {
		if( unfour((uchar*)in, (uchar*)out) == NONSENSE )
			return NONSENSE;
		in += 4;
		out += 3;
	}
	long n;
	int ch0, ch1, ch2;
	switch( last ) {
		case 1:
			if( (ch0 = undo((uchar)in[0])) == NONSENSE )
				return NONSENSE;
			if( (ch1 = undo((uchar)in[1])) == NONSENSE )
				return NONSENSE;
			if( (ch2 = undo((uchar)in[2])) == NONSENSE )
				return NONSENSE;
			n = ((((ch0 << 6) + ch1) << 6) + ch2) >> 2;
			out[1] = n & 0xFF;
			out[0] = n >> 8;
			out += 2;
			break;
		case 2:
			if( (ch0 = undo((uchar)in[0])) == NONSENSE )
				return NONSENSE;
			if( (ch1 = undo((uchar)in[1])) == NONSENSE )
				return NONSENSE;
			n = (ch0 << 6) + ch1;
			out[0] = n >> 4;
			out += 1;
			break;
	}
	return out - start;
}

//convert string or integer to base64 string
value base64(value x) {
	switch( x.type() ) {
		default:
			return nullvalue;
		case Integer:
		case BigInteger:
			x = integertobytes(x);
		case String: {
			char* enc = new char[4*x.dword/3+8]; //safety first
			long len = b64((char*)x.vword, x.dword, enc);
			value answer(enc, len);
			delete[] enc;
			return answer;
		}			
	}
}

//decode base64 encoding of a string
value base64tostring(value s) {
	if( s.type() != String ||
			s.dword % 4 != 0) 
		return nullvalue;
	if( s.dword == 0 ) return nullstring;
	long len;
	char* dec = new char[3 * s.dword/4]; //safety first
	if( (len = unb64((char*)s.vword, s.dword, dec)) == NONSENSE ) {
		delete[] dec;
		return nullvalue;
	}
	value answer(dec, len);
	delete[] dec;
	return answer;
}

//decode base64 encoding of an integer
value base64tointeger(value s) {
	return bytestointeger(base64tostring(s));
}



/*
 * 1. Calling Icon from C++ (mostly in iloadgpx.cpp and iloadnogpx.cpp)
 * 2. loadfuncpp itself
 * 3. binding records to procedure blocks
 */

namespace ifload {
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

#include "xfload.cpp"	//inline linkage --- three argument raw loadfunc
}; //end namespace ifload; put things that need Icon's rt.h included by xfload.cpp below here

//call to the modified loadfunc in xfload.cpp
static int rawloadfuncpp(value argv[]) {
	return ifload::Z_loadfunc((ifload::dptr)argv);
}


//get the record from the bottom of an extended procedure block
//(procedure bound to record) obtained from the procedure that
//called our procedure self(). Fail if no record is bound.
extern "C" int getbinding(value* argv) {
	value* pp = (value*)((ifload::pfp)->pf_argp);	//get saved procedure
	if( pp==0 ) syserror("loadfuncpp bug: attempt to find caller of self() failed!");
	proc_block* pbp = *pp;
    int nsafe = pbp->ndynam + pbp->nparam;
    if( (pbp->blksize) - sizeof(proc_block) == (nsafe-1) * sizeof(value) ) {
		argv[0] = nullvalue;
		return FAILED;
	}
    argv[0] = pbp->lnames[nsafe];
	return SUCCEEDED;
}


#if __CYGWIN__ //cygwin linkage problem workaround
namespace icall {
	using namespace ifload;
	//icall assigned from whichever of iloadgpx.so and iloadnogpx.so is loaded, on load thereof
extern "C" {
	typedef int icallfunction(dptr procptr, dptr arglistptr, dptr result);
};
	icallfunction *icall2;
};

value Value::call(const value& proc, const value& arglist) {
	value result;
	(*(icall::icall2))( (icall::dptr)(&proc), (icall::dptr)(&arglist), (icall::dptr)(&result) );
	return result;
}
#endif //cygwin linkage problem workaround

