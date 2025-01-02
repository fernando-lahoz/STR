#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "serial.h"
#include "util.h"

static struct msg data_out;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int end = 0;

static int end_program()
{
    int ret;
    pthread_mutex_lock(&mtx);
    ret = end;
    pthread_mutex_unlock(&mtx);
    return ret;
}

// --- Little mailbox to send data to writer ---

struct mbox_slot {
    struct msg msg;
    gboolean is_new;
};

static pthread_cond_t mbox_update = PTHREAD_COND_INITIALIZER;

static struct mbox_slot mbox_buff[OUTPUT_SIZE]; // One slot per output

static void mbox_init()
{
    for (size_t i = 0; i < OUTPUT_SIZE; ++i)
        mbox_buff[i].is_new = FALSE;
}

static gboolean mbox_is_there_available_data()
{
    gboolean res = FALSE;
    for (size_t i = 0; i < OUTPUT_SIZE; ++i) {
        if (mbox_buff[i].is_new) {
            res = TRUE;
            break;
        }
    }
    return res;
}

// This one can be used by other modules
void mbox_send(int slot, struct msg* data)
{
    if (slot >= OUTPUT_SIZE)
        return;

    pthread_mutex_lock(&mtx);
    mbox_buff[slot].is_new = TRUE;
    memcpy(&mbox_buff[slot].msg, data, sizeof(mbox_buff[slot].msg));
    pthread_cond_broadcast(&mbox_update);
    pthread_mutex_unlock(&mtx);
}

static gboolean mbox_receive(struct msg* data)
{
    pthread_mutex_lock(&mtx);
    
    while (!mbox_is_there_available_data()) {
        pthread_cond_wait(&mbox_update, &mtx);
        if (end) {
            pthread_mutex_unlock(&mtx);
            return FALSE;
        }
    }
    
    for (size_t i = 0; i < OUTPUT_SIZE; ++i) {
        if (mbox_buff[i].is_new) {
            memcpy(data, &mbox_buff[i].msg, sizeof(mbox_buff[i].msg));
            mbox_buff[i].is_new = FALSE;
            break;
        }
    }

    pthread_mutex_unlock(&mtx);
    return TRUE;
}

static void mbox_awake()
{
    pthread_cond_broadcast(&mbox_update);
}

// ---

static void *writer(void *)
{
    while (!end_program())
    {
        if (!mbox_receive(&data_out))
            goto exit;

        SERIAL_write(&data_out, sizeof(data_out));
        // SERIAL_write is probably a buffered operation
        // In order to not saturate uart's fifo let it sleep some time
        g_usleep(20000); 
    }

exit:
    return NULL;
}


static pthread_t writer_thread;

int launch_writer_thread()
{
    end = 0;

    mbox_init();

    if (pthread_create(&writer_thread, NULL, writer, (void*)&data_out) != 0) {
        fprintf(stderr, "Failed to create writer thread: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void* terminate_writer_thread()
{
    pthread_mutex_lock(&mtx);
    end = 1;
    mbox_awake();
    pthread_mutex_unlock(&mtx);    
    
    void *ret_val;
    pthread_join(writer_thread, &ret_val);

    return ret_val;
}