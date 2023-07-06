#ifndef __LIBKSWAP_H
#define __LIBKSWAP_H

/**
 * Hook for the only function provided by kswapapi.so
 * @param p No idea this gets discarded
 */
void kswap_collect(void *p);

#endif /* __LIBKSWAP_H */
