/*
  Adobe Systems Incorporated(r) Source Code License Agreement
  Copyright(c) 2006 Adobe Systems Incorporated. All rights reserved.

  Please read this Source Code License Agreement carefully before using
  the source code.

  Adobe Systems Incorporated grants to you a perpetual, worldwide, non-exclusive,
  no-charge, royalty-free, irrevocable copyright license, to reproduce,
  prepare derivative works of, publicly display, publicly perform, and
  distribute this source code and such derivative works in source or
  object code form without any attribution requirements.

  The name "Adobe Systems Incorporated" must not be used to endorse or promote products
  derived from the source code without prior written permission.

  You agree to indemnify, hold harmless and defend Adobe Systems Incorporated from and
  against any loss, damage, claims or lawsuits, including attorney's
  fees that arise or result from your use or distribution of the source
  code.

  THIS SOURCE CODE IS PROVIDED "AS IS" AND "WITH ALL FAULTS", WITHOUT
  ANY TECHNICAL SUPPORT OR ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. ALSO, THERE IS NO WARRANTY OF
  NON-INFRINGEMENT, TITLE OR QUIET ENJOYMENT. IN NO EVENT SHALL MACROMEDIA
  OR ITS SUPPLIERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOURCE CODE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File name: flashsupport.c
// Targets: Adobe Flash Player 9.1 beta 2 for Linux (9.0.21.78)
// Last Revision Date: 11/20/2006
//

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// These are the feature which can be turned on and off. They are all
// optional. The ALSA and Video4Linux support in this file is somewhat
// redundant and reduced in functionality as the Flash Player already has
// ALSA and Video4Linux support built-in. It is provided here for reference only.
// Also, if your system has ALSA support in the kernel there is no need to
// enable OSS here as it will override it.
//

#define OPENSSL
//#define GNUTLS
//#define ALSA
//#define OSS
#define V4L1
//#define PULSEAUDIO
#define JACK

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// To compile and install flashsupport.c the following components are required:
//
//  - a C compiler (gcc 4.03 is known to be working)
//  - OpenSSL developer package and working user libraries (OpenSSL 0.9.8 is known to be working)
//  - OSS (or ALSA) developer package and working users libraries (Linux 2.6.15 is known to be working)
//  - sudo or root access to install the generated library to /usr/lib
//
// We suggest these steps in a terminal:
//
// > cc -shared -O2 -Wall -Werror -lssl flashsupport.c -o libflashsupport.so
// > ldd libflashplayer.so
// > sudo cp libflashplayer.so /usr/lib
//
// Make sure that 'ldd' can resolve all dynamic libraries. Otherwise the Flash Player
// will silently fail to load and use libflashsupport.so.
//

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SHARED SECTION, DO NOT CHANGE!
//
#ifdef cplusplus
extern "C" {
#endif // cplusplus

//
// Imported functions
//

    typedef void *(*T_FPI_Mem_Alloc)(int size);     // This function is not thread safe
    typedef void (*T_FPI_Mem_Free)(void *ptr); // This function is not thread safe

#if defined(ALSA) || defined(OSS) || defined(PULSEAUDIO) || defined(JACK)
    typedef void (*T_FPI_SoundOutput_FillBuffer)(void *ptr, char *buffer, int n_bytes); // This function is thread safe
#endif // defined(ALSA) || defined(OSS)

    struct FPI_Functions {
        int       fpi_count;
        void *fpi_mem_alloc;                            // 1
        void *fpi_mem_free;                                     // 2
        void *fpi_soundoutput_fillbuffer;       // 3
    };

//
// Exported functions
//

    void *FPX_Init(void *ptr);

    static void FPX_Shutdown(void);

#if defined(OPENSSL) || defined(GNUTLS)
    static void *FPX_SSLSocket_Create(int socket_fd);
    static int FPX_SSLSocket_Destroy(void *ptr);
    static int FPX_SSLSocket_Connect(void *ptr);
    static int FPX_SSLSocket_Receive(void *ptr, char *buffer, int n_bytes);
    static int FPX_SSLSocket_Send(void *ptr, const char *buffer, int n_bytes);
#endif // defined(OPENSSL) || defined(GNUTLS)

#if defined(ALSA) || defined(OSS) || defined(PULSEAUDIO) || defined(JACK)
    static void *FPX_SoundOutput_Open(void);
    static int FPX_SoundOutput_Close(void *ptr);
    static int FPX_SoundOutput_Latency(void *ptr);
#endif // defined(ALSA) || defined(OSS)

#ifdef V4L1
    static void *FPX_VideoInput_Open(void);
    static int FPX_VideoInput_Close(void *ptr);
    static int FPX_VideoInput_GetFrame(void *ptr, char *data, int width, int height, int pitch_n_bytes);
#endif // V4L1

    struct FPX_Functions {
        int   fpx_count;
        void *fpx_shutdown;                                     // 1
        void *fpx_sslsocket_create;                     // 2
        void *fpx_sslsocket_destroy;            // 3
        void *fpx_sslsocket_connect;            // 4
        void *fpx_sslsocket_receive;            // 5
        void *fpx_sslsocket_send;                       // 6
        void *fpx_soundoutput_open;                     // 7
        void *fpx_soundoutput_close;            // 8
        void *fpx_soundoutput_latency;          // 9
        void *fpx_videoinput_open;                      // 10
        void *fpx_videoinput_close;                     // 11
        void *fpx_videoinput_getframe;          // 12
    };

#ifdef cplusplus
};
#endif // cplusplus
//
// END OF SHARED SECTION
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <memory.h>

#ifdef OPENSSL
#include <openssl/ssl.h>
#elif defined(GNUTLS)
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#endif // GNUTLS

#ifdef ALSA
#include <pthread.h>
#include <semaphore.h>
#include <alsa/asoundlib.h>
#elif defined(OSS)
#include <unistd.h>
#include <pthread.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif // OSS

#ifdef V4L1
#include <unistd.h>
#include <pthread.h>
#include "videodev.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#endif // V4L1

#ifdef PULSEAUDIO
#include <pulse/pulseaudio.h>
#endif

#ifdef JACK
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <jack/thread.h>
#include <samplerate.h>
#include <semaphore.h>
#endif

static struct FPX_Functions fpx_functions;

static T_FPI_Mem_Alloc FPI_Mem_Alloc = 0;
static T_FPI_Mem_Free FPI_Mem_Free = 0;

#if defined(ALSA) || defined(OSS) || defined(PULSEAUDIO) || defined(JACK)
static T_FPI_SoundOutput_FillBuffer FPI_SoundOutput_FillBuffer = 0;
#endif // defined(ALSA) || defined(OSS)

void *FPX_Init(void *ptr)
{
    struct FPI_Functions *fpi_functions;
    if ( !ptr ) return 0;

    //
    // Setup imported functions
    //

    fpi_functions = (struct FPI_Functions *)ptr;

    if ( fpi_functions->fpi_count >= 1 )    FPI_Mem_Alloc           = fpi_functions->fpi_mem_alloc;                                         // 1
    if ( fpi_functions->fpi_count >= 2 )    FPI_Mem_Free            = fpi_functions->fpi_mem_free;                                          // 2

#if defined(ALSA) || defined(OSS) || defined(PULSEAUDIO) || defined(JACK)
    if ( fpi_functions->fpi_count >= 3 )    FPI_SoundOutput_FillBuffer= fpi_functions->fpi_soundoutput_fillbuffer;  // 3
#endif // defined(ALSA) || defined(OSS)

    //
    // Setup exported functions
    //

    memset(&fpx_functions, 0, sizeof(fpx_functions));

    fpx_functions.fpx_shutdown                                      = FPX_Shutdown;                                 // 1

#if defined(OPENSSL) || defined(GNUTLS)
    fpx_functions.fpx_sslsocket_create                      = FPX_SSLSocket_Create;                 // 2
    fpx_functions.fpx_sslsocket_destroy             = FPX_SSLSocket_Destroy;                // 3
    fpx_functions.fpx_sslsocket_connect             = FPX_SSLSocket_Connect;                // 4
    fpx_functions.fpx_sslsocket_receive             = FPX_SSLSocket_Receive;                // 5
    fpx_functions.fpx_sslsocket_send                        = FPX_SSLSocket_Send;                   // 6
#endif // defined(OPENSSL) || defined(GNUTLS)

#if defined(ALSA) || defined(OSS) || defined(PULSEAUDIO) || defined(JACK)
    fpx_functions.fpx_soundoutput_open                      = FPX_SoundOutput_Open;                 // 7
    fpx_functions.fpx_soundoutput_close             = FPX_SoundOutput_Close;                // 8
    fpx_functions.fpx_soundoutput_latency           = FPX_SoundOutput_Latency;              // 9
#endif // defined(ALSA) || defined(OSS)

#ifdef V4L1
    fpx_functions.fpx_videoinput_open                       = FPX_VideoInput_Open;                  // 10
    fpx_functions.fpx_videoinput_close                      = FPX_VideoInput_Close;                 // 11
    fpx_functions.fpx_videoinput_getframe           = FPX_VideoInput_GetFrame;              // 12
#endif // V4L1

    fpx_functions.fpx_count                                         = 14;

#ifdef OPENSSL
    SSL_library_init();
#elif defined(GNUTLS)
    gnutls_global_init();
#endif // GNUTLS

    return (void *)&fpx_functions;
}

static void FPX_Shutdown(void)
{
#ifdef OPENSSL

#elif defined(GNUTLS)
    gnutls_global_deinit();
#endif // GNUTLS
}

//
// SSL support functions
//

#ifdef OPENSSL
struct SSL_Instance {
    SSL     *ssl;
    SSL_CTX *sslCtx;
};

static void *FPX_SSLSocket_Create(int socket_fd)
// return = instance pointer
{
    struct SSL_Instance *instance = (struct SSL_Instance *)FPI_Mem_Alloc(sizeof(struct SSL_Instance));
    memset(instance,0,sizeof(struct SSL_Instance));

    if ( ( instance->sslCtx = SSL_CTX_new( TLSv1_client_method() ) ) == 0 ) goto fail;

    if ( ( instance->ssl = SSL_new(instance->sslCtx) ) == 0 ) goto fail;

    if ( SSL_set_fd(instance->ssl, socket_fd) < 0 ) goto fail;

    return (void *)instance;
fail:
    if ( instance->ssl ) {
        SSL_shutdown(instance->ssl);
    }

    if ( instance->sslCtx ) {
        SSL_CTX_free(instance->sslCtx);
    }

    if (FPI_Mem_Free) FPI_Mem_Free(instance);

    return 0;
}

static int FPX_SSLSocket_Destroy(void *ptr)
// ptr = instance pointer
// return = 0 on sucess, < 0 on error
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;

    if ( instance->ssl ) {
        SSL_shutdown(instance->ssl);
    }

    if ( instance->sslCtx ) {
        SSL_CTX_free(instance->sslCtx);
    }

    if (FPI_Mem_Free) FPI_Mem_Free(instance);

    return 0;
}

static int FPX_SSLSocket_Connect(void *ptr)
// ptr = instance pointer
// socket_fd = BSD socket fd to be associated with SSL connection
// return = 0 on sucess, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return SSL_connect(instance->ssl);
}

static int FPX_SSLSocket_Receive(void *ptr, char *buffer, int n_bytes)
// ptr = instance pointer
// buffer = raw buffer to place received data into
// n_bytes = length of buffer in bytes
// return = actual bytes received, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return SSL_read(instance->ssl, buffer, n_bytes);
}

static int FPX_SSLSocket_Send(void *ptr, const char *buffer, int n_bytes)
// ptr = instance pointer
// buffer = raw buffer to be sent
// n_bytes = length of input buffer in bytes
// return = actual bytes sent, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return SSL_write(instance->ssl, buffer, n_bytes);
}
#elif defined(GNUTLS)
struct SSL_Instance {
    gnutls_session_t                                        session;
    gnutls_anon_client_credentials_t        anoncred;
};

static void *FPX_SSLSocket_Create(int socket_fd)
// return = instance pointer
{
    const int kx_prio[] = { GNUTLS_KX_ANON_DH, 0 };

    struct SSL_Instance *instance = (struct SSL_Instance *)FPI_Mem_Alloc(sizeof(struct SSL_Instance));
    memset(instance,0,sizeof(struct SSL_Instance));

    if ( gnutls_anon_allocate_client_credentials(&instance->anoncred) < 0 ) goto fail;

    if ( gnutls_init(&instance->session, GNUTLS_CLIENT) < 0 ) goto fail;

    if ( gnutls_set_default_priority(instance->session) < 0 ) goto fail;

    if ( gnutls_kx_set_priority(instance->session, kx_prio) < 0 ) goto fail;

    if ( gnutls_credentials_set(instance->session, GNUTLS_CRD_ANON, instance->anoncred) < 0 ) goto fail;

    gnutls_transport_set_ptr(instance->session, (gnutls_transport_ptr_t)socket_fd);

    return (void *)instance;
fail:

    if ( instance->session ) {
        gnutls_deinit(instance->session);
    }

    if ( instance->anoncred ) {
        gnutls_anon_free_client_credentials(instance->anoncred);
    }

    if (FPI_Mem_Free) FPI_Mem_Free(instance);

    return 0;
}

static int FPX_SSLSocket_Destroy(void *ptr)
// ptr = instance pointer
// return = 0 on sucess, < 0 on error
{
    struct SSL_Instance *instance = (struct SSL_Instance *)FPI_Mem_Alloc(sizeof(struct SSL_Instance));

    gnutls_bye(instance->session, GNUTLS_SHUT_RDWR);

    gnutls_deinit(instance->session);

    gnutls_anon_free_client_credentials(instance->anoncred);

    if (FPI_Mem_Free) FPI_Mem_Free(instance);

    return 0;
}

static int FPX_SSLSocket_Connect(void *ptr)
// ptr = instance pointer
// socket_fd = BSD socket fd to be associated with SSL connection
// return = 0 on sucess, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return gnutls_handshake(instance->session);
}

static int FPX_SSLSocket_Receive(void *ptr, char *buffer, int n_bytes)
// ptr = instance pointer
// buffer = raw buffer to place received data into
// n_bytes = length of buffer in bytes
// return = actual bytes received, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return gnutls_record_recv(instance->session, buffer, n_bytes);
}

static int FPX_SSLSocket_Send(void *ptr, const char *buffer, int n_bytes)
// ptr = instance pointer
// buffer = raw buffer to be sent
// n_bytes = length of input buffer in bytes
// return = actual bytes sent, < 0 on error
//
// Flash Player will use errno to obtain current status
{
    struct SSL_Instance *instance = (struct SSL_Instance *)ptr;
    return gnutls_record_send(instance->session, buffer, n_bytes);
}
#endif // GNUTLS

//
// Sound support functions
//
#ifdef ALSA
struct SoundOutput_Instance {
    snd_pcm_t *                             handle;
    snd_async_handler_t *   async_handler;
    sem_t                                   semaphore;
    pthread_t                               thread;
    char *                                  buffer;
    snd_pcm_sframes_t               buffer_size;
    snd_pcm_sframes_t               period_size;
    snd_pcm_sframes_t               buffer_pos;
    char *                                  buffer_ptr;
};

static void *alsa_thread(void *ptr)
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    snd_pcm_sframes_t avail = 0;
    int result = 0;
    int err = 0;
    int state = 0;

    for( ; ; ) {

        err = sem_wait(&instance->semaphore);
        if ( !instance->handle ) {
            pthread_exit(0);
            return 0;
        }

        if ( err < 0 ) {
            usleep(1);
            continue;
        }

        do {
            if ( instance->buffer_pos <= 0 ) {
                FPI_SoundOutput_FillBuffer(ptr, instance->buffer, snd_pcm_frames_to_bytes(instance->handle, instance->period_size));
                instance->buffer_pos = instance->period_size;
                instance->buffer_ptr = instance->buffer;
            }
            do {
                state = snd_pcm_state(instance->handle);
                if(state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_PREPARED) {
                    snd_pcm_prepare(instance->handle);
                }
                result = snd_pcm_writei(instance->handle, instance->buffer_ptr, instance->buffer_pos);
                if( result <= 0 ) {
                    switch( result ) {
                        case    -EPIPE: {
                            snd_pcm_prepare(instance->handle);
                        } break;
                        case    -ESTRPIPE: {
                            err = snd_pcm_resume(instance->handle);
                            if ( err < 0 ) {
                                snd_pcm_prepare(instance->handle);
                            }
                        } break;
                    }
                    break;
                } else {
                    instance->buffer_pos -= result;
                    instance->buffer_ptr += snd_pcm_frames_to_bytes(instance->handle, result);
                }
            } while (instance->buffer_pos);
            avail = snd_pcm_avail_update(instance->handle);
            if( avail < 0 ) {
                switch( avail ) {
                    case    -EPIPE: {
                        snd_pcm_prepare(instance->handle);
                    } break;
                    case    -ESTRPIPE: {
                        err = snd_pcm_resume(instance->handle);
                        if ( err < 0 ) {
                            snd_pcm_prepare(instance->handle);
                        }
                    } break;
                }
                break;
            }

        } while(avail >= instance->period_size);
    }
}

static void alsa_callback(snd_async_handler_t *ahandler)
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)snd_async_handler_get_callback_private(ahandler);
    // signal mixer thread
    sem_post(&instance->semaphore);
}

static void *FPX_SoundOutput_Open()
// return = instance pointer
{
    struct SoundOutput_Instance *instance = 0;
    snd_pcm_hw_params_t *hwparams = 0;
    snd_pcm_sw_params_t *swparams = 0;
    snd_pcm_format_t pcm_format;
    unsigned int buffer_time = 500000;
    unsigned int period_time =  20000;
    unsigned int actual_rate;
    snd_pcm_uframes_t size;
    int direction;
    void *retVal = 0;
    int err = 0;

    if ( !FPI_SoundOutput_FillBuffer )  goto fail;
    if ( !FPI_Mem_Alloc ) goto fail;

    instance = FPI_Mem_Alloc(sizeof(struct SoundOutput_Instance));
    memset(instance,0,sizeof(struct SoundOutput_Instance));

    if ( ( err = snd_pcm_open(&instance->handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) ) < 0) {
        if ( ( err = snd_pcm_open(&instance->handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) ) < 0) {
            goto fail;
        }
    }

    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    if (snd_pcm_hw_params_any(instance->handle, hwparams) < 0) goto fail;

    if (snd_pcm_hw_params_set_access(instance->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) goto fail;

    pcm_format = SND_PCM_FORMAT_S16_LE;

    if (snd_pcm_hw_params_set_format(instance->handle, hwparams, pcm_format) < 0) goto fail;

    if (snd_pcm_hw_params_set_channels(instance->handle, hwparams, 2) < 0) goto fail;

    actual_rate = 44100;

    if (snd_pcm_hw_params_set_rate_near(instance->handle, hwparams, &actual_rate, 0) < 0) goto fail;

    if (actual_rate != 44100) goto fail;

    if (snd_pcm_hw_params_set_buffer_time_near(instance->handle, hwparams, &buffer_time, &direction) < 0) goto fail;

    if (snd_pcm_hw_params_get_buffer_size(hwparams, &size) < 0) goto fail;

    instance->buffer_size = (snd_pcm_sframes_t)size;

    if (snd_pcm_hw_params_set_period_time_near(instance->handle, hwparams, &period_time, &direction) < 0) goto fail;

    if (snd_pcm_hw_params_get_period_size(hwparams, &size, &direction) < 0) goto fail;

    instance->period_size = (snd_pcm_sframes_t)size;

    if (snd_pcm_hw_params(instance->handle, hwparams) < 0) goto fail;

    if (snd_pcm_sw_params_current(instance->handle, swparams) < 0) goto fail;

    if (snd_pcm_sw_params_set_start_threshold(instance->handle, swparams, ((instance->buffer_size-1) / instance->period_size) * instance->period_size) < 0) goto fail;

    if (snd_pcm_sw_params_set_stop_threshold(instance->handle, swparams, ~0U) < 0) goto fail;

    if (snd_pcm_sw_params_set_avail_min(instance->handle, swparams, instance->period_size) < 0) goto fail;

    if (snd_pcm_sw_params_set_xfer_align(instance->handle, swparams, 1) < 0) goto fail;

    if (snd_pcm_sw_params(instance->handle, swparams) < 0) goto fail;

    if (snd_async_add_pcm_handler(&instance->async_handler, instance->handle, &alsa_callback, instance) < 0) goto fail;

    if ( ( instance->buffer = FPI_Mem_Alloc(instance->buffer_size * 2 * 2 * 2) ) == 0 ) goto fail;

    if ( pthread_create(&instance->thread, 0, alsa_thread, instance) < 0 ) goto fail;

    sem_post(&instance->semaphore);

    return instance;

fail:
    if ( instance ) {
        if ( instance->handle ) {
            snd_pcm_drop(instance->handle);
            snd_pcm_close(instance->handle);
            instance->handle = 0;
        }
        if ( instance->thread ) {
            sem_post(&instance->semaphore);
            sem_destroy(&instance->semaphore);
            pthread_join(instance->thread,&retVal);
        }
        if ( instance->buffer ) {
            if ( FPI_Mem_Free ) FPI_Mem_Free(instance->buffer);
        }
        if ( FPI_Mem_Free ) FPI_Mem_Free(instance);
    }
    return 0;
}

static int FPX_SoundOutput_Close(void *ptr)
// ptr = instance pointer
// return = 0 on success, < 0 on error
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    void *retVal = 0;
    if ( instance->handle ) {
        snd_pcm_drop(instance->handle);
        snd_pcm_close(instance->handle);
        instance->handle = 0;
    }
    if ( instance->thread ) {
        sem_post(&instance->semaphore);
        sem_destroy(&instance->semaphore);
        pthread_join(instance->thread,&retVal);
    }
    if ( instance->buffer ) {
        if ( FPI_Mem_Free ) FPI_Mem_Free(instance->buffer);
    }
    if ( FPI_Mem_Free ) FPI_Mem_Free(instance);
    return 0;
}

static int FPX_SoundOutput_Latency(void *ptr)
// ptr = instance pointer
// return = 0 on success, < 0 on error
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    if ( instance->handle ) {
        snd_pcm_sframes_t delay = 0;
        snd_pcm_delay(instance->handle, &delay);
        if ( snd_pcm_state(instance->handle) == SND_PCM_STATE_RUNNING && delay > 0 ) {
            return delay;
        } else {
            return 0;
        }
    }
    return -1;
}
#elif defined(OSS)
struct SoundOutput_Instance {
    int                                             oss_fd;
    pthread_t                               thread;
    int                                             signal;
};

static void *oss_thread(void *ptr)
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    char buffer[4096];
    int len = 0;
    int written = 0;
    for(;;) {
        FPI_SoundOutput_FillBuffer(ptr,buffer,4096);
        len = 4096;
        while ( len ) {
            written = write(instance->oss_fd, buffer, len);
            if ( written >= 0 ) {
                len -= written;
            }
            if ( instance->signal ) {
                pthread_exit(0);
            }
            if ( written < 0 ) {
                usleep(100);
            }
        }
    }
}

static void *FPX_SoundOutput_Open()
// return = instance pointer
{
    struct SoundOutput_Instance *instance = 0;
    int format = AFMT_S16_LE;
    int stereo = 1;
    int speed = 44100;

    if ( !FPI_SoundOutput_FillBuffer )  goto fail;
    if ( !FPI_Mem_Alloc ) goto fail;

    instance = (struct SoundOutput_Instance *)FPI_Mem_Alloc(sizeof(struct SoundOutput_Instance));
    memset(instance,0,sizeof(struct SoundOutput_Instance));

    if ( ( instance->oss_fd = open("/dev/dsp",O_WRONLY) ) < 0 ) goto fail;

    if ( ioctl(instance->oss_fd, SNDCTL_DSP_SETFMT, &format) < 0 ) goto fail;

    if ( ioctl(instance->oss_fd, SNDCTL_DSP_STEREO, &stereo) < 0 ) goto fail;

    if ( ioctl(instance->oss_fd, SNDCTL_DSP_SPEED, &speed) < 0 ) goto fail;

    if ( pthread_create(&instance->thread, 0, oss_thread, instance) < 0 ) goto fail;

    return instance;
fail:
    if ( instance ) {
        if ( FPI_Mem_Free ) FPI_Mem_Free(instance);
    }
    return 0;
}

static int FPX_SoundOutput_Close(void *ptr)
// ptr = instance pointer
// return = 0 on success, < 0 on error
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    void *retVal = 0;

    instance->signal = 1;

    if ( instance->oss_fd ) {
        ioctl(instance->oss_fd, SNDCTL_DSP_RESET, 0);
    }

    if ( instance->thread ) {
        pthread_join(instance->thread,&retVal);
    }

    if ( instance->oss_fd ) {
        close(instance->oss_fd);
    }

    if ( FPI_Mem_Free ) FPI_Mem_Free(instance);

    return 0;
}

static int FPX_SoundOutput_Latency(void *ptr)
// ptr = instance pointer
// return = 0 on success, < 0 on error
{
    struct SoundOutput_Instance *instance = (struct SoundOutput_Instance *)ptr;
    if ( instance->oss_fd ) {
        int value = 0;
        if ( ( value = ioctl(instance->oss_fd,SNDCTL_DSP_GETODELAY,&value) ) == 0 ) {
            return value / 4;
        }
        return 0;
    }
    return -1;
}
#endif // defined(OSS)

#ifdef PULSEAUDIO

#define BUFSIZE (4096)

struct output_data {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
    pa_stream *stream;
    uint8_t buf[BUFSIZE];
    pthread_t thread_id;
    int first;
};

static void context_state_cb(pa_context *c, void *userdata) {
    struct output_data *p = userdata;

    assert(c);
    assert(p);

    p->thread_id = pthread_self();
    p->context = c;

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(p->mainloop, 0);
            break;

        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
    }
}

static void stream_state_cb(pa_stream *s, void *userdata) {
    struct output_data *p = userdata;

    assert(s);
    assert(p);

    p->thread_id = pthread_self();
    p->stream = s;

    switch (pa_stream_get_state(s)) {

        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(p->mainloop, 0);
            break;

        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
            break;
    }
}

static void write_data(struct output_data *p) {
    size_t length;

    assert(p);

    /* Wait until timing info is available before we write the second
     * and all subsequent blocks */
    if (!p->first && !pa_stream_get_timing_info(p->stream))
        return;

    length = pa_stream_writable_size(p->stream);

    while (length > 4) {
        size_t l = length;

        if (l > BUFSIZE)
            l = BUFSIZE;

        l &= ~ ((size_t) 3);

        FPI_SoundOutput_FillBuffer(p, (char*) p->buf, l);

        if (pa_stream_write(p->stream, p->buf, l, NULL, 0, PA_SEEK_RELATIVE) < 0)
            break;

        length -= l;

        if (p->first)
            break;
    }

    /* There's no real handling of errors here. Unfortunately the
     * Flash API doesn't export a sane way to do this. With networked
     * audio streams and hotplug-capable audio devices the audio
     * stream might be killed in the middle of nothing, hence it is
     * very unfortunate that we cannot report errors that happen here
     * back to Flash. */

    p->first = 0; /* So, we write the first block noch, remember that */
}

static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
    struct output_data *p = userdata;

    assert(s);
    assert(length > 0);
    assert(p);

    p->thread_id = pthread_self();

    /* Write some data */
    write_data(p);
}

static void stream_latency_update_cb(pa_stream *s, void *userdata) {
    struct output_data *p = userdata;

    assert(s);
    assert(p);

    p->thread_id = pthread_self();

    /* Try to write some more, in case we delayed the first write until latency data became available */
    write_data(p);
}

static void *FPX_SoundOutput_Open(void) {

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE, /* Hmm, Flash wants LE here, not
                                    * NE. This makes porting Flash to
                                    * Big-Endian machines unnecessary
                                    * difficult. */
        .rate = 44100,
        .channels = 2
    };

    struct output_data *p;

    /* Unfortunately we cannot report any useful error string back to
     * Flash. It would be highly preferable if Flash supported some
     * way how we could tell the user what the reason is why audio is
     * not working for him. */

    if (!(p = FPI_Mem_Alloc(sizeof(struct output_data))))
        goto fail;

    memset(p, 0, sizeof(*p));
    p->first = 1;
    p->thread_id = (pthread_t) 0;

    /* First, let's create the main loop */
    if (!(p->mainloop = pa_threaded_mainloop_new()))
        goto fail;

    /* Second, initialize the connection context */
    if (!(p->context = pa_context_new(pa_threaded_mainloop_get_api(p->mainloop), "Adobe Flash")))
        goto fail;

    pa_context_set_state_callback(p->context, context_state_cb, p);

    if (pa_context_connect(p->context, NULL, 0, NULL) < 0)
        goto fail;

    /* Now, let's start the background thread */
    pa_threaded_mainloop_lock(p->mainloop);

    if (pa_threaded_mainloop_start(p->mainloop) < 0)
        goto fail;

    /* Wait until the context is ready */
    pa_threaded_mainloop_wait(p->mainloop);

    if (pa_context_get_state(p->context) != PA_CONTEXT_READY)
        goto unlock_and_fail;

    /* Now, initialize the stream */
    if (!(p->stream = pa_stream_new(p->context, "Flash Animation", &ss, NULL)))
        goto unlock_and_fail;

    pa_stream_set_state_callback(p->stream, stream_state_cb, p);
    pa_stream_set_write_callback(p->stream, stream_request_cb, p);
    pa_stream_set_latency_update_callback(p->stream, stream_latency_update_cb, p);

    if (pa_stream_connect_playback(p->stream, NULL, NULL, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL) < 0)
        goto unlock_and_fail;

    /* Wait until the stream is ready */
    pa_threaded_mainloop_wait(p->mainloop);

    if (pa_stream_get_state(p->stream) != PA_STREAM_READY)
        goto unlock_and_fail;

    pa_threaded_mainloop_unlock(p->mainloop);

    return p;

unlock_and_fail:

    pa_threaded_mainloop_unlock(p->mainloop);

fail:
    if (p)
        FPX_SoundOutput_Close(p);

    return NULL;
}

static int FPX_SoundOutput_Close(void *ptr) {
    struct output_data *p = ptr;

    assert(p);

    if (p->mainloop)
        pa_threaded_mainloop_stop(p->mainloop);

    if (p->stream) {
        pa_stream_disconnect(p->stream);
        pa_stream_unref(p->stream);
    }

    if (p->context) {
        pa_context_disconnect(p->context);
        pa_context_unref(p->context);
    }

    if (p->mainloop)
        pa_threaded_mainloop_free(p->mainloop);

    if (FPI_Mem_Free)
        FPI_Mem_Free(p);

    return 0;
}

static int FPX_SoundOutput_Latency(void *ptr) {
    struct output_data *p = ptr;
    pa_usec_t t = 0;
    int negative;
    int r;

    assert(p);

    /* We lock here only if we are not called from our event loop thread */
    if (!p->thread_id || !pthread_equal(p->thread_id, pthread_self()))
        pa_threaded_mainloop_lock(p->mainloop);

    if (pa_stream_get_latency(p->stream, &t, &negative) < 0 || negative)
        r = 0;
    else
        r = (int) (pa_usec_to_bytes(t, pa_stream_get_sample_spec(p->stream)) >> 2);

    if (!p->thread_id || !pthread_equal(p->thread_id, pthread_self()))
        pa_threaded_mainloop_unlock(p->mainloop);

    return r;
}

#endif

#ifdef JACK

struct jack_output_data {
	jack_client_t *client;
	jack_port_t *port_l;
	jack_port_t *port_r;
	SRC_STATE *src_l;
	SRC_STATE *src_r;
	int src_error;
	
	jack_ringbuffer_t *buffer;
	sem_t semaphore;
	volatile int quit;
	pthread_t tid;
};

static void *jack_flash_thread( void *arg )
{
	struct jack_output_data *p = arg;
	int jack_rate = jack_get_sample_rate( p->client );
	int flash_frames = jack_get_buffer_size( p->client ) * 44100 / jack_rate;
	
	size_t bufsize = 2*flash_frames * sizeof( int16_t );
	int16_t *buffer = alloca( bufsize );

	//sem_wait( &(p->semaphore) );
	//usleep( 50 );

	sem_wait( &(p->semaphore) );
	while( !p->quit ) {
		FPI_SoundOutput_FillBuffer(p, (char*) buffer, bufsize);
		if( jack_ringbuffer_write( p->buffer, (char*)buffer, bufsize ) != bufsize )
			printf( "something is wrong\n" );
		sem_wait( &(p->semaphore) );
	}

	return NULL;
}

static int jack_process_cb( jack_nframes_t nframes, void *arg ) 
{
	struct jack_output_data *p = arg;
	int i;
	int jack_rate = jack_get_sample_rate( p->client );
	int flash_frames = nframes * 44100 / jack_rate;
	
	size_t bufsize = 2*flash_frames * sizeof( int16_t );
	int16_t *buffer = alloca( bufsize );
	float   *float_buf = alloca( flash_frames * sizeof(float) );

	float *port_l = jack_port_get_buffer( p->port_l, nframes );
	float *port_r = jack_port_get_buffer( p->port_r, nframes );

	SRC_DATA sd;

	if( jack_ringbuffer_read_space( p->buffer ) < bufsize ) {
		// no data to read. fill ports with zero and return.
		memset( port_l, 0, nframes * sizeof( jack_default_audio_sample_t ) );
		memset( port_r, 0, nframes * sizeof( jack_default_audio_sample_t ) );
		return 0;
	}

	if( jack_ringbuffer_read( p->buffer, (char *)buffer, bufsize ) != bufsize ) {
		printf( "Something is pretty wrong :( \n" );
		return 0;
	}
	sem_post( &(p->semaphore) );

	for( i=0; i<flash_frames; i++ ) {
		float_buf[i] = (float) (buffer[2*i]) / 32768.0;
	}

	sd.data_in = float_buf;
	sd.data_out = port_l;
	sd.input_frames = flash_frames; 
	sd.output_frames = nframes; 
	sd.end_of_input = 0;
//	sd.src_ratio = (double)flash_frames / (double)nframes;
	sd.src_ratio = (double)nframes / (double)flash_frames;

	src_process( p->src_l, &sd );


	for( i=0; i<flash_frames; i++ ) {
		float_buf[i] = (float) (buffer[2*i+1]) / 32768.0;
	}

	sd.data_in = float_buf;
	sd.data_out = port_r;
	sd.input_frames = flash_frames; 
	sd.output_frames = nframes; 
	sd.end_of_input = 0;
//	sd.src_ratio = (double)flash_frames / (double)nframes;
	sd.src_ratio = (double)nframes / (double)flash_frames;

	src_process( p->src_r, &sd );

	return 0;
}

jack_ringbuffer_t *
FPI_jack_ringbuffer_create (size_t sz)
{
  int power_of_two;
  jack_ringbuffer_t *rb;

  rb = FPI_Mem_Alloc (sizeof (jack_ringbuffer_t));

  for (power_of_two = 1; 1 << power_of_two < sz; power_of_two++);

  rb->size = 1 << power_of_two;
  rb->size_mask = rb->size;
  rb->size_mask -= 1;
  rb->write_ptr = 0;
  rb->read_ptr = 0;
  rb->buf = FPI_Mem_Alloc (rb->size);
  rb->mlocked = 0;

  return rb;
}

static void *FPX_SoundOutput_Open(void) {

    struct jack_output_data *p=NULL;
    char namebuf[100];
    int jack_rate;
    int flash_frames;
    size_t bufsize;
    char *connect_port_1 = getenv( "JACK_FLASH_PORT_1" );
    char *connect_port_2 = getenv( "JACK_FLASH_PORT_2" );
    int i;

    /* Unfortunately we cannot report any useful error string back to
     * Flash. It would be highly preferable if Flash supported some
     * way how we could tell the user what the reason is why audio is
     * not working for him. */

    if (!(p = FPI_Mem_Alloc(sizeof(struct jack_output_data))))
        goto fail;

    memset(p, 0, sizeof(*p));

    p->src_l = src_new(SRC_SINC_FASTEST, 1, & (p->src_error));
    p->src_r = src_new(SRC_SINC_FASTEST, 1, & (p->src_error));


    /* First, let's create the main loop */
    if (!(p->client = jack_client_open( "flash", JackNoStartServer, NULL )))
        goto fail;

    jack_rate = jack_get_sample_rate( p->client );
    flash_frames = jack_get_buffer_size( p->client ) * 44100 / jack_rate;
    bufsize = 2*flash_frames * sizeof( int16_t );

    p->buffer = FPI_jack_ringbuffer_create( bufsize * 6 );
    sem_init( &(p->semaphore), 0, 0 );

    pthread_create( &(p->tid), NULL, jack_flash_thread, p );
#if 0 
// This seems to trigger a race condition in flash.

    jack_client_create_thread( p->client,
		    &(p->tid),
		    (jack_client_real_time_priority( p->client ) == -1) ? 1 : (jack_client_real_time_priority( p->client ) - 1),
		    (jack_client_real_time_priority( p->client ) == -1) ? 0 : 1,
		    jack_flash_thread,
		    p );
#endif


    /* Second, initialize the connection context */
    if (!(p->port_l = jack_port_register( p->client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) )
        goto fail;
    if (!(p->port_r = jack_port_register( p->client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) )
        goto fail;

    jack_set_process_callback( p->client, jack_process_cb, p );
    jack_activate( p->client );

    snprintf( namebuf, 99, "%s:out1", jack_get_client_name( p->client ) );
    jack_connect( p->client, namebuf, (connect_port_1 != NULL) ? connect_port_1 : "system:playback_1" );
    snprintf( namebuf, 99, "%s:out2", jack_get_client_name( p->client ) );
    jack_connect( p->client, namebuf, (connect_port_2 != NULL) ? connect_port_2 : "system:playback_2" );

    // Now start the fill thread.
    // the semaphore represents the number of writeable buffers in
    // the ringbuffer. we add 1 for the initial sem_wait.

    for(i=0; i<( 3 + 1 ); i++ )
	    sem_post( &(p->semaphore) );

    return p;

fail:
    if (p)
        FPX_SoundOutput_Close(p);

    return NULL;
}

static int FPX_SoundOutput_Close(void *ptr) {
    struct jack_output_data *p = ptr;

    //assert(p);
    if( p->tid ) {
	    p->quit = 1;
	    usleep(300);
	    sem_post( &(p->semaphore) );
	    pthread_join( p->tid, NULL );
    }

    if (p->client) {
	    jack_deactivate( p->client );
	    jack_client_close( p->client );
    }

    return 0;
}

static int FPX_SoundOutput_Latency(void *ptr) {
	// heh ? jack has no latency :P
    struct jack_output_data *p = ptr;
    //return jack_ringbuffer_read_space( p->buffer ) / 4;
    return 1024;
}
#endif


#ifdef V4L1
struct VideoOutput_Instance {
    int                                             v4l_fd;
    pthread_t                               thread;
    int                                             signal;
    char *                                  buffer[2];
    int                                             buffercurrent;
    int                                             buffersize;
    struct video_window     window;
    struct video_picture    picture;
};

static void *v4l_thread(void *ptr)
{
    struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *)ptr;
    int result;
    int status;

    for(;;) {

        result = read(instance->v4l_fd, instance->buffer[instance->buffercurrent], instance->buffersize);
        if(result > 0) {
        }

        if ( result < 0 ) {
            usleep(10000);
        }

        if ( instance->signal ) {
            status = 0;
            ioctl(instance->v4l_fd, VIDIOCCAPTURE, &status);
            pthread_exit(0);
        }
    }
    return NULL;
}

static void *FPX_VideoInput_Open(void)
{
    struct VideoOutput_Instance *instance = 0;

    if ( !FPI_Mem_Alloc ) goto fail;

    instance = (struct VideoOutput_Instance *)FPI_Mem_Alloc(sizeof(struct VideoOutput_Instance));
    memset(instance,0,sizeof(struct VideoOutput_Instance));

    if ( ( instance->v4l_fd = open("/dev/video", O_RDONLY) ) < 0 ) goto fail;

    if ( ioctl(instance->v4l_fd, VIDIOCGPICT, &instance->picture) < 0 ) goto fail;

    switch(instance->picture.palette) {
        case    VIDEO_PALETTE_YUV420P:
            break;
        case    VIDEO_PALETTE_RGB24:
        case    VIDEO_PALETTE_YUV422P:
        case    VIDEO_PALETTE_YUV411P:
        case    VIDEO_PALETTE_YUV410P:
        case    VIDEO_PALETTE_GREY:
        case    VIDEO_PALETTE_HI240:
        case    VIDEO_PALETTE_RGB565:
        case    VIDEO_PALETTE_RGB32:
        case    VIDEO_PALETTE_RGB555:
        case    VIDEO_PALETTE_YUV422:
        case    VIDEO_PALETTE_YUYV:
        case    VIDEO_PALETTE_UYVY:
        case    VIDEO_PALETTE_YUV420:
        case    VIDEO_PALETTE_YUV411:
        case    VIDEO_PALETTE_RAW:
        default:
            goto fail;
    }

    if( ioctl(instance->v4l_fd, VIDIOCGWIN, &instance->window) < 0 ) goto fail;

    instance->buffer[0] = FPI_Mem_Alloc(instance->window.width * instance->window.height * 2);
    instance->buffer[1] = FPI_Mem_Alloc(instance->window.width * instance->window.height * 2);

    if ( pthread_create(&instance->thread, 0, v4l_thread, instance) < 0 ) goto fail;

    return instance;

fail:
    if ( FPI_Mem_Free ) {
        if ( instance->buffer[0] ) {
            FPI_Mem_Free(instance->buffer[0]);
        }
        if ( instance->buffer[1] ) {
            FPI_Mem_Free(instance->buffer[1]);
        }
        FPI_Mem_Free(instance);
    }
    return 0;
}

static int FPX_VideoInput_Close(void *ptr)
{
    struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *)ptr;
    void *retVal = 0;

    instance->signal = 1;

    if ( instance->thread ) {
        pthread_join(instance->thread,&retVal);
    }

    if ( instance->v4l_fd ) {
        close(instance->v4l_fd);
    }

    if ( FPI_Mem_Free ) {
        if ( instance->buffer[0] ) {
            FPI_Mem_Free(instance->buffer[0]);
        }
        if ( instance->buffer[1] ) {
            FPI_Mem_Free(instance->buffer[1]);
        }
        FPI_Mem_Free(instance);
    }

    return 0;
}

static int FPX_VideoInput_GetFrame(void *ptr, char *data, int width, int height, int pitch_n_bytes)
{
    struct VideoOutput_Instance *instance = (struct VideoOutput_Instance *)ptr;
    int ix, iy, ox, oy, ow, oh, dx, dy, Y, U, V, R, G, B;
    unsigned char *y, *u, *v;

    switch(instance->picture.palette) {
        case    VIDEO_PALETTE_YUV420P: {
            ow = instance->window.width;
            oh = instance->window.height;

            dx = (ow<<16) / width;
            dy = (oh<<16) / height;

            y  = (unsigned char *)instance->buffer[instance->buffercurrent^1];
            u  = y + ow * oh;
            v  = u + ow * oh / 4;

            oy = 0;

            for ( iy = 0; iy < height; iy++ ) {

                ox = 0;

                for ( ix = 0; ix < width; ix++ ) {

                    Y = ( 149 * ((int)(y[(oy>>16)*(ow  )+(ox>>16)]) - 16) ) / 2;
                    U =          (int)(u[(oy>>17)*(ow/2)+(ox>>17)]) - 128;
                    V =          (int)(v[(oy>>17)*(ow/2)+(ox>>17)]) - 128;

                    R = (Y + V * 102          ) / 64;
                    G = (Y - V *  52 - U * 25 ) / 64;
                    B = (Y + U * 129          ) / 64;

                    R = R < 0 ? 0 : ( R > 255 ? 255 : R );
                    G = G < 0 ? 0 : ( G > 255 ? 255 : G );
                    B = B < 0 ? 0 : ( B > 255 ? 255 : B );

                    data[ix*3+0] = R;
                    data[ix*3+1] = G;
                    data[ix*3+2] = B;

                    ox += dx;
                }

                oy += dy;

                data += pitch_n_bytes;
            }
        } break;
        default:
            goto fail;
    }

    instance->buffercurrent ^= 1;

    return 0;

fail:
    return -1;
}
#endif // V4L1
