/* error message test: octal escape too large in a condition */

#if 0
one
#elif '\777'
two
#endif
