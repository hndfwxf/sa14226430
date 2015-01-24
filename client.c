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

#define    MAXLINE        1024*16  /*数据包大小*/
#define    SERVER_PORT    6666    /*端口号*/
#define    OFFSET         1024*1024*128      /*文件指针偏移量 */
#define    FILE_NAME_MAX_SIZE 512    
#define    THREAD_NUM       8             /*线程数*/

char  file_name[FILE_NAME_MAX_SIZE]="test";  /*设置默认传输的文件名*/
void* connectToServer(void* arg);   /*声明连接服务端函数*/

int main(int argc,char **argv)
{
    struct sockaddr_in     serv_addr;
    time_t                 t_start,t_end;
    pthread_t              tempthread_id[THREAD_NUM]; 
    int                    i; 

    /*初始化服务端地址*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


     /* 开始计时*/
     t_start=time(NULL);   
     for (i=0; i<THREAD_NUM; i++) 
     {   
         while (pthread_create (&tempthread_id[i], NULL, (void *)connectToServer, (void *)&serv_addr) != 0)     //创建线程
         {    
               printf ("线程创建失败: %s.\n\n", strerror(errno));          
         } 

     } 

    /*等待线程退出*/

    for (i=0; i<THREAD_NUM; i++)  
    {
		pthread_join(tempthread_id[i], NULL);
		printf(" 线程id = %d 退出!\n", tempthread_id[i]);
    } 

    /*结束计时*/
    t_end=time(NULL);  
    printf("共用时%.0fs\n",difftime(t_end,t_start));
    printf("%s 传输成功!\n ", file_name);
    return 0;
}


void *connectToServer(void *arg)
{
    char                   buf[MAXLINE];  /*文件缓冲区*/
    int                    sock_id;
    int                    recv_len;
    int                    write_len;
    int                    send_len;
    FILE                   *fp;
    int                    i_ret;
    int                    threadIndex;         /*服务端线程索引*/ 

    if((sock_id = socket (AF_INET, SOCK_STREAM, 0)) == 0)  
    {   
        printf ("端口错误: %s.\n\n", strerror(errno));  
    }   

     /* 准备写入文件*/   
    fp = fopen(file_name, "w");    
    if(NULL == fp)    
    {   
		printf("文件\t%s 无法写入\n", file_name);       
		exit(1);    
    } 

    /*请求连接*/
    i_ret = connect(sock_id, (struct sockaddr*)arg, sizeof(struct sockaddr));
    if (-1 == i_ret) 
    {
        printf("连接端口失败\n");
		return NULL;
    }

    /*发送文件名给服务器*/
    if((send_len = send(sock_id, file_name, sizeof(file_name), 0)) < sizeof(file_name))
    {
		printf("发送文件名失败\n");
		exit(1); 
    }

    /*从服务端接收线程索引*/
    if(recv(sock_id, buf, MAXLINE, 0) < 0)  
    {
		printf("线程 id = %d, 接收线程id失败\n", pthread_self());
		exit(1); 
    }
	
    threadIndex = atoi(buf);
    /*指定写入位置*/
    fseek(fp, OFFSET*(threadIndex-1), 0);
    bzero(buf, MAXLINE);

    while (recv_len = recv(sock_id, buf, MAXLINE, 0)) 
    {   
        if(recv_len <= 0) 
		{
            printf("接收服务端数据失败!\n");
            break;
        }
        write_len = fwrite(buf, sizeof(char), recv_len, fp);    /*把缓冲区数据写入文件*/
        if (write_len < recv_len ) 
		{
            printf("写文件失败\n");
            break;
        }
        bzero(buf, MAXLINE);
    }
    fclose(fp);
    close(sock_id);
    pthread_exit(NULL);

}

