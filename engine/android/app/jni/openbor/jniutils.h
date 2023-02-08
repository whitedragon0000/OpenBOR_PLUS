#ifndef OPENBOR_JNIUTILS_H
#define OPENBOR_JNIUTILS_H

#include <jni.h>

/** WARNING: We should minimally call Java method from C code. Use it with care. **/

// include guard here to allow C-code to include this header file
#ifdef __cplusplus
extern "C" {
#endif

struct frame_dimensions {
    int x;
    int y;
    int width;
    int height;
    int top;
    int left;
    int bottom;
    int right;
};

/**
 * Vibrate device
 */
void jniutils_vibrate_device(jint intensity);
struct frame_dimensions jniutils_get_frame_dimensions();
void jniutils_get_storage_path(char*);

#ifdef __cplusplus
}
#endif

#endif
