#include "jniutils.h"
#include "SDL.h"

#define ACTIVITY_CLS_NAME "GameActivity"

extern "C"
{
    #include "video.h"
}

extern "C" JNIEXPORT void JNICALL Java_org_openbor_engine_GameActivity_fireSystemUiVisibilityChangeEvent(JNIEnv* env, jobject obj, jint isSystemBarsVisible)
{
    return on_system_ui_visibility_change_event(isSystemBarsVisible);
}

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

struct frame_dimensions jniutils_get_frame_dimensions()
{
    // retrieve the JNI environment
   JNIEnv  *env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    // retrieve the Java instance of the GameActivity
    jobject activity = (jobject)SDL_AndroidGetActivity();

    // find the Java class of the activity. It should be GameActivity.
    jclass cls = env->GetObjectClass(activity);

    // find the identifier of the method to call
    jmethodID method_id = env->GetStaticMethodID(cls, "jni_get_frame_dimensions", "()Lorg/openbor/engine/utils/FrameDimensions;");

    // effectively call the Java method
    jobject jData = env->CallStaticObjectMethod(cls, method_id);

    // get the class
    jclass javaDataClass = env->FindClass("org/openbor/engine/utils/FrameDimensions");

    // get the field id
    jfieldID xFieldId = env->GetFieldID(javaDataClass, "x", "I");
    jfieldID yFieldId = env->GetFieldID(javaDataClass, "y", "I");
    jfieldID widthFieldId = env->GetFieldID(javaDataClass, "width", "I");
    jfieldID heightFieldId = env->GetFieldID(javaDataClass, "height", "I");
    jfieldID topFieldId = env->GetFieldID(javaDataClass, "top", "I");
    jfieldID leftFieldId = env->GetFieldID(javaDataClass, "left", "I");
    jfieldID bottomFieldId = env->GetFieldID(javaDataClass, "bottom", "I");
    jfieldID rightFieldId = env->GetFieldID(javaDataClass, "right", "I");

    struct frame_dimensions frm_dim;

    // get the data from the field
    frm_dim.x = env->GetIntField(jData, xFieldId);
    frm_dim.y = env->GetIntField(jData, yFieldId);
    frm_dim.width = env->GetIntField(jData, widthFieldId);
    frm_dim.height = env->GetIntField(jData, heightFieldId);
    frm_dim.top = env->GetIntField(jData, topFieldId);
    frm_dim.left = env->GetIntField(jData, leftFieldId);
    frm_dim.bottom = env->GetIntField(jData, bottomFieldId);
    frm_dim.right = env->GetIntField(jData, rightFieldId);

    // clean up the local references
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);

    return frm_dim;
}

