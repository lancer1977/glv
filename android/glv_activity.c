/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Dec 20, 2012 - Modified android_native_app_glue for GLV by Karl Robillard.
 */

#include <jni.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include "glv_activity.h"
#include <android/log.h>


#define LOG_TAG "glv_activity"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

/* For debug builds, always enable the debug traces in this library */
#ifndef NDEBUG
#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#else
#  define LOGV(...)  ((void)0)
#endif

#define USE_DPAD    1


extern void glv_initEGL( GLView*, ANativeWindow* );
extern void glv_freeEGL( GLView* );

struct android_app* gGlvApp = 0;


static void free_saved_state(struct android_app* android_app) {
    pthread_mutex_lock(&android_app->mutex);
    if (android_app->savedState != NULL) {
        free(android_app->savedState);
        android_app->savedState = NULL;
        android_app->savedStateSize = 0;
    }
    pthread_mutex_unlock(&android_app->mutex);
}

int8_t android_app_read_cmd(struct android_app* android_app) {
    int8_t cmd;
    if (read(android_app->msgread, &cmd, sizeof(cmd)) == sizeof(cmd)) {
        switch (cmd) {
            case APP_CMD_SAVE_STATE:
                free_saved_state(android_app);
                break;
        }
        return cmd;
    } else {
        LOGE("No data on command pipe!");
    }
    return -1;
}

static void print_cur_config(struct android_app* android_app) {
    char lang[2], country[2];
    AConfiguration_getLanguage(android_app->config, lang);
    AConfiguration_getCountry(android_app->config, country);

    LOGV("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
            "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
            "modetype=%d modenight=%d",
            AConfiguration_getMcc(android_app->config),
            AConfiguration_getMnc(android_app->config),
            lang[0], lang[1], country[0], country[1],
            AConfiguration_getOrientation(android_app->config),
            AConfiguration_getTouchscreen(android_app->config),
            AConfiguration_getDensity(android_app->config),
            AConfiguration_getKeyboard(android_app->config),
            AConfiguration_getNavigation(android_app->config),
            AConfiguration_getKeysHidden(android_app->config),
            AConfiguration_getNavHidden(android_app->config),
            AConfiguration_getSdkVersion(android_app->config),
            AConfiguration_getScreenSize(android_app->config),
            AConfiguration_getScreenLong(android_app->config),
            AConfiguration_getUiModeType(android_app->config),
            AConfiguration_getUiModeNight(android_app->config));
}

void android_app_pre_exec_cmd(struct android_app* android_app, int8_t cmd) {
    switch (cmd) {
        case APP_CMD_INPUT_CHANGED:
            LOGV("APP_CMD_INPUT_CHANGED\n");
            pthread_mutex_lock(&android_app->mutex);
            if (android_app->inputQueue != NULL) {
                AInputQueue_detachLooper(android_app->inputQueue);
            }
            android_app->inputQueue = android_app->pendingInputQueue;
            if (android_app->inputQueue != NULL) {
                LOGV("Attaching input queue to looper");
                AInputQueue_attachLooper(android_app->inputQueue,
                        android_app->looper, LOOPER_ID_INPUT, NULL,
                        &android_app->inputPollSource);
            }
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_INIT_WINDOW:
            LOGV("APP_CMD_INIT_WINDOW\n");
            pthread_mutex_lock(&android_app->mutex);
            android_app->window = android_app->pendingWindow;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_TERM_WINDOW:
            LOGV("APP_CMD_TERM_WINDOW\n");
            pthread_cond_broadcast(&android_app->cond);
            break;

        case APP_CMD_RESUME:
        case APP_CMD_START:
        case APP_CMD_PAUSE:
        case APP_CMD_STOP:
            LOGV("activityState=%d\n", cmd);
            pthread_mutex_lock(&android_app->mutex);
            android_app->activityState = cmd;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_CONFIG_CHANGED:
            LOGV("APP_CMD_CONFIG_CHANGED\n");
            AConfiguration_fromAssetManager(android_app->config,
                    android_app->activity->assetManager);
            print_cur_config(android_app);
            break;

        case APP_CMD_DESTROY:
            LOGV("APP_CMD_DESTROY\n");
            android_app->destroyRequested = 1;
            break;
    }
}

void android_app_post_exec_cmd(struct android_app* android_app, int8_t cmd) {
    switch (cmd) {
        case APP_CMD_TERM_WINDOW:
            LOGV("APP_CMD_TERM_WINDOW\n");
            pthread_mutex_lock(&android_app->mutex);
            android_app->window = NULL;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_SAVE_STATE:
            LOGV("APP_CMD_SAVE_STATE\n");
            pthread_mutex_lock(&android_app->mutex);
            android_app->stateSaved = 1;
            pthread_cond_broadcast(&android_app->cond);
            pthread_mutex_unlock(&android_app->mutex);
            break;

        case APP_CMD_RESUME:
            free_saved_state(android_app);
            break;
    }
}

#ifndef GLV_H
void app_dummy() {

}
#endif

static void android_app_destroy(struct android_app* android_app) {
    LOGV("android_app_destroy!");
    free_saved_state(android_app);
    pthread_mutex_lock(&android_app->mutex);
    if (android_app->inputQueue != NULL) {
        AInputQueue_detachLooper(android_app->inputQueue);
    }
    AConfiguration_delete(android_app->config);
    android_app->destroyed = 1;
    pthread_cond_broadcast(&android_app->cond);
    pthread_mutex_unlock(&android_app->mutex);
    // Can't touch android_app object after this.
}

RQUEUE_DECLARE(GLViewEvent);
#define appendEvent()   rqueue_append_GLViewEvent(&app->eventQueue)

static void process_input(struct android_app* app) {
    AInputEvent* event = NULL;
    GLViewEvent* ve;
    int32_t type;
    int32_t isrc;
    int32_t handled;

    while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
        type = AInputEvent_getType( event );
        isrc = AInputEvent_getSource( event );
        LOGV("New input event: type=%d source=0x%08X\n", type, isrc);
        if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
            continue;
        }
        handled = 0;

        switch (type)
        {
            case AINPUT_EVENT_TYPE_KEY:
            {
                switch( AKeyEvent_getAction( event ) ) {
                    case AKEY_EVENT_ACTION_DOWN:
                        type = GLV_EVENT_KEY_DOWN;
                        break;
                    case AKEY_EVENT_ACTION_UP:
                        type = GLV_EVENT_KEY_UP;
                        break;
                    case AKEY_EVENT_ACTION_MULTIPLE:
                        type = GLV_EVENT_KEY_DOWN;
                        break;
                    default:
                        type = 0;
                        break;
                }

                if( type ) {
                    ve = appendEvent();
                    ve->type  = type;
                    ve->code  = AKeyEvent_getKeyCode( event );
                    //        = AKeyEvent_getScanCode( event );
                    ve->state = AKeyEvent_getMetaState( event );
                    //        = AKeyEvent_getFlags( event );
                    ve->x     = 0;
                    ve->y     = 0;

                    handled = 1;
                }
            }
                break;

            case AINPUT_EVENT_TYPE_MOTION:
            {
                int32_t action = AMotionEvent_getAction( event );
                size_t count = AMotionEvent_getPointerCount( event );
                int state;

                //LOGV("motion action: 0x%X button-state: 0x%X pcount: %lu\n",
                //        action, AMotionEvent_getButtonState(event), count);

#ifdef USE_DPAD
                const DPadDetector* dpad = &app->dpad;
                if( dpad_detect(&app->dpad, event, app->dpFactor) ) {
                    ve = appendEvent();
                    ve->type  = GLV_EVENT_DPAD;
                    ve->code  = dpad->state;
                    ve->state = dpad->prevState;
                    ve->x     = dpad->originX;
                    ve->y     = dpad->originY;
                }
#endif

                state = pinch_detect(&app->pinch, event);
                if( state > GESTURE_STATE_NONE ) {
                    float p1[2], p2[2];
                    if( pinch_positions(&app->pinch, event, p1, p2) ) {
                        //fprintf( stderr, "KR pinch %d %f,%f\n",
                        //         state, p2[0]-p1[0], p2[1]-p1[1] );

                        ve = appendEvent();
                        ve->type  = GLV_EVENT_PINCH;
                        ve->code  = state;
                        ve->state = 0;
                        *((float*) &ve->x) = p2[0] - p1[0];
                        *((float*) &ve->y) = p2[1] - p1[1];
                    }
                }

                // Only support single touch for mouse emulation.
                if( count > 1 )
                    break;

                switch( action & AMOTION_EVENT_ACTION_MASK ) {
                    case AMOTION_EVENT_ACTION_DOWN:
                        type = GLV_EVENT_BUTTON_DOWN;
                        break;
                    case AMOTION_EVENT_ACTION_UP:
                        type = GLV_EVENT_BUTTON_UP;
                        break;
                    case AMOTION_EVENT_ACTION_MOVE:
                        type = GLV_EVENT_MOTION;
                        break;
                    default:
                        type = 0;
                        break;
                }

                if( type ) {
                    ve = appendEvent();
                    ve->type  = type;
                    ve->code  = (type == GLV_EVENT_MOTION) ? 0 : GLV_BUTTON_LEFT;
                    ve->state = AMotionEvent_getButtonState( event ) << 4;
                    //        = AMotionEvent_getFlags( event );
                    ve->x     = (int) AMotionEvent_getX( event, 0 );
                    ve->y     = (int) AMotionEvent_getY( event, 0 );

                    handled = 1;
                }
            }
                break;
        }

        AInputQueue_finishEvent(app->inputQueue, event, handled);
    }
}

static void process_cmd(struct android_app* app) {
    GLViewEvent* ve;
    int cmd = android_app_read_cmd(app);
    android_app_pre_exec_cmd(app, cmd);

    switch( cmd )
    {
#if 0
        case APP_CMD_SAVE_STATE:
            glv->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)glv->app->savedState) = glv->state;
            glv->app->savedStateSize = sizeof(struct saved_state);
            break;
#endif
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if( app->window != NULL && app->view.appRef )
            {
                glv_initEGL( &app->view, app->window );
                //engine_draw_frame(glv);

                /*
                ve->type = GLV_EVENT_RESIZE;
                ve->x    = view->width;
                ve->y    = view->height;
                goto dispatch;
                */
            }
            break;

        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            if( app->view.appRef )
            {
                glv_freeEGL( &app->view );
            }
            break;

        case APP_CMD_GAINED_FOCUS:
            // Reset motion detectors.
            {
                int32_t density = AConfiguration_getDensity(app->config);
                if (density > ACONFIGURATION_DENSITY_DEFAULT &&
                    density < ACONFIGURATION_DENSITY_ANY)
                    app->dpFactor = 160.0f / density;
                else
                    app->dpFactor = 1.0f;
                //fprintf(stderr, "KR density %d %f\n", density, app->dpFactor);
#ifdef USE_DPAD
                dpad_init(&app->dpad);
#endif
                pinch_init(&app->pinch);
            }

            ve = appendEvent();
            ve->type = GLV_EVENT_FOCUS_IN;
            goto dispatch;
#if 0
            // When our app gains focus, we start monitoring the accelerometer.
            if (glv->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(glv->sensorEventQueue,
                        glv->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(glv->sensorEventQueue,
                        glv->accelerometerSensor, (1000L/60)*1000);
            }
#endif

        case APP_CMD_LOST_FOCUS:
            ve = appendEvent();
            ve->type = GLV_EVENT_FOCUS_OUT;
            goto dispatch;
#if 0
            // When our app gains focus, we start monitoring the accelerometer.
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (glv->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(glv->sensorEventQueue,
                        glv->accelerometerSensor);
            }
            // Also stop animating.
            glv->animating = 0;
            engine_draw_frame(glv);
#endif
    }

    if( cmd > -1 )
    {
        ve = appendEvent();
        ve->type  = GLV_EVENT_APP;
        ve->code  = cmd;
        ve->state = 0;
        ve->x     = 0;
        ve->y     = 0;
//dispatch:
        //app->view.eventHandler( &app->view, &ve );
    }

dispatch:
    android_app_post_exec_cmd(app, cmd);
}

#ifdef GLV_H
int android_app_wait_window(struct android_app* app) {
    int i = 0;
    while( app->window == NULL )
    {
        if( ++i > 20 )
            return 0;
        //LOGI( "KR wait_window\n" );
        process_cmd( app );
    }
    if( app->view.display == EGL_NO_DISPLAY )
        glv_initEGL( &app->view, app->window );
    return 1;
}

/*
void android_app_wait_state(struct android_app* app, int cmd) {
    while( app->activityState != cmd ) {
        process_cmd( app );
    }
}
*/

void android_app_wait_destroy(struct android_app* app) {
    int cmd;
    do {
        cmd = android_app_read_cmd(app);
        android_app_pre_exec_cmd(app, cmd);
        android_app_post_exec_cmd(app, cmd);
    } while( cmd != APP_CMD_DESTROY );
}
#endif

static void* android_app_entry(void* param) {
    struct android_app* android_app = (struct android_app*)param;

    android_app->config = AConfiguration_new();
    AConfiguration_fromAssetManager(android_app->config, android_app->activity->assetManager);

    print_cur_config(android_app);

    android_app->cmdPollSource.id = LOOPER_ID_MAIN;
    android_app->cmdPollSource.app = android_app;
    android_app->cmdPollSource.process = process_cmd;
    android_app->inputPollSource.id = LOOPER_ID_INPUT;
    android_app->inputPollSource.app = android_app;
    android_app->inputPollSource.process = process_input;

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd(looper, android_app->msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, NULL,
            &android_app->cmdPollSource);
    android_app->looper = looper;

    pthread_mutex_lock(&android_app->mutex);
    android_app->running = 1;
    pthread_cond_broadcast(&android_app->cond);
    pthread_mutex_unlock(&android_app->mutex);

    android_main(android_app);

    android_app_destroy(android_app);
    return NULL;
}

// --------------------------------------------------------------------
// Native activity interaction (called from main thread)
// --------------------------------------------------------------------

extern void glv_nullHandler( void*, GLViewEvent* );

static struct android_app* android_app_create(ANativeActivity* activity,
        void* savedState, size_t savedStateSize) {
    struct android_app* android_app = (struct android_app*)malloc(sizeof(struct android_app));
    memset(android_app, 0, sizeof(struct android_app));
    android_app->activity = activity;

    pthread_mutex_init(&android_app->mutex, NULL);
    pthread_cond_init(&android_app->cond, NULL);

    if (savedState != NULL) {
        android_app->savedState = malloc(savedStateSize);
        android_app->savedStateSize = savedStateSize;
        memcpy(android_app->savedState, savedState, savedStateSize);
    }

    int msgpipe[2];
    if (pipe(msgpipe)) {
        LOGE("could not create pipe: %s", strerror(errno));
        return NULL;
    }
    android_app->msgread = msgpipe[0];
    android_app->msgwrite = msgpipe[1];

#ifdef GLV_H
    // Init GLV data before creating app thread.
    gGlvApp = android_app;
    rqueue_init(&android_app->eventQueue, 8, sizeof(GLViewEvent));
    android_app->view.eventHandler = glv_nullHandler;
#endif

    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&android_app->thread, &attr, android_app_entry, android_app);

    // Wait for thread to start.
    pthread_mutex_lock(&android_app->mutex);
    while (!android_app->running) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }
    pthread_mutex_unlock(&android_app->mutex);

    return android_app;
}

static void android_app_write_cmd(struct android_app* android_app, int8_t cmd) {
    if (write(android_app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd)) {
        LOGE("Failure writing android_app cmd: %s\n", strerror(errno));
    }
}

static void android_app_set_input(struct android_app* android_app, AInputQueue* inputQueue) {
    pthread_mutex_lock(&android_app->mutex);
    android_app->pendingInputQueue = inputQueue;
    android_app_write_cmd(android_app, APP_CMD_INPUT_CHANGED);
    while (android_app->inputQueue != android_app->pendingInputQueue) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }
    pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_set_window(struct android_app* android_app, ANativeWindow* window) {
    pthread_mutex_lock(&android_app->mutex);
    if (android_app->pendingWindow != NULL) {
        android_app_write_cmd(android_app, APP_CMD_TERM_WINDOW);
    }
    android_app->pendingWindow = window;
    if (window != NULL) {
        android_app_write_cmd(android_app, APP_CMD_INIT_WINDOW);
    }
    while (android_app->window != android_app->pendingWindow) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }
    pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_set_activity_state(struct android_app* android_app, int8_t cmd) {
    pthread_mutex_lock(&android_app->mutex);
    android_app_write_cmd(android_app, cmd);
    while (android_app->activityState != cmd) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }
    pthread_mutex_unlock(&android_app->mutex);
}

static void android_app_free(struct android_app* android_app) {
    // Ensure GLView cleanup even if user fails to do so.
    glv_destroy( &android_app->view );

    pthread_mutex_lock(&android_app->mutex);
    android_app_write_cmd(android_app, APP_CMD_DESTROY);
    while (!android_app->destroyed) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }
    pthread_mutex_unlock(&android_app->mutex);

    rqueue_free(&android_app->eventQueue);

    close(android_app->msgread);
    close(android_app->msgwrite);
    pthread_cond_destroy(&android_app->cond);
    pthread_mutex_destroy(&android_app->mutex);
    free(android_app);

    gGlvApp = 0;
}

static void onDestroy(ANativeActivity* activity) {
    LOGV("Destroy: %p\n", activity);
    android_app_free((struct android_app*)activity->instance);
}

static void onStart(ANativeActivity* activity) {
    LOGV("Start: %p\n", activity);
    android_app_set_activity_state((struct android_app*)activity->instance, APP_CMD_START);
}

static void onResume(ANativeActivity* activity) {
    LOGV("Resume: %p\n", activity);
    android_app_set_activity_state((struct android_app*)activity->instance, APP_CMD_RESUME);
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen) {
    struct android_app* android_app = (struct android_app*)activity->instance;
    void* savedState = NULL;

    LOGV("SaveInstanceState: %p\n", activity);
    pthread_mutex_lock(&android_app->mutex);
    android_app->stateSaved = 0;
    android_app_write_cmd(android_app, APP_CMD_SAVE_STATE);
    while (!android_app->stateSaved) {
        pthread_cond_wait(&android_app->cond, &android_app->mutex);
    }

    if (android_app->savedState != NULL) {
        savedState = android_app->savedState;
        *outLen = android_app->savedStateSize;
        android_app->savedState = NULL;
        android_app->savedStateSize = 0;
    }

    pthread_mutex_unlock(&android_app->mutex);

    return savedState;
}

static void onPause(ANativeActivity* activity) {
    LOGV("Pause: %p\n", activity);
    android_app_set_activity_state((struct android_app*)activity->instance, APP_CMD_PAUSE);
}

static void onStop(ANativeActivity* activity) {
    LOGV("Stop: %p\n", activity);
    android_app_set_activity_state((struct android_app*)activity->instance, APP_CMD_STOP);
}

static void onConfigurationChanged(ANativeActivity* activity) {
    LOGV("ConfigurationChanged: %p\n", activity);
    android_app_write_cmd((struct android_app*)activity->instance, APP_CMD_CONFIG_CHANGED);
}

static void onLowMemory(ANativeActivity* activity) {
    LOGV("LowMemory: %p\n", activity);
    android_app_write_cmd((struct android_app*)activity->instance, APP_CMD_LOW_MEMORY);
}

static void onWindowFocusChanged(ANativeActivity* activity, int focused) {
    LOGV("WindowFocusChanged: %p -- %d\n", activity, focused);
    android_app_write_cmd((struct android_app*)activity->instance,
            focused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    LOGV("NativeWindowCreated: %p -- %p\n", activity, window);
    android_app_set_window((struct android_app*)activity->instance, window);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    LOGV("NativeWindowDestroyed: %p -- %p\n", activity, window);
    android_app_set_window((struct android_app*)activity->instance, NULL);
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    LOGV("InputQueueCreated: %p -- %p\n", activity, queue);
    android_app_set_input((struct android_app*)activity->instance, queue);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    LOGV("InputQueueDestroyed: %p -- %p\n", activity, queue);
    android_app_set_input((struct android_app*)activity->instance, NULL);
}

JNIEXPORT
void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState,
                              size_t savedStateSize) {
    LOGV("Creating: %p\n", activity);
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onStart = onStart;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onSaveInstanceState = onSaveInstanceState;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onConfigurationChanged = onConfigurationChanged;
    activity->callbacks->onLowMemory = onLowMemory;
    activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;

    activity->instance = android_app_create(activity, savedState, savedStateSize);
}

void glv_showSoftInput( GLView* view, int visible )
{
    ANativeActivity* activity = ((struct android_app*) view)->activity;
#if 0
    // This showSoftInput call does nothing...
    ANativeActivity_showSoftInput( activity,
            //ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT
            ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED
            );
#else
    /* The NativeActivity must be extended with these methods:
    public void showKeyboard()
    {
        InputMethodManager imm = (InputMethodManager)
            getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.showSoftInput( this.getWindow().getDecorView(),
            InputMethodManager.SHOW_FORCED );
    }
    public void hideKeyboard()
    {
        InputMethodManager imm = (InputMethodManager)
            getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.hideSoftInputFromWindow(
            this.getWindow().getDecorView().getWindowToken(), 0 );
    }
    */

    JavaVMAttachArgs vmArgs;
    const char* method;
    JavaVM* pJavaVM = activity->vm;
    JNIEnv* pEnv;
    jint result;

    // Attach the current thread to the JVM.  Can't use activity->env as
    // glv_ calls will not be in the ANativeActivityCallbacks thread.

    vmArgs.version = JNI_VERSION_1_6;
    vmArgs.name    = "NativeThread";
    vmArgs.group   = NULL;

    result = (*pJavaVM)->AttachCurrentThread( pJavaVM, &pEnv, &vmArgs );
    if( result != JNI_ERR )
    {
        // Retrieve NativeActivity class.
        // NOTE: activity->clazz is actually a jobject, not a jclass.
        jclass class = (*pEnv)->GetObjectClass( pEnv, activity->clazz );

        method = visible ? "showKeyboard" : "hideKeyboard";
        jmethodID mid = (*pEnv)->GetMethodID( pEnv, class, method, "()V" );
        (*pEnv)->CallVoidMethod( pEnv, activity->clazz, mid );

        // Finished with the JVM.
        (*pJavaVM)->DetachCurrentThread( pJavaVM );
    }
#endif
}

#ifdef USE_DPAD
void glv_setDPadRect( GLView* view, int pad, const float* rect )
{
    if( pad == 0 ) {
        float* dest = ((struct android_app*) view)->dpad.rect;
        memcpy(dest, rect, 4 * sizeof(float));
    }
}
#endif
