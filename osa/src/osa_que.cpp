
#include <sys/time.h>
#include <osa_que.h>
#include <errno.h>
/*
void maketimeout(struct timespec *tsp,long msec)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	tsp->tv_sec = now.tv_sec;
	tsp->tv_nsec= now.tv_usec * 1000u;
	tsp->tv_sec += msec/1000;
	tsp->tv_nsec+= (msec%1000) * 1000000u;
}
*/
extern void maketimeout(struct timespec *tsp,long msec);

int OSA_queCreate(OSA_QueHndl *hndl, Uint32 maxLen)
{
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;
  int status=OSA_SOK;

  hndl->curRd = hndl->curWr = 0;
  hndl->count = 0;
  hndl->len   = maxLen;
  hndl->queue = (Int32 *)OSA_memAlloc(sizeof(Int32)*hndl->len);
  
  if(hndl->queue==NULL) {
    OSA_ERROR("OSA_queCreate() = %d \r\n", status);
    return OSA_EFAIL;
  }
 
  status |= pthread_mutexattr_init(&mutex_attr);
  status |= pthread_condattr_init(&cond_attr);  
  
  status |= pthread_mutex_init(&hndl->lock, &mutex_attr);
  status |= pthread_cond_init(&hndl->condRd, &cond_attr);    
  status |= pthread_cond_init(&hndl->condWr, &cond_attr);  

  if(status!=OSA_SOK)
    OSA_ERROR("OSA_queCreate() = %d \r\n", status);
    
  pthread_condattr_destroy(&cond_attr);
  pthread_mutexattr_destroy(&mutex_attr);
    
  return status;
}

int OSA_queDelete(OSA_QueHndl *hndl)
{
  if(hndl->queue!=NULL)
    OSA_memFree(hndl->queue);
    
  pthread_cond_destroy(&hndl->condRd);
  pthread_cond_destroy(&hndl->condWr);
  pthread_mutex_destroy(&hndl->lock);  
  
  return OSA_SOK;
}

int OSA_quePut(OSA_QueHndl *hndl, Int32 value, Uint32 timeout)
{
  int status = OSA_EFAIL;

  pthread_mutex_lock(&hndl->lock);

  while(1) {
    if( hndl->count < hndl->len ) {
      hndl->queue[hndl->curWr] = value;
      hndl->curWr = (hndl->curWr+1)%hndl->len;
      hndl->count++;
      status = OSA_SOK;
      pthread_cond_signal(&hndl->condRd);
      break;
    } else {
      if(timeout == OSA_TIMEOUT_NONE)
        break;

      status = pthread_cond_wait(&hndl->condWr, &hndl->lock);
    }
  }

  pthread_mutex_unlock(&hndl->lock);

  return status;
}


int OSA_queGet(OSA_QueHndl *hndl, Int32 *value, Uint32 timeout)
{
	int ret;
	int status = OSA_EFAIL;

  	if(timeout == OSA_TIMEOUT_FOREVER || timeout == OSA_TIMEOUT_NONE)
	{
		pthread_mutex_lock(&hndl->lock);

		while(1) {
			if(hndl->count > 0 ) {
				if(value!=NULL) {
					*value = hndl->queue[hndl->curRd];
				}

				hndl->curRd = (hndl->curRd+1)%hndl->len;
				hndl->count--;
				status = OSA_SOK;
				pthread_cond_signal(&hndl->condWr);
				break;
			} else {
				if(timeout == OSA_TIMEOUT_NONE)
					break;
				status = pthread_cond_wait(&hndl->condRd, &hndl->lock);
			}
		}
		pthread_mutex_unlock(&hndl->lock);
		return status;
	}

	struct timespec timer;
	maketimeout(&timer,timeout);
	
	pthread_mutex_lock(&hndl->lock);
	while(1) {
		if(hndl->count > 0) {
			if(value!=NULL) {
				*value = hndl->queue[hndl->curRd];
			}

			hndl->curRd = (hndl->curRd+1)%hndl->len;
			hndl->count--;
			status = OSA_SOK;
			pthread_cond_signal(&hndl->condWr);
			break;
		} 
		else {
			ret = pthread_cond_timedwait(&hndl->condRd, &hndl->lock, &timer);
			if(ret==ETIMEDOUT)
			{
				status = OSA_EFAIL;
				//OSA_printf(" %s time out !!!\r\n",__func__);
				break;
			}
			else
			{
				//OSA_printf(" %s hndl->count %d!!!\r\n",__func__,hndl->count);
				if(hndl->count > 0){
					if(value!=NULL) {
						*value = hndl->queue[hndl->curRd];
					}

					hndl->curRd = (hndl->curRd+1)%hndl->len;
					hndl->count--;
					status = OSA_SOK;
					pthread_cond_signal(&hndl->condWr);
				} 
				break;
			}
		}
	}

	pthread_mutex_unlock(&hndl->lock);
	return status;
}

Uint32 OSA_queGetQueuedCount(OSA_QueHndl *hndl)
{
  Uint32 queuedCount = 0;

  pthread_mutex_lock(&hndl->lock);
  queuedCount = hndl->count;
  pthread_mutex_unlock(&hndl->lock);
  return queuedCount;
}

int OSA_quePeek(OSA_QueHndl *hndl, Int32 *value)
{
  int status = OSA_EFAIL;
  pthread_mutex_lock(&hndl->lock);
  if(hndl->count > 0 ) {
      if(value!=NULL) {
        *value = hndl->queue[hndl->curRd];
        status = OSA_SOK;
      }
  }
  pthread_mutex_unlock(&hndl->lock);

  return status;
}

Bool OSA_queIsEmpty(OSA_QueHndl *hndl)
{
  Bool isEmpty;

  pthread_mutex_lock(&hndl->lock);
  if (hndl->count == 0)
  {
      isEmpty = TRUE;
  }
  else
  {
      isEmpty = FALSE;
  }
  pthread_mutex_unlock(&hndl->lock);

  return isEmpty;
}



