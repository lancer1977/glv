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
#include <android/log.h>
#include "glv_activity.h"


#define LOG_TAG "glv_activity"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

/* For debug builds, always enable the debug traces in this library */
#ifndef NDEBUG
#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#else
#  define LOGV(...)  ((void)0)
#endif


extern void glv_initEGL( GLView*, ANativeWindow* );
extern void glv_freeEGL( GLView* );


struct android_app* gGlvApp = 0;

static void free_saved_state( struct android_app* app )
{
    pthread_mutex_lock(&app->mutex);
    if( app->savedState != NULL )
    {
        free(app->savedState);
        app->savedState = NULL;
        app->savedStateSize = 0;
    }
    pthread_mutex_unlock(&app->mutex);
}

int8_t android_app_read_cmd( struct android_app* app )
{
    int8_t cmd;
    if( read(app->msgread, &cmd, sizeof(cmd)) == sizeof(cmd) )
    {
        switch( cmd )
        {
            case APP_CMD_SAVE_STATE:
                free_saved_state(app);
                break;
        }
        return cmd;
    }
    else
    {
        LOGE("No data on command pipe!");
    }
    return -1;
}

static void print_cur_config( struct android_app* app )
{
    char lang[2], country[2];
    AConfiguration_getLanguage(app->config, lang);
    AConfiguration_getCountry(app->config, country);

    LOGV("Config: mcc=%d mnc=%d lang=%c%c cnt=%c%c orien=%d touch=%d dens=%d "
            "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d long=%d "
            "modetype=%d modenight=%d",
            AConfiguration_getMcc(app->config),
            AConfiguration_getMnc(app->config),
            lang[0], lang[1], country[0], country[1],
            AConfiguration_getOrientation(app->config),
            AConfiguration_getTouchscreen(app->config),
            AConfiguration_getDensity(app->config),
            AConfiguration_getKeyboard(app->config),
            AConfiguration_getNavigation(app->config),
            AConfiguration_getKeysHidden(app->config),
            AConfiguration_getNavHidden(app->config),
            AConfiguration_getSdkVersion(app->config),
            AConfiguration_getScreenSize(app->config),
            AConfiguration_getScreenLong(app->config),
            AConfiguration_getUiModeType(app->config),
            AConfiguration_getUiModeNight(app->config));
}

void android_app_pre_exec_cmd( struct android_app* app, int8_t cmd )
{
    switch( cmd )
    {
        case APP_CMD_INPUT_CHANGED:
            LOGV("APP_CMD_INPUT_CHANGED\n");
            pthread_mutex_lock(&app->mutex);
            if (app->inputQueue != NULL) {
                AInputQueue_detachLooper(app->inputQueue);
            }
            app->inputQueue = app->pendingInputQueue;
            if (app->inputQueue != NULL) {
                LOGV("Attaching input queue to looper");
                AInputQueue_attachLooper(app->inputQueue,
                        app->looper, LOOPER_ID_INPUT, NULL,
                        &app->inputPollSource);
            }
            pthread_cond_broadcast(&app->cond);
            pthread_mutex_unlock(&app->mutex);
            break;

        case APP_CMD_INIT_WINDOW:
            LOGV("APP_CMD_INIT_WINDOW\n");
            pthread_mutex_lock(&app->mutex);
            app->window = app->pendingWindow;
            pthread_cond_broadcast(&app->cond);
            pthread_mutex_unlock(&app->mutex);
            break;

        case APP_CMD_TERM_WINDOW:
            LOGV("APP_CMD_TERM_WINDOW\n");
            pthread_cond_broadcast(&app->cond);
            break;

        case APP_CMD_RESUME:
        case APP_CMD_START:
        case APP_CMD_PAUSE:
        case APP_CMD_STOP:
            LOGV("activityState=%d\n", cmd);
            pthread_mutex_lock(&app->mutex);
            app->activityState = cmd;
            pthread_cond_broadcast(&app->cond);
            pthread_mutex_unlock(&app->mutex);
            break;

        case APP_CMD_CONFIG_CHANGED:
            LOGV("APP_CMD_CONFIG_CHANGED\n");
            AConfiguration_fromAssetManager(app->config,
                    app->activity->assetManager);
            print_cur_config(app);
            break;

        case APP_CMD_DESTROY:
            LOGV("APP_CMD_DESTROY\n");
            app->destroyRequested = 1;
            break;
    }
}

void android_app_post_exec_cmd( struct android_app* app, int8_t cmd )
{
    switch( cmd )
    {
        case APP_CMD_TERM_WINDOW:
            LOGV("APP_CMD_TERM_WINDOW\n");
            pthread_mutex_lock(&app->mutex);
            app->window = NULL;
            pthread_cond_broadcast(&app->cond);
            pthread_mutex_unlock(&app->mutex);
            break;

        case APP_CMD_SAVE_STATE:
            LOGV("APP_CMD_SAVE_STATE\n");
            pthread_mutex_lock(&app->mutex);
            app->stateSaved = 1;
            pthread_cond_broadcast(&app->cond);
            pthread_mutex_unlock(&app->mutex);
            break;

        case APP_CMD_RESUME:
            free_saved_state(app);
            break;
    }
}

static void android_app_destroy( struct android_app* app )
{
    LOGV("android_app_destroy!");
    free_saved_state(app);
    pthread_mutex_lock(&app->mutex);
    if (app->inputQueue != NULL) {
        AInputQueue_detachLooper(app->inputQueue);
    }
    AConfiguration_delete(app->config);
    app->destroyed = 1;
    pthread_cond_broadcast(&app->cond);
    pthread_mutex_unlock(&app->mutex);
    // Can't touch android_app object after this.
}

static void process_input( struct android_app* app,
                           struct android_poll_source* source )
{
    AInputEvent* ie = NULL;

    if( AInputQueue_getEvent(app->inputQueue, &ie) >= 0 )
    {
        GLViewEvent ve;
        int32_t type = AInputEvent_getType( ie );
        int32_t handled = 0;

        LOGV("New input event: type=%d\n", type);
        if( AInputQueue_preDispatchEvent(app->inputQueue, ie) )
            return;

        switch( type )
        {
            case AINPUT_EVENT_TYPE_KEY:
            {
                GLView* view = &app->view;

                switch( AKeyEvent_getAction( ie ) )
                {
                    case AKEY_EVENT_ACTION_DOWN:
                        ve.type = GLV_EVENT_KEY_DOWN;
                        break;
                    case AKEY_EVENT_ACTION_UP:
                        ve.type = GLV_EVENT_KEY_UP;
                        break;
                    case AKEY_EVENT_ACTION_MULTIPLE:
                        ve.type = GLV_EVENT_KEY_DOWN;
                        break;
                }

                ve.code  = AKeyEvent_getKeyCode( ie );
                ve.state = AKeyEvent_getFlags( ie );
                ve.x     = 0;
                ve.y     = 0;

                view->eventHandler( view, &ve );
                handled = 1;
            }
                break;

            case AINPUT_EVENT_TYPE_MOTION:
            {
                GLView* view = &app->view;

                ve.type  = GLV_EVENT_MOTION;
                ve.code  = 0;
                ve.state = AMotionEvent_getFlags( ie );
                ve.x     = (int) AMotionEvent_getX( ie, 0 );
                ve.y     = (int) AMotionEvent_getY( ie, 0 );

                view->eventHandler( view, &ve );
                handled = 1;
            }
                break;
        }

        AInputQueue_finishEvent(app->inputQueue, ie, handled);
    }
    else
    {
        LOGE("Failure reading next input event: %s\n", strerror(errno));
    }
}

static void process_cmd( struct android_app* app,
                         struct android_poll_source* source )
{
    GLViewEvent ve;
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
            ve.type = GLV_EVENT_FOCUS_IN;
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
            ve.type = GLV_EVENT_FOCUS_OUT;
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
        ve.type  = GLV_EVENT_APP;
        ve.code  = cmd;
        ve.state = 0;
        ve.x     = 0;
        ve.y     = 0;
dispatch:
        app->view.eventHandler( &app->view, &ve );
    }

    android_app_post_exec_cmd(app, cmd);
}

int android_app_wait_window( struct android_app* app )
{
    int i = 0;
    while( app->window == NULL )
    {
        if( ++i > 20 )
            return 0;
        //LOGI( "KR wait_window\n" );
        process_cmd( app, &app->cmdPollSource );
    }
    if( app->view.display == EGL_NO_DISPLAY )
        glv_initEGL( &app->view, app->window );
    return 1;
}

static void* android_app_entry(void* param)
{
    struct android_app* app = (struct android_app*)param;

    app->config = AConfiguration_new();
    AConfiguration_fromAssetManager(app->config,
                                    app->activity->assetManager);

    print_cur_config(app);

    app->cmdPollSource.id = LOOPER_ID_MAIN;
    app->cmdPollSource.app = app;
    app->cmdPollSource.process = process_cmd;

    app->inputPollSource.id = LOOPER_ID_INPUT;
    app->inputPollSource.app = app;
    app->inputPollSource.process = process_input;

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd( looper, app->msgread, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT,
                   NULL, &app->cmdPollSource );
    app->looper = looper;

    pthread_mutex_lock(&app->mutex);
    app->running = 1;
    pthread_cond_broadcast(&app->cond);
    pthread_mutex_unlock(&app->mutex);

    android_main(app);

    android_app_destroy(app);
    return NULL;
}

// --------------------------------------------------------------------
// Native activity interaction (called from main thread)
// --------------------------------------------------------------------

extern void glv_nullHandler( void*, GLViewEvent* );

static struct android_app* android_app_create( ANativeActivity* activity,
        void* savedState, size_t savedStateSize )
{
    struct android_app* app = (struct android_app*)
                                malloc( sizeof(struct android_app) );
    memset(app, 0, sizeof(struct android_app));
    app->activity = activity;

    pthread_mutex_init(&app->mutex, NULL);
    pthread_cond_init(&app->cond, NULL);

    if (savedState != NULL) {
        app->savedState = malloc(savedStateSize);
        app->savedStateSize = savedStateSize;
        memcpy(app->savedState, savedState, savedStateSize);
    }

    int msgpipe[2];
    if (pipe(msgpipe)) {
        LOGE("could not create pipe: %s", strerror(errno));
        return NULL;
    }
    app->msgread = msgpipe[0];
    app->msgwrite = msgpipe[1];

    // Init GLV data before creating app thread.
    gGlvApp = app;
    app->view.eventHandler = glv_nullHandler;

    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&app->thread, &attr, android_app_entry, app);

    // Wait for thread to start.
    pthread_mutex_lock(&app->mutex);
    while( ! app->running )
        pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);

    return app;
}

static void android_app_write_cmd( struct android_app* app, int8_t cmd )
{
    if( write(app->msgwrite, &cmd, sizeof(cmd)) != sizeof(cmd) )
    {
        LOGE("Failure writing android_app cmd: %s\n", strerror(errno));
    }
}

static void android_app_set_input( struct android_app* app,
                                   AInputQueue* inputQueue )
{
    pthread_mutex_lock(&app->mutex);
    app->pendingInputQueue = inputQueue;
    android_app_write_cmd(app, APP_CMD_INPUT_CHANGED);
    while( app->inputQueue != app->pendingInputQueue )
        pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);
}

static void android_app_set_window( struct android_app* app,
                                    ANativeWindow* window )
{
    pthread_mutex_lock(&app->mutex);
    if (app->pendingWindow != NULL) {
        android_app_write_cmd(app, APP_CMD_TERM_WINDOW);
    }
    app->pendingWindow = window;
    if (window != NULL) {
        android_app_write_cmd(app, APP_CMD_INIT_WINDOW);
    }
    while( app->window != app->pendingWindow )
        pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);
}

static void android_app_set_activity_state(struct android_app* app, int8_t cmd)
{
    pthread_mutex_lock(&app->mutex);
    android_app_write_cmd(app, cmd);
    while( app->activityState != cmd )
        pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);
}

#define ACT_APP    (struct android_app*) activity->instance

static void onDestroy( ANativeActivity* activity )
{
    LOGV("Destroy: %p\n", activity);
    struct android_app* app = ACT_APP;

    // Ensure GLView cleanup even if user fails to do so.
    glv_destroy( &app->view );

    pthread_mutex_lock(&app->mutex);
    android_app_write_cmd(app, APP_CMD_DESTROY);
    while( ! app->destroyed )
        pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);

    close(app->msgread);
    close(app->msgwrite);
    pthread_cond_destroy(&app->cond);
    pthread_mutex_destroy(&app->mutex);
    free(app);
    gGlvApp = 0;
}

static void onStart( ANativeActivity* activity )
{
    LOGV("Start: %p\n", activity);
    android_app_set_activity_state( ACT_APP, APP_CMD_START );
}

static void onResume( ANativeActivity* activity )
{
    LOGV("Resume: %p\n", activity);
    android_app_set_activity_state( ACT_APP, APP_CMD_RESUME );
}

static void* onSaveInstanceState( ANativeActivity* activity, size_t* outLen )
{
    struct android_app* app = ACT_APP;
    void* savedState = NULL;

    LOGV("SaveInstanceState: %p\n", activity);
    pthread_mutex_lock(&app->mutex);
    app->stateSaved = 0;
    android_app_write_cmd(app, APP_CMD_SAVE_STATE);
    while( ! app->stateSaved )
        pthread_cond_wait(&app->cond, &app->mutex);

    if (app->savedState != NULL)
    {
        savedState = app->savedState;
        *outLen = app->savedStateSize;
        app->savedState = NULL;
        app->savedStateSize = 0;
    }
    pthread_mutex_unlock(&app->mutex);

    return savedState;
}

static void onPause(ANativeActivity* activity)
{
    LOGV("Pause: %p\n", activity);
    android_app_set_activity_state( ACT_APP, APP_CMD_PAUSE );
}

static void onStop(ANativeActivity* activity)
{
    LOGV("Stop: %p\n", activity);
    android_app_set_activity_state( ACT_APP, APP_CMD_STOP );
}

static void onConfigurationChanged( ANativeActivity* activity )
{
    LOGV("ConfigurationChanged: %p\n", activity);
    android_app_write_cmd( ACT_APP, APP_CMD_CONFIG_CHANGED );
}

static void onLowMemory( ANativeActivity* activity )
{
    LOGV("LowMemory: %p\n", activity);
    android_app_write_cmd( ACT_APP, APP_CMD_LOW_MEMORY);
}

static void onWindowFocusChanged( ANativeActivity* activity, int focused )
{
    LOGV("WindowFocusChanged: %p -- %d\n", activity, focused);
    android_app_write_cmd( ACT_APP,
            focused ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS );
}

static void onNativeWindowCreated( ANativeActivity* activity,
                                   ANativeWindow* window )
{
    LOGV("NativeWindowCreated: %p -- %p\n", activity, window);
    android_app_set_window( ACT_APP, window);
}

static void onNativeWindowDestroyed( ANativeActivity* activity,
                                     ANativeWindow* window )
{
    LOGV("NativeWindowDestroyed: %p -- %p\n", activity, window);
    android_app_set_window( ACT_APP, NULL );
}

static void onInputQueueCreated( ANativeActivity* activity, AInputQueue* queue )
{
    LOGV("InputQueueCreated: %p -- %p\n", activity, queue);
    android_app_set_input( ACT_APP, queue );
}

static void onInputQueueDestroyed( ANativeActivity* activity,
                                   AInputQueue* queue )
{
    LOGV("InputQueueDestroyed: %p -- %p\n", activity, queue);
    android_app_set_input( ACT_APP, NULL );
}

void ANativeActivity_onCreate( ANativeActivity* activity, void* savedState,
                               size_t savedStateSize )
{
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

    activity->instance = android_app_create( activity, savedState,
                                             savedStateSize );
}
