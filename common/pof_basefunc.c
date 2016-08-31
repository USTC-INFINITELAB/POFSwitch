/**
 * Copyright (c) 2012, 2013, Huawei Technologies Co., Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../include/pof_common.h"
#include "../include/pof_type.h"
#include "../include/pof_log_print.h"
#include "../include/pof_global.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_memory.h"
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

/* Define pofbf_key to build queue using ftok function. */
static key_t pofbf_key = 0;

/***********************************************************************
 * Create task.
 * Form:     uint32_t pofbf_task_create(void *arg, \
 *                                      POF_TASK_FUNC task_func, \
 *                                      task_t *task_id_ptr)
 * Input:    arguments of task function, task function
 * Output:   task id
 * Return:   POF_OK or Error code
 * Discribe: This function creates a new task that runs the task function
 *           with the arguments.
 ***********************************************************************/
uint32_t pofbf_task_create(void *arg, POF_TASK_FUNC task_func, task_t *task_id_ptr){
    if(NULL == task_func || NULL == task_id_ptr){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
    }

    if(POF_OK != pthread_create((task_t *)task_id_ptr, NULL, (void *)task_func, arg)){
        *task_id_ptr = POF_INVALID_TASKID;
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
    }

    return POF_OK;
}

/***********************************************************************
 * Task delay.
 * Form:     uint32_t pofbf_task_delay(uint32_t delay)
 * Input:    delay time
 * Output:   NONE
 * Return:   VOID
 * Discribe: This function delays the task. The unit of the delay is
 *           milli-second.
 ***********************************************************************/
uint32_t pofbf_task_delay(uint32_t delay){
    /* The unit of the argument of usleep function is micro-seconds. */
    usleep(delay*1000);
    return;
}

/***********************************************************************
 * Delete task.
 * Form:     uint32_t pofbf_task_delete(task_t *task_id_ptr)
 * Input:    task id
 * Output:   task id
 * Return:   POF_OK or Error code
 * Discribe: This function deletes the task corresponding to the task id.
 ***********************************************************************/
uint32_t pofbf_task_delete(task_t *task_id_ptr){
    task_t task_id = POF_INVALID_TASKID;

    if(NULL == task_id_ptr){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_DELETE_FAIL);
    }
    task_id = *task_id_ptr;

    if(task_id == POF_INVALID_TASKID){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_DELETE_FAIL);
    }
    if(task_id == pthread_self()){
        return POF_OK;
    }

    if(POF_OK != pthread_cancel(task_id)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_DELETE_FAIL);
    }

    *task_id_ptr = POF_INVALID_TASKID;

    return POF_OK;
}

/***********************************************************************
 * Create a message queue.
 * Form:     uint32_t pofbf_queue_create(uint32_t *queue_id_ptr)
 * Input:    NONE
 * Output:   queue id
 * Return:   POF_OK or Error code
 * Discribe: This function creates a new message queue.
 ***********************************************************************/
uint32_t pofbf_queue_create(uint32_t *queue_id_ptr){
    uint32_t queue_id = POF_INVALID_QUEUEID;
    int i = (int)g_poflr_dev_id;

    if(queue_id_ptr == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_QUEUE_CREATE_FAIL);
    }

    /* Build the pofbf_key to create the queue. */
    if(pofbf_key == 0){
        pofbf_key = ftok(".", i);
    }

    /* Create the queue using pofbf_key. */
    if(-1 == (queue_id = msgget(pofbf_key,IPC_CREAT|0666))){
        *queue_id_ptr = POF_INVALID_QUEUEID;
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_QUEUE_CREATE_FAIL);
    }

    pofbf_key ++;
    *queue_id_ptr = queue_id;
    return POF_OK;
}

/***********************************************************************
 * Delete an exist message queue.
 * Form:     uint32_t pofbf_queue_delete(uint32_t *queue_id_ptr)
 * Input:    queue id
 * Output:   queue id
 * Return:   POF_OK or Error code
 * Discribe: This function deletes an exist message queue.
 ***********************************************************************/
uint32_t pofbf_queue_delete(uint32_t *queue_id_ptr){
    uint32_t queue_id = POF_INVALID_QUEUEID;

    if(NULL == queue_id_ptr){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_QUEUE_DELETE_FAIL);
    }
    queue_id = *queue_id_ptr;

    if(queue_id == POF_INVALID_QUEUEID){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_QUEUE_DELETE_FAIL);
    }

    /* Delete the message queue. */
    if(POF_OK != msgctl(queue_id, IPC_RMID, NULL)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_QUEUE_DELETE_FAIL);
    }

    *queue_id_ptr = POF_INVALID_QUEUEID;

    return POF_OK;
}

/***********************************************************************
 * Read message from queue.
 * Form:     uint32_t pofbf_queue_read(uint32_t queue_id, \
 *                                     char *buf, \
 *                                     uint32_t max_len, \
 *                                     int timeout)
 * Input:    queue id, max len, timeout mode
 * Output:   data buffer
 * Return:   POF_OK or Error code
 * Discribe: This function reads message from the queue.
 ***********************************************************************/
uint32_t pofbf_queue_read(uint32_t queue_id, void *buf, uint32_t max_len, int timeout){
    uint32_t len;
    /* Define the message struct. */
    struct msg{
        long int mtype;
        char mdata[0];
    } *msg_ptr;

    if(queue_id == POF_INVALID_QUEUEID || buf == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_READ_MSG_QUEUE_FAILURE);
    }

    if(timeout != POF_WAIT_FOREVER && timeout != POF_NO_WAIT){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE);
    }

    msg_ptr = MALLOC(max_len + sizeof(struct msg));
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(msg_ptr);

    /* Receive the messsage from the message queue. */
    if(-1 == (len = msgrcv(queue_id, msg_ptr, max_len + sizeof(struct msg), POF_MSGTYPE_ANY, timeout))){
        FREE(msg_ptr);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_READ_MSG_QUEUE_FAILURE);
    }

    memcpy(buf, msg_ptr->mdata, len - sizeof(struct msg));

    FREE(msg_ptr);
    return POF_OK;
}

/***********************************************************************
 * Write message to queue.
 * Form:     uint32_t pofbf_queue_write(uint32_t queue_id, \
 *                                      char *message, \
 *                                      uint32_t msg_len, \
 *                                      int timeout)
 * Input:    queue id, message data, message length, timeout
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function writes the message data to the message queue
 *           corresponding the queue id.
 ***********************************************************************/
uint32_t pofbf_queue_write(uint32_t queue_id, const void *message, uint32_t msg_len, int timeout){
    /* Define the message struct. */
    struct msg{
        long int mtype;
        char mdata[0];
    }* msg_ptr;

    if(queue_id == POF_INVALID_QUEUEID || message == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE);
    }

    if(msg_len > POF_MESSAGE_SIZE){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_MESSAGE_SIZE_TOO_BIG);
    }

    if(timeout != POF_WAIT_FOREVER && timeout != POF_NO_WAIT){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE);
    }

    msg_ptr = MALLOC(msg_len+sizeof(struct msg));
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(msg_ptr);

    msg_ptr->mtype = POF_MSGTYPE;
    memcpy(msg_ptr->mdata, message, msg_len);

//    msgsnd(queue_id, msg_ptr, msg_len+sizeof(struct msg), IPC_NOWAIT);
    if(-1 == msgsnd(queue_id, msg_ptr, msg_len+sizeof(struct msg), timeout)){
        FREE(msg_ptr);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE);
    }

    FREE(msg_ptr);
	
    return POF_OK;
}

/***********************************************************************
 * Create timer.
 * Form:     uint32_t pofbf_timer_create(uint32_t delay, \
 *                                       uint32_t interval, \
 *                                       POF_TIMER_FUNC timer_handler, \
 *                                       uint32_t *timer_id_ptr)
 * Input:    delay: the timer works after delay, unit is milli-second
 *           interval: timer's interval, unit is milli-second
 *           handler: the routine to execute.
 * Output:   timer id
 * Return:   POF_OK or Error code
 * Discribe: This function creates a timer.
 ***********************************************************************/
uint32_t pofbf_timer_create(uint32_t delay, \
                            uint32_t interval, \
                            POF_TIMER_FUNC timer_handler, \
                            uint32_t *timer_id_ptr)
{
    struct itimerval val;

    if(timer_handler == NULL || timer_id_ptr == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_CREATE_FAIL);
    }

    /* Set the value and the interval of the timer. */
    val.it_interval.tv_sec = (uint32_t)(interval / 1000);
    val.it_interval.tv_usec = (interval % 1000) * 1000;
    val.it_value.tv_sec = (uint32_t)(delay / 1000);
    val.it_value.tv_usec = (delay % 1000) * 1000;

    /* Set the timer function. */
    if(SIG_ERR == signal(SIGALRM, (__sighandler_t)timer_handler)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_CREATE_FAIL);
    }

    /* Set the timer with val. */
    if(POF_OK != setitimer(ITIMER_REAL, &val, NULL)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_CREATE_FAIL);
    }

    *timer_id_ptr = POF_VALID_TIMERID;
    return POF_OK;
}

/***********************************************************************
 * Delete timer.
 * Form:     uint32_t pofbf_timer_delete(uint32_t *timer_id_ptr)
 * Input:    timer id
 * Output:   timer id
 * Return:   POF_OK or Error code
 * Discribe: This function deletes a timer.
 ***********************************************************************/
uint32_t pofbf_timer_delete(uint32_t *timer_id_ptr){
    uint32_t timer_id = POF_INVALID_TIMERID;

    if(NULL == timer_id_ptr){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_DELETE_FAIL);
    }
    timer_id = *timer_id_ptr;

    if(timer_id == POF_INVALID_TIMERID){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_DELETE_FAIL);
    }

    /* Delete the timer. */
    if(POF_OK != setitimer(ITIMER_REAL, NULL, NULL)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_DELETE_FAIL);
    }

    /* Reset the signal function. */
    if(SIG_ERR == signal(SIGALRM, NULL)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TIMER_DELETE_FAIL);
    }

    *timer_id_ptr = POF_INVALID_TIMERID;

    return POF_OK;
}

/***********************************************************************
 * Cover the piece of data using the specified value.
 * Form:     uint32_t pofbf_cover_bit(uint8_t *data_ori, \
 *                                    uint8_t *value, \
 *                                    uint16_t pos_b, \
 *                                    uint16_t len_b)
 * Input:    original data, value, position offset(bit unit), length(bit unit)
 * Output:   data
 * Return:   POF_OK or Error code
 * Discribe: This function covers the piece of data using the specified
 *           value. The value is not a number, but a data buffer. Caller
 *           should make sure that data_ori and value are not NULL.
 ***********************************************************************/
void pofbf_cover_bit(uint8_t *data_ori, const uint8_t *value, uint16_t pos_b, uint16_t len_b){
    uint32_t process_len_b = 0;
    uint16_t pos_b_x, len_B, after_len_b_x;
    uint8_t *ptr;

    pos_b_x = pos_b % 8;
    len_B = (uint16_t)((len_b - 1) / 8 + 1);
    after_len_b_x = (len_b + pos_b - 1) % 8 + 1;
    ptr = data_ori + (uint16_t)(pos_b / 8);

    if(len_b <= (8 - pos_b_x)){
        *ptr = (*ptr & POF_MOVE_BIT_LEFT(0xff, (8-pos_b_x)) \
            | (POF_MOVE_BIT_RIGHT(*value, pos_b_x) & POF_MOVE_BIT_LEFT(0xff, 8-after_len_b_x)) \
            | (*ptr & POF_MOVE_BIT_RIGHT(0xff, after_len_b_x)));
        return;
    }

    *ptr = *ptr & POF_MOVE_BIT_LEFT(0xff, (8-pos_b_x)) \
        | POF_MOVE_BIT_RIGHT(*value, pos_b_x);

    process_len_b = 8 - pos_b_x;
    while(process_len_b < (len_b-8)){
        *(++ptr) = POF_MOVE_BIT_LEFT(*value, 8-pos_b_x) | POF_MOVE_BIT_RIGHT(*(++value), pos_b_x);
        process_len_b += 8;
    }

    *(ptr+1) = (POF_MOVE_BIT_LEFT(*value, 8-pos_b_x) | POF_MOVE_BIT_RIGHT(*(++value), pos_b_x) \
        & POF_MOVE_BIT_LEFT(0xff, 8 - (len_b - process_len_b))) \
        | (*(ptr+1) & POF_MOVE_BIT_RIGHT(0xff, len_b - process_len_b));

    return;
}

/***********************************************************************
 * Copy the piece of original data to the result data buffer.
 * Form:     uint32_t pofbf_copy_bit(uint8_t *data_ori, \
 *                                   uint8_t *data_res, \
 *                                   uint16_t offset_b, \
 *                                   uint16_t len_b)
 * Input:    original data, offset(bit unit), length(bit unit)
 * Output:   result data
 * Return:   POF_OK or Error code
 * Discribe: This function copies the piece of original data to the result
 *           data buffer.
 ***********************************************************************/
void pofbf_copy_bit(const uint8_t *data_ori, uint8_t *data_res, uint16_t offset_b, uint16_t len_b){
    uint32_t process_len_b = 0, offset_b_x;
    uint16_t offset_B;
    uint8_t  *ptr;

	if(NULL==data_ori || NULL==data_res){
		return;
	}

    offset_B = (uint16_t)(offset_b / 8);
    offset_b_x = offset_b % 8;
    ptr = (uint8_t *)(data_ori + offset_B);

    while(process_len_b < len_b){
        *(data_res++) = POF_MOVE_BIT_LEFT(*ptr, offset_b_x) \
            | POF_MOVE_BIT_RIGHT(*(++ptr), 8 - offset_b_x);
        process_len_b += 8;
    }

    data_res--;
    *data_res = *data_res & POF_MOVE_BIT_LEFT(0xff, process_len_b - len_b);

    return;
}

void
pofbf_split_str(char *strSrc, const char *split, char *strDst[], uint32_t count)
{
    uint32_t i = 0;
    strDst[i] = strtok(strSrc, split);
    for(i=1; i<count; i++){
        strDst[i] = strtok(NULL, split);
    }
    return;
}
