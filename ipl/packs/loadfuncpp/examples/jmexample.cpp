
/* Example of a C++ extension to icon via loadfunc,
 * without garbage collection difficulties.
 * Type 'make <platform>' to build.
 * For available <platform>s type 'make'.
 * Carl Sturtivant, 2007/9/25
 */

#include "loadfuncpp.h"

enum { JMUP, JMDOWN };
class sequence: public generator {
    long count;
    long limit;
    int direction;
    bool hasNext() {
		switch(direction) {
			case JMUP:
				return count <= limit;
			case JMDOWN:
				return count >= limit;
			default:
				return false;
		}
    }
    value giveNext() {
		switch(direction) {
			case JMUP:
				return count++;
			case JMDOWN:
				return count--;
			default:
				return nullvalue;
        }
    }
  public:
    sequence(value start, value end) {
		count = start;
		limit = end;
		direction = ((count < limit) ? JMUP : JMDOWN);
    };
};

extern "C" int jm_test_1(int argc, value argv[]) {
    if( argc != 2 ) {
		return FAILED;
    }
    sequence s(argv[1], argv[2]);
    return s.generate(argv);
}


