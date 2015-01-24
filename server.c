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

#define    MAXLINE         1024*16   /*数据包大小*/
#define    SERVER_PORT     6666  /*端口号*/
#define    OFFSET         1024*1024*128      /*文件指针偏移量 */
#define    FILE_NAME_MAX_SIZE 512 

void fileTransfer(void *arg);    /*线程*/
static  int  threadCount = 0;  /*线程计数器*/

pthread_mutex_t alock = PTHREAD_MUTEX_INITIALIZER;  /*线程锁*/
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
        perror("创建socket失败\n");
        exit(0);
    }

    /*初始化服务端*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    /*绑定端口*/
    if (bind(sock_id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("端口绑定失败\n");
        exit(0);
    }

    if (-1 == listen(sock_id, 10)) 
    {
        perror("端口监听失败\n");
        exit(0);
    }

    /*等待客户端连接*/
    while (1) 
    {
        clie_addr_len = sizeof(clie_addr);
		link_id = (int *)malloc(sizeof(int));
		
		/*客户端请求到达时建立连接*/
        *link_id = accept(sock_id, (struct sockaddr *)&clie_addr, &clie_addr_len);  
    	if (-1 == *link_id) 
		{
            perror("接收端口失败\n");
            continue;
        }

		/*创建线程处理客户端请求*/
		while (pthread_create (&thread_id, NULL, (void *)fileTransfer, (void *)link_id) != 0)   
        {    
            printf ("线程创建失败: %s.\n\n", strerror(errno));
			break;          
        }
    }
    printf("%s 传输成功!\n ", file_name);
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

	/*互斥锁*/
	pthread_mutex_lock(&alock);
	threadCount++;
	threadIndex = threadCount;
	pthread_mutex_unlock(&alock);

	/*接收客户端请求的文件名*/
	if((recv_len = recv(*link_id, file_name, FILE_NAME_MAX_SIZE, 0)) == -1) 
	{
		printf("文件名错误, error = %d\n", errno);
		exit(1);
	}

	/*打开文件*/
	if ((fp = fopen(file_name, "r")) == NULL)
	{
		perror("打开文件失败\n");
		exit(0);
	}

	/*定位读指针*/
	fseek(fp, OFFSET*(threadIndex-1), 0);
	
	/*发送线程索引给客户端*/
	sprintf(buf, "%d", threadIndex);
	if((send_len = send(*link_id, buf, sizeof(buf), 0)) < sizeof(buf))
	{
		perror("发送线程id失败\n");
		exit(0);
	}
	bzero(buf, MAXLINE);
	while ((read_len = fread(buf, sizeof(char), MAXLINE, fp)) >0)  /*读数据至缓冲区 */
	{
		send_len = send(*link_id, buf, read_len, 0);   /*发送数据*/
		if ( send_len < read_len ) 
		{
			perror("文件发送失败\n");
			exit(0);
		}
	    bzero(buf, MAXLINE);
		if(ftell(fp) >= OFFSET*threadIndex)    /*读至边界退出线程*/
			break;
	}
    fclose(fp);
    close(*link_id);
	pthread_exit(NULL);
}
