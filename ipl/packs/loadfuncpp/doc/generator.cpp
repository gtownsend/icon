
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make iexample' to build.
 * Carl Sturtivant, 2008/3/16
 */

#include "loadfuncpp.h"
using namespace Icon;

class sequence: public generator {
    safe current, inc;
  public:
    sequence(safe start, safe increment) {
        current = start - increment;
        inc = increment;
    }
    virtual bool hasNext() {
        return true;
    }
    virtual value giveNext() {
        return current = current + inc;
    }
};

extern "C" int seq2(value argv[]){
    sequence seq(argv[1], argv[2]);
    return seq.generate(argv);
}


