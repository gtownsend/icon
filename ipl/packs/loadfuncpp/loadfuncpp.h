
/* C++ support for easy extensions to icon via loadfunc,
 * without garbage collection difficulties.
 * Include this and link to iload.cpp which
 * contains the necessary glue.
 * See iexample.cpp for typical use.
 * Carl Sturtivant, 2008/3/17
 */

#include<new>
#include<cstdio>

enum kind { Null, Integer, BigInteger, Real, Cset, File, Procedure, Record, List,
            Set=10, Table=12, String, Constructor, Coexpression=18, External, Variable };

enum special_value { NullString, StringLiteral, NewString, NullChar, Illegal };

enum {
	SUCCEEDED = 7,  // Icon function call returned: A_Continue
	FAILED    = 1   // Icon function call failed: A_Resume
};

class value; 	//Icon value (descriptor)
class safe; //for garbage-collection-safe Icon valued C++ variables and parameters of all kinds
class keyword;  //Icon keyword represented as an object with unary &
class variadic; //for garbage-collection-safe variadic function argument lists
class proc_block; 	//block specifying a procedure to iconx
class external_block;		//block specifying an external value to iconx
class external_ftable;	//function pointers specifying external value behavior to iconx
class external;			//C++ Object specifying an external value

typedef int iconfunc(value argv[]); //type of icon built in functions or operators with a fixed number of arguments
typedef int iconfvbl(int argc, value argv[]); //type of icon built in functions with a variable number of arguments

extern const value nullvalue; 		//for default arguments
extern const value nullstring;
extern const value nullchar;
extern const value illegal;			//for unwanted trailing arguments
extern void syserror(const char*); 	//fatal termination Icon-style with error message
#define Fs_Read		0001		// file open for reading
#define Fs_Write	0002		// file open for writing
extern value IconFile(int fd, int status, char* fname); //make an Icon file descriptor
extern value integertobytes(value);	//get the bytes of an Icon long integer as an Icon string (ignore sign)
extern value bytestointeger(value);	//get the bytes of a new Icon long integer from an Icon string
extern value base64(value);				//convert string or integer to base64 encoding (string)
extern value base64tointeger(value);	//decode base64 string to integer
extern value base64tostring(value);		//decode base64 string to string

namespace Icon {
//all keywords excepting &fail, &cset (avoiding a name collision with function cset)
extern keyword allocated;
extern keyword ascii;
extern keyword clock;
extern keyword collections;
extern keyword current;
extern keyword date;
extern keyword dateline;
extern keyword digits;
extern keyword dump;
extern keyword e;
extern keyword error;
extern keyword errornumber;
extern keyword errortext;
extern keyword errorvalue;
extern keyword errout;
extern keyword features;
extern keyword file;
extern keyword host;
extern keyword input;
extern keyword lcase;
extern keyword letters;
extern keyword level;
extern keyword line;
extern keyword main;
extern keyword null;
extern keyword output;
extern keyword phi;
extern keyword pi;
extern keyword pos;
extern keyword progname;
extern keyword random;
extern keyword regions;
extern keyword source;
extern keyword storage;
extern keyword subject;
extern keyword time;
extern keyword trace;
extern keyword ucase;
extern keyword version;
}; //namespace Icon

static void initialize_keywords();

class keyword { //objects representing Icon keywords
	friend void initialize_keywords();
	iconfunc* f;
  public:
    safe operator&(); //get the keyword's value (could be an Icon 'variable')
};


class value { //a descriptor with class
//data members modelled after 'typedef struct { word dword, vword; } descriptor;' from icall.h
  private:
    long dword;
    long vword;
  public:
    friend class safe;
	friend value IconFile(FILE* fd, int status, char* fname);
	friend value integertobytes(value);
	friend value bytestointeger(value);
	friend value base64(value);
	friend value base64tointeger(value);
	friend value base64tostring(value);
    value(); //&null
    value(special_value, const char* text = "");
    value(int argc, value* argv); //makes a list of parameters passed in from Icon
	value(int);
	value(long);
	value(float);
	value(double);
    value(char*);
    value(const char*);
    value(const char*, long);
	value(proc_block&);
	value(proc_block*);
	value(external*);
	operator int();
    operator long();
	operator float();
	operator double();
    operator char*();
	operator external*();
	operator proc_block*() const;
	bool operator==(const value&) const;
    value& dereference();
    value intify();
    bool isNull();
    bool notNull();
	bool isExternal(const value&);
    value size() const;
    kind type();
    bool toString(); //attempted conversion in place
    bool toCset();
    bool toInteger();
    bool toReal();
    bool toNumeric();
    value subscript(const value&) const; //produces an Icon 'variable'
    value& assign(const value&);   //dereferences Icon style
    value put(value x = nullvalue);
    value push(value x = nullvalue);
    void dump() const;
    void printimage() const;
    int compare(const value&) const; 	//comparator-style result: used for Icon sorting
    value negative() const;		//  -x
    value complement() const;		// 	~x
    value refreshed() const;		// 	^x
    value random() const;			//	?x
    value plus(const value&) const;
    value minus(const value&) const;
    value multiply(const value&) const;
    value divide(const value&) const;
    value remainder(const value&) const;
    value power(const value&) const;
    value union_(const value&) const;			//	x ++ y
    value intersection(const value&) const;	//	x ** y
    value difference(const value&) const;		//	x -- y
    value concatenate(const value&) const; 	// 	x || y
    value listconcatenate(const value&) const;//	x ||| y
    value slice(const value&, const value&) const; 		//	x[y:z]
    value& swap(value&);				//  x :=: y
    value activate(const value& y = nullvalue) const;		//	y @ x ('*this' is activated)
    value apply(const value&) const;			//  x!y (must return, not fail or suspend)
}; //class value


class generator {
//class to inherit from for defining loadable functions that are generators
  public:
	int generate(value argv[]); //call to suspend everything produced by next()
  protected: //override these, and write a constructor
	virtual bool hasNext();
	virtual value giveNext();
}; //class generator


class iterate {
//class to inherit from for iterating over f!arg or !x
  public:
	void every(const value& g, const value& arg); //perform the iteration over g!arg
	void bang(const value& x); //perform the iteration over !x
	//override these, write a constructor and the means of recovering the answer
	virtual bool wantNext(const value& x);
	virtual void takeNext(const value& x);
};



class safe_variable {
//data members modelled after 'struct tend_desc' from rstructs.h
	friend class value;
    friend inline int safecall_0(iconfunc*,  value&);
    friend inline int safecall_1(iconfunc*,  value&, const value&);
    friend inline int safecall_2(iconfunc*,  value&, const value&, const value&);
    friend inline int safecall_3(iconfunc*,  value&, const value&, const value&, const value&);
    friend inline int safecall_4(iconfunc*,  value&, const value&, const value&, const value&, const value&);
    friend inline int safecall_5(iconfunc*,  value&, const value&, const value&, const value&, const value&, const value&);
    friend inline int safecall_6(iconfunc*,  value&, const value&, const value&, const value&, const value&, const value&, const value&);
    friend inline int safecall_v0(iconfvbl*, value&);
    friend inline int safecall_v1(iconfvbl*, value&, const value&);
    friend inline int safecall_v2(iconfvbl*, value&, const value&, const value&);
    friend inline int safecall_v3(iconfvbl*, value&, const value&, const value&, const value&);
    friend inline int safecall_vbl(iconfvbl*,safe&, const variadic&);
  protected:
    safe_variable *previous;
    int num;
  	value val;
  	safe_variable();
  	safe_variable(int);
  	safe_variable(long);
  	safe_variable(double);
  	safe_variable(value);
	safe_variable(proc_block&);
	safe_variable(proc_block*);
  	safe_variable(int, value*);
    inline void push(safe_variable*& tendlist, int numvalues=1);
    inline void pop(safe_variable*& tendlist);
}; //class safe_variable


class variadic: public safe_variable {
  public:
	variadic(int);
	variadic(long);
	variadic(float);
	variadic(double);
	variadic(char*);
	variadic(value);
    variadic(const safe&);
    variadic(const safe&, const safe&);
    variadic& operator,(const safe&);
	operator value();
	~variadic();
}; //class variadic


class external_block {
//modelled on 'struct b_external' in icon/src/h/rstructs.h
	friend class external;
	friend class value;
	static long extra_bytes;	//silent extra parameter to new
	long title;
	long blksize;
	long id;
	external_ftable* funcs;
	external* val;
	static void* operator new(size_t); 	//allocated by iconx
	static void operator delete(void*);	//do nothing
	external_block();
};

class external {
	friend class value;
	static external_block* blockptr; //silent extra result of new
  protected:
	long id;
  public:
 	static void* operator new(size_t); 	//allocated by new external_block()
	static void operator delete(void*);	//do nothing
 	external();
	virtual ~external() {}	//root class
	virtual long compare(external*);
	virtual value name();
	virtual external* copy();
	virtual value image();
};


class safe: public safe_variable {
//use for a garbage collection safe icon valued safe C++ variable
	friend class variadic;
	friend class global;
  public:
    safe(); //&null
    safe(const safe&);
    safe(int);
    safe(long);
	safe(float);
	safe(double);
    safe(char*);
    safe(const value&);
    safe(const variadic&);
    safe(proc_block&);
    safe(proc_block*);
    safe(int, value*); //from parameters sent in from Icon
    ~safe();
    safe& operator=(const safe&);
    //augmenting assignments here
    safe& operator+=(const safe&);
    safe& operator-=(const safe&);
    safe& operator*=(const safe&);
    safe& operator/=(const safe&);
    safe& operator%=(const safe&);
    safe& operator^=(const safe&);
    safe& operator&=(const safe&);
    safe& operator|=(const safe&);
    // ++ and -- here
    safe& operator++();
    safe& operator--();
    safe operator++(int);
    safe operator--(int);
    //conversion to value
    operator value() const;
    //procedure call
    safe operator()();
    safe operator()(const safe&);
    safe operator()(const safe& x1, const safe& x2,
					const safe& x3 = illegal, const safe& x4 = illegal,
					const safe& x5 = illegal, const safe& x6 = illegal,
					const safe& x7 = illegal, const safe& x8 = illegal);
    safe operator[](const safe&);

    friend safe operator*(const safe&);    //size
    friend safe operator-(const safe&);
    friend safe operator~(const safe&);            //set complement
    friend safe operator+(const safe&, const safe&);
    friend safe operator-(const safe&, const safe&);
    friend safe operator*(const safe&, const safe&);
    friend safe operator/(const safe&, const safe&);
    friend safe operator%(const safe&, const safe&);
    friend safe operator^(const safe&, const safe&);  //exponentiation
    friend safe operator|(const safe&, const safe&);  //union
    friend safe operator&(const safe&, const safe&);  //intersection
    friend safe operator&&(const safe&, const safe&); //set or cset difference
    friend safe operator||(const safe&, const safe&); //string concatenation
    friend bool operator<(const safe&, const safe&);
    friend bool operator>(const safe&, const safe&);
    friend bool operator<=(const safe&, const safe&);
    friend bool operator>=(const safe&, const safe&);
    friend bool operator==(const safe&, const safe&);
    friend bool operator!=(const safe&, const safe&);
    friend variadic operator,(const safe&, const safe&);   //variadic argument list construction

    safe slice(const safe&, const safe&);     // x[y:z]
    safe apply(const safe&);           // x ! y
    safe listcat(const safe&);     // x ||| y
    safe& swap(safe&);             // x :=: y
    safe create();                 // create !x
    safe create(const safe&);     // create x!y
    safe activate(const safe& y = nullvalue); // y@x
    safe refresh();                 // ^x
    safe random();                      // ?x
    safe dereference();               // .x
	bool isIllegal() const;	//is an illegal value used for trailing arguments
}; //class safe


//Icon built-in functions
namespace Icon {
    safe abs(const safe&);
    safe acos(const safe&);
    safe args(const safe&);
    safe asin(const safe&);
    safe atan(const safe&, const safe&);
    safe center(const safe&, const safe&, const safe&);
    safe char_(const safe&);
    safe chdir(const safe&);
    safe close(const safe&);
    safe collect();
    safe copy(const safe&);
    safe cos(const safe&);
    safe cset(const safe&);
    safe delay(const safe&);
    safe delete_(const safe&, const safe&);
    safe detab(const variadic&);
	safe detab(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe display(const safe&, const safe&);
    safe dtor(const safe&);
    safe entab(const variadic&);
	safe entab(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe errorclear();
    safe exit(const safe&);
    safe exp(const safe&);
    safe flush(const safe&);
    safe function();            //generative: returns a list
    safe get(const safe&);
    safe getch();
    safe getche();
    safe getenv(const safe&);
    safe iand(const safe&, const safe&);
    safe icom(const safe&);
    safe image(const safe&);
    safe insert(const safe&, const safe&, const safe&);
    safe integer(const safe&);
    safe ior(const safe&, const safe&);
    safe ishift(const safe&, const safe&);
    safe ixor(const safe&, const safe&);
    safe kbhit();
    safe left(const safe&, const safe&, const safe&);
    safe list(const safe&, const safe&);
    safe loadfunc(const safe&, const safe&);
    safe log(const safe&);
    safe map(const safe&, const safe&, const safe&);
    safe member(const safe&, const safe&);
    safe name(const safe&);
    safe numeric(const safe&);
    safe open(const safe&, const safe&);
    safe ord(const safe&);
    safe pop(const safe&);
    safe proc(const safe&, const safe&);
    safe pull(const safe&);
    safe push(const variadic&);
	safe push(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe put(const variadic&);
	safe put(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe read(const safe&);
    safe reads(const safe&, const safe&);
    safe real(const safe&);
    safe remove(const safe&);
    safe rename(const safe&, const safe&);
    safe repl(const safe&, const safe&);
    safe reverse(const safe&);
    safe right(const safe&, const safe&, const safe&);
    safe rtod(const safe&);
    safe runerr(const safe&, const safe&);
    safe runerr(const safe&);
    safe seek(const safe&, const safe&);
    safe serial(const safe&);
    safe set(const safe&);
    safe sin(const safe&);
    safe sort(const safe&, const safe&);
    safe sortf(const safe&, const safe&);
    safe sqrt(const safe&);
	safe stop();
    safe stop(const variadic&);
	safe stop(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe string(const safe&);
    safe system(const safe&);
    safe table(const safe&);
    safe tan(const safe&);
    safe trim(const safe&, const safe&);
    safe type(const safe&);
    safe variable(const safe&);
    safe where(const safe&);
	safe write();
    safe write(const variadic&);
	safe write(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
    safe writes(const variadic&);
	safe writes(	const safe& x1, const safe& x2,
				const safe& x3=illegal, const safe& x4=illegal,
				const safe& x5=illegal, const safe& x6=illegal,
				const safe& x7=illegal, const safe& x8=illegal );
	//generative functions follow, crippled to return a single value
    safe any(const safe&, const safe&, const safe&, const safe&);
    safe many(const safe&, const safe&, const safe&, const safe&);
    safe upto(const safe&, const safe&, const safe&, const safe&);
    safe find(const safe&, const safe&, const safe&, const safe&);
    safe match(const safe&, const safe&, const safe&, const safe&);
    safe bal(const safe&, const safe&, const safe&, const safe&, const safe&, const safe&);
    safe move(const safe&);
    safe tab(const safe&);
}; //namespace Icon

