#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define    MAXLINE         1024*16   /*���ݰ���С*/
#define    SERVER_PORT     6666  /*�˿ں�*/
#define    OFFSET         1024*1024*128      /*�ļ�ָ��ƫ���� */
#define    FILE_NAME_MAX_SIZE 512 

void fileTransfer(void *arg);    /*�߳�*/
static  int  threadCount = 0;  /*�̼߳�����*/

pthread_mutex_t alock = PTHREAD_MUTEX_INITIALIZER;  /*�߳���*/
char  file_name[FILE_NAME_MAX_SIZE];   

int main(int argc,char **argv) 
{
    struct sockaddr_in    serv_addr;
    struct sockaddr_in    clie_addr;
    int                   sock_id;
    int                   clie_addr_len;
    pthread_t             thread_id;
    int                   *link_id; 

    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("����socketʧ��\n");
        exit(0);
    }

    /*��ʼ�������*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    /*�󶨶˿�*/
    if (bind(sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("�˿ڰ�ʧ��\n");
        exit(0);
    }

    if (-1 == listen(sock_id, 10)) 
    {
        perror("�˿ڼ���ʧ��\n");
        exit(0);
    }

    /*�ȴ��ͻ�������*/
    while (1) 
    {
        clie_addr_len = sizeof(clie_addr);
		link_id = (int *)malloc(sizeof(int));
		
		/*�ͻ������󵽴�ʱ��������*/
        *link_id = accept(sock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);  
    	if (-1 == *link_id) 
		{
            perror("���ն˿�ʧ��\n");
            continue;
        }

		/*�����̴߳���ͻ�������*/
		while (pthread_create (&thread_id, NULL, (void *)fileTransfer, (void *)link_id) != 0)   
        {    
            printf ("�̴߳���ʧ��: %s.\n\n", strerror(errno));
			break;          
        }
    }
    printf("%s ����ɹ�!\n ", file_name);
    close(sock_id); 
    return 0;
}



void fileTransfer(void *arg)
{ 
	int                    *link_id;
    FILE                   *fp;
   	int                    read_len;
   	int                    send_len;
	int                    recv_len;
    char                   buf[MAXLINE];
	int                    threadIndex;

	link_id = (int *)malloc(sizeof(int));
	*link_id = *((int*)arg);

	/*������*/
	pthread_mutex_lock(&alock);
	threadCount++;
	threadIndex = threadCount;
	pthread_mutex_unlock(&alock);

	/*���տͻ���������ļ���*/
	if((recv_len = recv(*link_id, file_name, FILE_NAME_MAX_SIZE, 0)) == -1) 
	{
		printf("�ļ�������, error = %d\n", errno);
		exit(1);
	}

	/*���ļ�*/
	if ((fp = fopen(file_name, "r")) == NULL)
	{
		perror("���ļ�ʧ��\n");
		exit(0);
	}

	/*��λ��ָ��*/
	fseek(fp, OFFSET*(threadIndex-1), 0);
	
	/*�����߳��������ͻ���*/
	sprintf(buf, "%d", threadIndex);
	if((send_len = send(*link_id, buf, sizeof(buf), 0)) < sizeof(buf))
	{
		perror("�����߳�idʧ��\n");
		exit(0);
	}
	bzero(buf, MAXLINE);
	while ((read_len = fread(buf, sizeof(char), MAXLINE, fp)) >0)  /*�������������� */
	{
		send_len = send(*link_id, buf, read_len, 0);   /*��������*/
		if ( send_len < read_len ) 
		{
			perror("�ļ�����ʧ��\n");
			exit(0);
		}
	    bzero(buf, MAXLINE);
		if(ftell(fp) >= OFFSET*threadIndex)    /*�����߽��˳��߳�*/
			break;
	}
    fclose(fp);
    close(*link_id);
	pthread_exit(NULL);
}
