#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "message.h"

#define MEMSIZE 4;

void provide(int msqid, struct message msg);
void *componentsFactory();

int components = 0;
static sem_t sync_sem;

void main()
{
    int msqid;
    int clientnum = 0;
    key_t key = (key_t)60110;
    pthread_t threads[3];
    int rc;
    struct message msg;

    // // 맥북 테스트용
    // if ((sync_sem = sem_open("/semaphore", O_CREAT, 0644, 1)) == SEM_FAILED) {
    //     perror("sem_open");
    //     exit(EXIT_FAILURE);
    // }
    if (sem_init(&sync_sem, 0, 1) == -1)
    {
        perror("sem_open");
        exit(1);
    }

    for (int i = 0; i < 10; i++)
    {
        // 메시지큐 생성
        if ((msqid = msgget(key, IPC_CREAT | 0666)) == -1)
        {
            printf("msgget faild\n");
        }
        // 미사용상태 메시지 보내기
        msg.msg_type = 1;
        msg.data.client_num = i;
        msg.data.attr = 0;
        if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
            printf("msgsnd failed\n");
            exit(0);
        }
    }

    for (int i = 0; i < 3; i++)
    {
        printf("In main : creating thread %d\n", i);
        rc = pthread_create(&threads[i], NULL, componentsFactory, NULL);
        if (rc)
        {
            printf("[ERROR] : return code from pthread_create is %d\n", rc);
            exit(-1);
        }
    }

    while (1)
    {
        // 부품요청 메시지만 받기
        if (msgrcv(msqid, &msg, sizeof(struct clientData), 2, 0) == -1)
        {
            printf("msgrcv failed\n");
            exit(0);
        }
        provide(msqid, msg);
    }
}

void provide(int msqid, struct message msg)
{
    printf("client num : %d\n", msg.data.client_num);
    sem_wait(&sync_sem);
    components--;
    // 요청한 부품 보내기
    // msg_type 제외하고 생략 가능하지면 명시적으로 표현
    msg.msg_type = 3;
    msg.data.client_num = 1;
    msg.data.attr = 1;
    if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
        printf("msgsnd failed\n");
        exit(0);
    }
    sem_post(&sync_sem);
    sleep(1);
}

void *componentsFactory()
{
    srand(time(NULL));
    int productivity = rand() % 4 + 5;

    while (1)
    {
        if (components < 20)
        {
            sem_wait(&sync_sem);
            components++;
            sem_post(&sync_sem);
            printf("Thread : Component Supplied, (Quentity : %d)\n", components);
        }

        sleep(productivity);
    }
}