#include "jniutils.h"
#include "SDL.h"

#define ACTIVITY_CLS_NAME "GameActivity"

void jniutils_vibrate_device(jint intensity)
{
  // retrieve the JNI environment
  JNIEnv  *env = (JNIEnv*)SDL_AndroidGetJNIEnv();

  // retrieve the Java instance of the GameActivity
  jobject activity = (jobject)SDL_AndroidGetActivity();

  // find the Java class of the activity. It should be GameActivity.
  jclass cls = env->GetObjectClass(activity);

  // find the identifier of the method to call
  jmethodID method_id = env->GetStaticMethodID(cls, "jni_vibrate", "(I)V");

  // effectively call the Java method
  env->CallStaticVoidMethod(cls, method_id, intensity);

  // clean up the local references
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity);
}

struct frame_borders jniutils_get_frame_borders()
{
    // retrieve the JNI environment
   JNIEnv  *env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    // retrieve the Java instance of the GameActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();

    // find the Java class of the activity. It should be GameActivity.
    jclass cls = env->GetObjectClass(activity);

    // find the identifier of the method to call
    jmethodID method_id = env->GetStaticMethodID(cls, "jni_get_frame_borders", "()Lorg/openbor/engine/utils/FrameBorders;");

    // effectively call the Java method
    jobject jData = env->CallStaticObjectMethod(cls, method_id);

    // get the class
    jclass javaDataClass = env->FindClass("org/openbor/engine/utils/FrameBorders");

    // get the field id
    jfieldID topFieldId = env->GetFieldID(javaDataClass, "top", "I");
    jfieldID leftFieldId = env->GetFieldID(javaDataClass, "left", "I");
    jfieldID bottomFieldId = env->GetFieldID(javaDataClass, "bottom", "I");
    jfieldID rightFieldId = env->GetFieldID(javaDataClass, "right", "I");

    struct frame_borders frm_borders;

    // get the data from the field
    frm_borders.top = env->GetIntField(jData, topFieldId);
    frm_borders.left = env->GetIntField(jData, leftFieldId);
    frm_borders.bottom = env->GetIntField(jData, bottomFieldId);
    frm_borders.right = env->GetIntField(jData, rightFieldId);

    // clean up the local references
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);

    return frm_borders;
}

