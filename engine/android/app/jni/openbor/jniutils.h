#ifndef OPENBOR_JNIUTILS_H
#define OPENBOR_JNIUTILS_H

#include <jni.h>

/** WARNING: We should minimally call Java method from C code. Use it with care. **/

// include guard here to allow C-code to include this header file
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Vibrate device
 */
void jniutils_vibrate_device(jint intensity);

#ifdef __cplusplus
}
#endif

#endif
