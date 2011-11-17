#ifndef _except_h_
#define _except_h_

#ifndef W2K_ONLY
#pragma warning( disable : 4102 ) /*  suppress "unreferenced label" message */

/** The MSDN says nothing about __leave in try-except, but the compiler accepts it.
 * This is how it is done in GCC (http://reactos.wox.org/index.php?page=gccseh):
 * __leave // Jump to closing brace of try block.
 * And so do we, jump to beginning of __except() and __finally.
 */
#define __except(filter)	___finally: if(0)	//not supported: never execute this block
#define __try									//always execute, nothing needed
#define __finally			___finally:			//label for the "finally" block
#define __leave				{goto ___finally;}	//Jump to that label

#endif

#endif // _except_h_
