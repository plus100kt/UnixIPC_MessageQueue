#include <stdio.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/msg.h>
#include "message.h"
#include <time.h>


void checkUsableClient(int msqid);
void choiceClient(int msqid, int *client);
void *makeCar(void *ptr);
void *paintCar(void *ptr);
void *inspectCar(void *ptr);
void getComponents();

static sem_t sync_sem;
int components = 0;

typedef struct Car
{
    bool isCreated;
    bool isPainted;
    bool isInspected;
    struct Car *next;
} Car;

void main()
{
    int initsetval = 0;
    int client;
    key_t key = (key_t)60110;
    int msqid;
    struct message msg;
    struct timespec time;
    struct timespec begin;
    struct timespec end;

    
    // 메시지큐 연결 (없으면 오류)
    if ((msqid = msgget(key, IPC_EXCL | 0666)) == -1)
    {
        printf("서버에서 메시지를 생성해주세요\n");
    }

    checkUsableClient(msqid);
    choiceClient(msqid, &client);

    // // 맥북테스트용
    // if ((sync_sem = sem_open("/semaphore", O_CREAT, 0644, 1)) == SEM_FAILED) {
    //     perror("sem_open");;
    // }
    if (sem_init(&sync_sem, 0, 1) == -1)
    {
        perror("sem_init");
        exit(1);
    }

    pthread_t thread[3];
    Car *headCar = (Car *)malloc(sizeof(Car));
    headCar->next = NULL;

    int err_code = pthread_create(&thread[0], NULL, makeCar, (void *)headCar); // 차를 만든다.
    if (err_code)
    {
        printf("ERROR code is %d\n", err_code);
        exit(1);
    }
    //printf("point 6\n");
    err_code = 0;
    err_code = pthread_create(&thread[1], NULL, paintCar, (void *)headCar); // 차를 도색한다.
    if (err_code)
    {
        printf("ERROR code is %d\n", err_code);
        exit(1);
    }
    //printf("point 7\n");
    err_code = 0;
    err_code = pthread_create(&thread[2], NULL, inspectCar, (void *)headCar); // 차를 검사한 후 출고한다.
    //printf("point 8\n");
    if (err_code)
    {
        printf("ERROR code is %d\n", err_code);
        exit(1);
    }

    while (1)
    {
        if (components < 10)
        {
            printf("component get ! \n");
            sem_wait(&sync_sem);
            
            // 부품 요청하기
            msg.msg_type = 2;
            msg.data.client_num = client;
            msg.data.attr = 1;
            msg.data.time.tv_nsec = 0;
            msg.data.time.tv_sec = 0;
            if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
                printf("msgsnd failed\n");
                exit(0);
            }

            // 요청한 부품 받기
            if(msgrcv(msqid, &msg, sizeof(struct clientData), 3 ,0)==-1){
                printf("msgrcv failed\n");
                exit(0);
            }

            // 시간 측정
            struct timespec time;
            if( clock_gettime(CLOCK_MONOTONIC, &time) == -1 ) {
                perror( "clock gettime" );
            }

            begin = msg.data.time;
            end = time;

            if (end.tv_nsec - begin.tv_nsec < 0)
            {
                printf("Client %d : %ld.%09ld sec\n", msg.data.client_num, end.tv_sec - begin.tv_sec - 1, end.tv_nsec - begin.tv_nsec + 1000000000);
            }
            else
            {
                printf("Client %d : %ld.%09ld sec\n", msg.data.client_num, end.tv_sec - begin.tv_sec, end.tv_nsec - begin.tv_nsec);
            }
            components++;
            
            sem_post(&sync_sem);
        }
    }

}

void choiceClient(int msqid, int *client)
{
    struct message msg;

    printf("사용할 키 입력 : ");
    scanf("%d", client);

    for (int i = 0; i < 10; i++) {
        // 사용가능여부 메시지 받기
        if (msgrcv(msqid, &msg, sizeof(struct clientData), 1, 0) == -1)
        {
            printf("msgrcv failed\n");
            exit(0);
        }
        if (*client == msg.data.client_num) {
            // 선택한 클라이언트 사용가능 처리해서 다시 큐에 넣기
            msg.data.attr = 1;
            if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
                printf("msgsnd failed\n");
                exit(0);
            }
        } else {
            // 이외 받은메시지 다시 큐에 넣기
            if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
                printf("msgsnd failed\n");
                exit(0);
            }
        }
    }
}

void checkUsableClient(int msqid)
{
    struct message msg;

    for (int i = 0; i < 10; i++)
    {
        // 사용가능여부 메시지 받기
        if (msgrcv(msqid, &msg, sizeof(struct clientData), 1, 0) == -1)
        {
            printf("msgrcv failed\n");
            exit(0);
        }
        // 받은메시지 다시 큐에 넣기
        if(msgsnd(msqid, &msg ,sizeof(struct clientData), 0)==-1){
            printf("msgsnd failed\n");
            exit(0);
        }

        if (msg.data.attr == 0)
        {
            printf("%d - 사용 가능\n", msg.data.client_num);
        }
        else
        {
            printf("%d - 사용 불가\n", msg.data.client_num);
        }
    }
}

void *makeCar(void *ptr)
{
    int i = 0;
    Car *current_made_car = ptr;
    for (;;)
    {
        if (components > 0)
        {
            sleep(3);
            sem_wait(&sync_sem);
            components--; // 부품 갯수 빼주는 녀석
            sem_post(&sync_sem);

            current_made_car->next = (Car *)malloc(sizeof(Car));
            current_made_car = current_made_car->next;
            current_made_car->isCreated = true;
            current_made_car->isPainted = false;
            current_made_car->isInspected = false;
            current_made_car->next = NULL;
            printf("car %d is created, client have %d components\n", i, components);
            i++;
        }
    }
}

void *paintCar(void *ptr)
{
    int i = 0;
    Car *current_painted_car = ptr;
    for (;;)
    {
        if ((current_painted_car->next != NULL) && current_painted_car->next->isCreated)
        {
            sleep(1);
            current_painted_car = current_painted_car->next;
            current_painted_car->isPainted = true;
            printf("car %d is painted\n", i);
            i++;
        }
    }
}

void *inspectCar(void *ptr)
{
    int i = 0;
    Car *current_inspect_target = ptr;
    for (;;)
    {
        if (current_inspect_target->next != NULL && current_inspect_target->next->isPainted)
        {
            sleep(2);
            Car *inpected_car = current_inspect_target;
            current_inspect_target = current_inspect_target->next;
            current_inspect_target->isInspected = true;
            free(inpected_car);
            printf("car %d is inspected\n", i);
            i++;
        }
    }
}
