#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define    MAXLINE        1024*16  /*���ݰ���С*/
#define    SERVER_PORT    6666    /*�˿ں�*/
#define    OFFSET         1024*1024*128      /*�ļ�ָ��ƫ���� */
#define    FILE_NAME_MAX_SIZE 512    
#define    THREAD_NUM       8             /*�߳���*/

char  file_name[FILE_NAME_MAX_SIZE]="test";  /*����Ĭ�ϴ�����ļ���*/
void* connectToServer(void* arg);   /*�������ӷ���˺���*/

int main(int argc,char **argv)
{
    struct sockaddr_in     serv_addr;
    time_t                 t_start,t_end;
    pthread_t              tempthread_id[THREAD_NUM]; 
    int                    i; 

    /*��ʼ������˵�ַ*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


     /* ��ʼ��ʱ*/
     t_start=time(NULL);   
     for (i=0; i<THREAD_NUM; i++) 
     {   
         while (pthread_create (&tempthread_id[i], NULL, (void *)connectToServer, (void *)&serv_addr) != 0)     //�����߳�
         {    
               printf ("�̴߳���ʧ��: %s.\n\n", strerror(errno));          
         } 

     } 

    /*�ȴ��߳��˳�*/

    for (i=0; i<THREAD_NUM; i++)  
    {
		pthread_join(tempthread_id[i], NULL);
		printf(" �߳�id = %d �˳�!\n", tempthread_id[i]);
    } 

    /*������ʱ*/
    t_end=time(NULL);  
    printf("����ʱ%.0fs\n",difftime(t_end,t_start));
    printf("%s ����ɹ�!\n ", file_name);
    return 0;
}


void *connectToServer(void *arg)
{
    char                   buf[MAXLINE];  /*�ļ�������*/
    int                    sock_id;
    int                    recv_len;
    int                    write_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;
    int                    threadIndex;         /*������߳�����*/ 

    if((sock_id = socket (AF_INET, SOCK_STREAM, 0)) == 0)  
    {   
        printf ("�˿ڴ���: %s.\n\n", strerror(errno));  
    }   

     /* ׼��д���ļ�*/   
    fp = fopen(file_name, "w");    
    if(NULL == fp)    
    {   
		printf("�ļ�\t%s �޷�д��\n", file_name);       
		exit(1);    
    } 

    /*��������*/
    i_ret = connect(sock_id, (struct sockaddr*)arg, sizeof(struct sockaddr));
    if (-1 == i_ret) 
    {
        printf("���Ӷ˿�ʧ��\n");
		return NULL;
    }

    /*�����ļ�����������*/
    if((send_len = send(sock_id, file_name, sizeof(file_name), 0)) < sizeof(file_name))
    {
		printf("�����ļ���ʧ��\n");
		exit(1); 
    }

    /*�ӷ���˽����߳�����*/
    if(recv(sock_id, buf, MAXLINE, 0) < 0)  
    {
		printf("�߳� id = %d, �����߳�idʧ��\n", pthread_self());
		exit(1); 
    }
	
    threadIndex = atoi(buf);
    /*ָ��д��λ��*/
    fseek(fp, OFFSET*(threadIndex-1), 0);
    bzero(buf, MAXLINE);

    while (recv_len = recv(sock_id, buf, MAXLINE, 0)) 
    {   
        if(recv_len <= 0) 
		{
            printf("���շ��������ʧ��!\n");
            break;
        }
        write_len = fwrite(buf, sizeof(char), recv_len, fp);    /*�ѻ���������д���ļ�*/
        if (write_len < recv_len ) 
		{
            printf("д�ļ�ʧ��\n");
            break;
        }
        bzero(buf, MAXLINE);
    }
    fclose(fp);
    close(sock_id);
    pthread_exit(NULL);

}

