/*
 * ftp_server.c
 *
 *  Created on: 2018年1月8日
 *      Author: ginger
 */
#include "ftp_server.h"

int main(int argc,char *argv[])
{
	read_ROOTPATH();

	printf("ROOTPATH==%s\n",ROOTPATH);
	chdir(ROOTPATH);
	pthread_t thread;
	struct ARG arg; //存放客户端的标识符和地址
	struct sockaddr_in server; //服务端地址

//	if(chroot(ROOTPATH) == -1 )
//   {
//		   printf("chroot erro：please run as root!\n");
//		   //  exit(0);
//   }
 //   创建Socket， 返回一个套接口描述符<--ftp_server_sock
	if((ftp_server_sock=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("Creating socket failed");
		exit(1);
	}
	char t_dir[DIR_INFO];
	printf("%s\n",getcwd(t_dir, DIR_INFO));
	printf("创建成功\n");
	int opt=SO_REUSEADDR;
	//设置地址可重用
	setsockopt(ftp_server_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	bzero(&server,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(FTP_SERVER_PORT);  //设置ftp服务器的控制端口为8000
	server.sin_addr.s_addr=htonl(INADDR_ANY); //监听服务器的本地网卡ip地址

	//socket绑定端口
	if(bind(ftp_server_sock,(struct sockaddr *)&server,sizeof(struct sockaddr))==-1)
	{
		perror("Bind error");
		exit(1);
	}
	printf("绑定端口成功\n");
    //监听， 第二个参数是进入队列中允许的连接的个数，在这里是允许5个
	if(listen(ftp_server_sock,LISTEN_QENU)==-1)
	{
		perror("listen error");
		exit(1);
	}
	printf("监听成功\n");
   //客户端的窗口描述符号以及地址
	int ftp_client_sock;
	struct sockaddr_in client;
	int sin_size=sizeof(struct sockaddr_in);



	while(1)
	{
		printf("等待链接....\n");

		//接受链接
		if((ftp_client_sock=accept(ftp_server_sock,(struct sockaddr *)&client,&sin_size))==-1)
		{
			perror("accept error");
			exit(1);
		}
		printf("链接成功....\n");
		arg.client_sock=ftp_client_sock;
		memcpy((void*)&arg.client,&client,sizeof(client));

		//创建线程
		if(pthread_create(&thread,NULL,Handle_Client_Request,(void*)&arg))
		{
			perror("thread create error ");
			exit(1);
		}
		printf("创建线程成功....\n");

	}
	//关闭链接
	close(ftp_server_sock);
}

//客户端处理函数的主要过程
void *Handle_Client_Request(void* arg)
{
	printf("处理主要函数\n");
	struct ARG*info;
	info=(struct ARG*)arg;
	//inet_ntoa：网络地址转换成“.”点隔的字符串格式
	//printf("getting a connection from %s\n",inet_ntoa(info->client.sin_addr));

	//处理函数
	do_client_work(info->client_sock,info->client);
	//关闭客户端，以及线程退出
	close(info->client_sock);
	pthread_exit(NULL);
}

//具体过程
void do_client_work(int client_sock,struct sockaddr_in client)
{
	printf("具体函数\n");
	int login_flag;
	login_flag=login(client_sock); //1为root,2为匿名，0为错误，若为0就直接结束函数

	//root用户的处理
	while(recv_client_info(client_sock)&&login_flag==1)
	{
		printf("root用户等待命令\n");
		if((strncmp("quit", client_Control_Info, 4) == 0)||(strncmp("QUIT", client_Control_Info, 4) == 0))
		{
				send_client_info(client_sock, serverInfo221, strlen(serverInfo221)); // 221  服务正在关闭控制连接
				break;
		}
		else if((strncmp("close",client_Control_Info,5) == 0)||(strncmp("CLOSE",client_Control_Info,5) == 0))
		{
			printf("Client Quit!\n");
            shutdown(client_sock,SHUT_WR);
		}

		else if(strncmp("pwd", client_Control_Info, 3) == 0||(strncmp("PWD", client_Control_Info, 3) == 0))
		{
			char 	pwd_info[MSG_INFO];
   			char 	tmp_dir[DIR_INFO];
    		snprintf(pwd_info, MSG_INFO, "257 \"%s\" is current location.\r\n", getcwd(tmp_dir, DIR_INFO));
			send_client_info(client_sock, pwd_info, strlen(pwd_info));
		}
		else if(strncmp("cwd", client_Control_Info, 3) == 0||(strncmp("CWD", client_Control_Info, 3) == 0))
		{
			printf("接收到CWD\n");
				handle_cwd(client_sock);
		}
		else if(strncmp("mkd",client_Control_Info,3) ==0||(strncmp("MKD",client_Control_Info,3)==0))
		{
				handle_mkd(client_sock);
		}
		else if(strncmp("rmd",client_Control_Info,3)==0||(strncmp("RMD",client_Control_Info,3)==0))
		{
			handle_rmd(client_sock);
		}
		else if(strncmp("dele",client_Control_Info,4)==0||(strncmp("DELE",client_Control_Info,4)==0))
		{
				handle_del(client_sock);

		}
		else if(strncmp("pasv", client_Control_Info, 4) == 0||(strncmp("PASV", client_Control_Info, 4) == 0))
		{
			printf("接收到pasv\n");
			   handle_pasv(client_sock,client);
		}
		else if(strncmp("list", client_Control_Info, 4) == 0||(strncmp("LIST", client_Control_Info, 4) == 0))
		{
			printf("接收到LIST\n");
			   handle_list(client_sock);
			   send_client_info(client_sock,serverInfo226, strlen(serverInfo226));
		}
		else if(strncmp("type", client_Control_Info, 4) == 0||(strncmp("TYPE", client_Control_Info, 4) == 0))
		{
				if(strncmp("type I", client_Control_Info, 6) == 0||(strncmp("TYPE I", client_Control_Info, 6) == 0))
			  			translate_data_mode=FILE_TRANS_MODE_BIN;
			  	send_client_info(client_sock, serverInfo200, strlen(serverInfo200));
		}
		else if(strncmp("retr", client_Control_Info, 4) == 0||(strncmp("RETR", client_Control_Info, 4) == 0))
		{  //从服务器下载文件
				handle_file(client_sock);
				send_client_info(client_sock,serverInfo226, strlen(serverInfo226));
		}
		else if(strncmp("stor", client_Control_Info, 4) == 0||(strncmp("STOR", client_Control_Info, 4) == 0))
		{ //文件上传到服务器
				handle_file(client_sock);
				send_client_info(client_sock,serverInfo226, strlen(serverInfo226));
		}
		else if(strncmp("syst", client_Control_Info, 4) == 0||(strncmp("SYST", client_Control_Info, 4) == 0))
		{//返回服务器的系统类型，响应码215
				send_client_info(client_sock, serverInfo215, strlen(serverInfo215));
		}
		else if(strncmp("site", client_Control_Info, 4) == 0||(strncmp("SITE", client_Control_Info, 4) == 0))
		{
			    handle_site(client_sock);
		}
		else if(strncmp("feat", client_Control_Info, 4) == 0||(strncmp("FEAT", client_Control_Info, 4) == 0))
		{
				send_client_info(client_sock, serverInfo211, strlen(serverInfo211));
		}
		else if(strncmp("rest", client_Control_Info, 4) == 0||(strncmp("REST", client_Control_Info, 4) == 0))
		{//由特定偏移量重启文件传递，350 	文件行为暂停
				send_client_info(client_sock, serverInfo350, strlen(serverInfo350));
		}
		else
		{
				send_client_info(client_sock, serverInfo, strlen(serverInfo));
		}

	}
//匿名用户的处理
	while(recv_client_info(client_sock)&&(login_flag == 2))
        {
                if((strncmp("quit", client_Control_Info, 4) == 0)||(strncmp("QUIT", client_Control_Info, 4) ==0))
                {
                                send_client_info(client_sock, serverInfo221, strlen(serverInfo221));
                                break;
                }
                else if((strncmp("close",client_Control_Info,5) == 0)||(strncmp("CLOSE",client_Control_Info,5)== 0))
                {
                        printf("Client Quit!\n");
                        shutdown(client_sock,SHUT_WR);
                       // shutdown(ftp_data_sock,SHUT_RDWR);
                }

                else if(strncmp("pwd", client_Control_Info, 3) == 0||(strncmp("PWD", client_Control_Info, 3) == 0))
                {
                        char    pwd_info[MSG_INFO];
                        char    tmp_dir[DIR_INFO];
                        snprintf(pwd_info, MSG_INFO, "257 \"%s\" is current location.\r\n", getcwd(tmp_dir, DIR_INFO));
                                send_client_info(client_sock, pwd_info, strlen(pwd_info));
                }
                else if(strncmp("cwd", client_Control_Info, 3) == 0||(strncmp("CWD", client_Control_Info, 3) == 0))
                {
                                handle_cwd(client_sock);
                }
               else if(strncmp("pasv", client_Control_Info, 4) == 0||(strncmp("PASV", client_Control_Info, 4)== 0))
                {
                           handle_pasv(client_sock,client);
                }
                else if(strncmp("list", client_Control_Info, 4) == 0||(strncmp("LIST", client_Control_Info, 4)== 0))
                {
                           handle_list(client_sock);
                           send_client_info(client_sock,serverInfo226, strlen(serverInfo226));
                }
                else if(strncmp("type", client_Control_Info, 4) == 0||(strncmp("TYPE", client_Control_Info, 4)== 0))
                {
                                if(strncmp("type I", client_Control_Info, 6) == 0||(strncmp("TYPE I", client_Control_Info, 6) == 0))
                                                translate_data_mode=FILE_TRANS_MODE_BIN;
                                send_client_info(client_sock, serverInfo200, strlen(serverInfo200));
                }
                else if(strncmp("retr", client_Control_Info, 4) == 0||(strncmp("RETR", client_Control_Info, 4)== 0))
                {
                                handle_file(client_sock);
                                send_client_info(client_sock,serverInfo226, strlen(serverInfo226));
                }
       		    else if(strncmp("syst", client_Control_Info, 4) == 0||(strncmp("SYST", client_Control_Info, 4)== 0))
                {
                                send_client_info(client_sock, serverInfo215, strlen(serverInfo215));
                }
                else if(strncmp("size", client_Control_Info, 4) == 0||(strncmp("SIZE", client_Control_Info, 4)== 0))
                {
                                send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
                }
                else if(strncmp("feat", client_Control_Info, 4) == 0||(strncmp("FEAT", client_Control_Info, 4)== 0))
                {
                                send_client_info(client_sock, serverInfo211, strlen(serverInfo211));
                }
                else if(strncmp("rest", client_Control_Info, 4) == 0||(strncmp("REST", client_Control_Info, 4)== 0))
                {
                                send_client_info(client_sock, serverInfo350, strlen(serverInfo350));
                }
                else
                {
                                send_client_info(client_sock, serverInfo, strlen(serverInfo));
                }

	}
}

int login(int client_sock)
{
	printf("登录函数\n");

	//当客户端控制信息开头为user时，是成功链接
	send_client_info(client_sock, serverInfo220, strlen(serverInfo220));
	while(1)
	{
		if(recv_client_info(client_sock)==2)break;
		else	send_client_info(client_sock, serverInfo, strlen(serverInfo));//202,命令还没有实现

	}

	//检验用户帐号，root的flag为1,匿名的为2,其他为0
	int flag=0;
	int i=0;
	int length=strlen(client_Control_Info);
	for(i=5;i<length;i++)
	{
		    login_user[i-5]=client_Control_Info[i];
			format_client_Info[i-5]=client_Control_Info[i];
	}
	login_user[i-7]='\0';
	format_client_Info[i-7]='\0';
	if(strncmp(format_client_Info, default_user, 4) == 0)
	{
		flag=1;
	}

	if(strncmp(format_client_Info, anony_user, 9) == 0)
     {
                flag=2;
     }

    //检查密码
   	send_client_info(client_sock, serverInfo331, strlen(serverInfo331));//   331  登录需要帐号,检验密码
	recv_client_info(client_sock);

	printf("检查密码信息\n");

	length=strlen(client_Control_Info);
	for(i=5;i<length;i++)
	{
		login_pass[i-5]=client_Control_Info[i];
		format_client_Info[i-5]=client_Control_Info[i];
	}
	login_pass[i-7]='\0';
	format_client_Info[i-7]='\0';

	int flag_user=0;
	flag_user=read_system_user();
//	if(strncmp(format_client_Info, default_pass, 4) == 0&&flag==1)
//   	{
//   		send_client_info(client_sock, serverInfo230, strlen(serverInfo230));//230  用户已登录，请继续。root
//   		return 1;
//   	}
//	else if(strncmp(format_client_Info, anony_pass, 9) == 0&&flag==2)
//    {
//		printf("匿名登录\n");
//        send_client_info(client_sock, serverInfo230, strlen(serverInfo230)); //531非root用户登录
//        return 2;
//     }
//   	else
//  	{
//  		send_client_info(client_sock, serverInfo530, strlen(serverInfo530));//  530  无法登录。
//  		return 0;
//     }
	if(flag_user==1)
	{
		send_client_info(client_sock, serverInfo230, strlen(serverInfo230));//230  用户已登录，请继续。root
		   		return 1;
	}
	else
	{
  		send_client_info(client_sock, serverInfo530, strlen(serverInfo530));//  530  无法登录。
  		return 0;
	}

}
//切换目录用到chdir
void handle_cwd(int client_sock)
{
	char 	cwd_info[MSG_INFO];
   	char 	tmp_dir[DIR_INFO];
   	char 	client_dir[DIR_INFO];

   	char t_dir[DIR_INFO];
   	int dirlength=-1;
   	int length=strlen(client_Control_Info);
     	int i=0;
   	for(i=4;i<length;i++)
			format_client_Info[i-4]=client_Control_Info[i];
		format_client_Info[i-6]='\0';

		if(strncmp(getcwd(t_dir, DIR_INFO),format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-10)!=0)
		{
			getcwd(client_dir, DIR_INFO);
			dirlength=strlen(client_dir);
			client_dir[dirlength]='/';
		}

      	for(i=4;i<length;i++)
   		{
   			client_dir[dirlength+i-3]=client_Control_Info[i];
   		}
		client_dir[dirlength+i-5]='\0';

		if(strncmp(ROOTPATH,client_dir,strlen(ROOTPATH)-1)!=0)
		{
			snprintf(cwd_info, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
			send_client_info(client_sock, cwd_info, strlen(cwd_info));
		}
		else
		{

                if((strncmp(can_no_change1,client_dir,strlen(can_no_change1)-1)==0)||(strncmp(can_no_change2,client_dir,strlen(can_no_change2)-1)==0))
                {
                	snprintf(cwd_info, MSG_INFO, "550 this file can not be wrote or opened.\r\n");
                	send_client_info(client_sock, cwd_info, strlen(cwd_info));

                }
		        else if (chdir(client_dir) >= 0)
				{
						snprintf(cwd_info, MSG_INFO, "257 \"%s\" is current location.\r\n", getcwd(tmp_dir, DIR_INFO));
						send_client_info(client_sock, cwd_info, strlen(cwd_info));
				}
				else
				{
						snprintf(cwd_info, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
						perror("chdir():");
						send_client_info(client_sock, cwd_info, strlen(cwd_info));
				}
		}
}
//删除目录 用到mkdir 250
void handle_rmd(int client_sock)
{
        char    rmd_info[MSG_INFO];
        char    tmp_dir[DIR_INFO];
        char    client_dir[DIR_INFO];

        char t_dir[DIR_INFO];
        int dirlength=-1;
        int length=strlen(client_Control_Info);
        int i=0;
        for(i=4;i<length;i++)
                  format_client_Info[i-4]=client_Control_Info[i];
        format_client_Info[i-6]='\0';

        if(strncmp(getcwd(t_dir, DIR_INFO),format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-10)!=0)
         {
                getcwd(client_dir, DIR_INFO);
                dirlength=strlen(client_dir);
                client_dir[dirlength]='/';
          }


        for(i=4;i<length;i++)
         {
                 client_dir[dirlength+i-3]=client_Control_Info[i];
          }
          client_dir[dirlength+i-5]='\0';

                // printf("%s\r\n",client_dir);

          if((strncmp(can_no_change1,client_dir,strlen(can_no_change1)-1)==0)||(strncmp(can_no_change2,client_dir,strlen(can_no_change2)-1)==0))
          {
          	snprintf(rmd_info, MSG_INFO, "550 this file can not be removed.\r\n");
          	send_client_info(client_sock, rmd_info, strlen(rmd_info));

          }
          else if (rmdir(client_dir) >= 0)
         {
                printf( " \"%s\" is deleted successfully.\r\n", client_dir);
                send_client_info(client_sock, serverInfo250, strlen(serverInfo250));
         }
         else
          {
                        snprintf(rmd_info, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
                        perror("rmdir():");
                        send_client_info(client_sock, rmd_info, strlen(rmd_info));
          }
}
//创建目录用到mkdir函数 250 ，失败550
void handle_mkd(int client_sock)
{
        char    mkd_info[MSG_INFO];
        char    tmp_dir[DIR_INFO];
        char    client_dir[DIR_INFO];

        char t_dir[DIR_INFO];
        int dirlength=-1;
        int length=strlen(client_Control_Info);
        int i=0;
        for(i=4;i<length;i++)
                   format_client_Info[i-4]=client_Control_Info[i];
         format_client_Info[i-6]='\0';

         printf("%s\n",format_client_Info);
          if(strncmp(getcwd(t_dir, DIR_INFO),format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-10)!=0)
          {
        	  printf("%s\n",t_dir);
                getcwd(client_dir, DIR_INFO);
                dirlength=strlen(client_dir);
                client_dir[dirlength]='/';
            }
          printf("%s\n",client_dir);
        for(i=4;i<length;i++)
          {
                   client_dir[dirlength+i-3]=client_Control_Info[i];
           }
           client_dir[dirlength+i-5]='\0';
           printf("%s\n",client_dir);
                // printf("%s\r\n",client_dir);

       if((strncmp(can_no_change1,client_dir,strlen(can_no_change1)-1)==0)||(strncmp(can_no_change2,client_dir,strlen(can_no_change2)-1)==0))
       {
                    	snprintf(mkd_info, MSG_INFO, "550 this file can not be wrote or changed.\r\n");
                    	send_client_info(client_sock, mkd_info, strlen(mkd_info));

       }
         else if (mkdir(client_dir,0766) >= 0)
         {
                        printf( " \"%s\" is created successfully.\r\n", client_dir);
                        //send_client_info(client_sock, mkd_info, strlen(mkd_info));
                        send_client_info(client_sock, serverInfo250, strlen(serverInfo250));
          }
          else
           {
                        snprintf(mkd_info, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
                        perror("mkdir():");
                        send_client_info(client_sock, mkd_info, strlen(mkd_info));
            }
}


//用到被动链接
void handle_list(int client_sock)
{
    send_client_info(client_sock, serverInfo150, strlen(serverInfo150));

   	int t_data_sock;
	struct sockaddr_in client;
	int sin_size=sizeof(struct sockaddr_in);
	if((t_data_sock=accept(ftp_data_sock,(struct sockaddr *)&client,&sin_size))==-1)
	{
		perror("accept error");
		return;
	}

	FILE *pipe_fp;
	char t_dir[DIR_INFO];
	char list_cmd_info[DIR_INFO];
	snprintf(list_cmd_info, DIR_INFO, "ls -l %s", getcwd(t_dir, DIR_INFO));

   	if ((pipe_fp = popen(list_cmd_info, "r")) == NULL)
    {
		printf("pipe open error in cmd_list\n");
		return ;
    }
   	//printf("pipe open successfully!, cmd is %s\n", list_cmd_info);

	char t_char;
   	while ((t_char = fgetc(pipe_fp)) != EOF)
    {
		printf("%c", t_char);
		write(t_data_sock, &t_char, 1);
    }
   	pclose(pipe_fp);
   	printf("close pipe successfully!\n");
   	close(t_data_sock);
	printf("%s close data successfully!\n",serverInfo226);
	close(ftp_data_sock);
}




//被动链接（数据接口）
void handle_pasv(int client_sock,struct sockaddr_in client)
{

	char pasv_msg[MSG_INFO];
	char	port_str[8];
   	char	addr_info_str[30];
	struct sockaddr_in user_data_addr;

		int t_client_sock;
	 	t_client_sock = socket(AF_INET, SOCK_STREAM, 0);
	   	if (t_client_sock < 0)
	    {
			printf("create data socket error!\n");
			return;
	    }
	    srand((int)time(0));
	    int a=rand()%1000+1025;
	    bzero(&user_data_addr, sizeof(user_data_addr));
	    user_data_addr.sin_family = AF_INET;
	    user_data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	    user_data_addr.sin_port = htons(a);

	    if (bind(t_client_sock, (struct sockaddr*)&user_data_addr, sizeof(struct sockaddr)) < 0)
	    {
				printf("bind error in create data socket:%s\n",strerror(errno));
				return  ;
	     }
	    if( listen(t_client_sock, LISTEN_QENU)<0)
	    {
	    	printf("listen  error in create data socket:%s\n",strerror(errno));
	    	return  ;
	    }
	    ftp_data_sock=t_client_sock;


       int    tmp_port1;
       int		tmp_port2;
       tmp_port1 = a/256;
       tmp_port2 = a%256;
       snprintf(addr_info_str, sizeof(addr_info_str), "%d,%d,%d,%d,", user_data_addr.sin_addr.s_addr&0x000000FF,
		   (user_data_addr.sin_addr.s_addr&0x0000FF00)>>8,
		   (user_data_addr.sin_addr.s_addr&0x00FF0000)>>16,
		   (user_data_addr.sin_addr.s_addr&0xFF000000)>>24);

      snprintf(port_str, sizeof(port_str), "%d,%d", tmp_port1, tmp_port2);
      strcat(addr_info_str, port_str);
      snprintf(pasv_msg, MSG_INFO, "227 Entering Passive Mode (%s).\r\n", addr_info_str);
      send_client_info(client_sock, pasv_msg, strlen(pasv_msg));

}


void handle_file(int client_sock)
{
	send_client_info(client_sock, serverInfo150, strlen(serverInfo150));

    int t_data_sock;
	struct sockaddr_in client;
	int sin_size=sizeof(struct sockaddr_in);
	if((t_data_sock=accept(ftp_data_sock,(struct sockaddr *)&client,&sin_size))==-1)
	{
		perror("accept error");
		return;
	}
	int i=0;
	int length=strlen(client_Control_Info);
	for(i=5;i<length;i++)
		format_client_Info[i-5]=client_Control_Info[i];
	format_client_Info[i-7]='\0';

	FILE* fp;
	int file_fd;
	int n;
	char t_dir[DIR_INFO];
	char file_info[DIR_INFO];

	char file_mode[3];
	char file1[3]="rb";//读文件
	char file2[3]="ab";//写文件
	if(strncmp("retr", client_Control_Info, 4) == 0||(strncmp("RETR", client_Control_Info, 4) == 0))
		snprintf(file_mode, 3,file1);
   	else
   		snprintf(file_mode, 3,file2);
     file_mode[3]='\0';
 	snprintf(file_info, DIR_INFO, "%s/%s", getcwd(t_dir, DIR_INFO),format_client_Info);
	if(strncmp(getcwd(t_dir, DIR_INFO),format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-1)==0)
		fp = fopen(format_client_Info, file_mode);
	else
		fp = fopen(file_info, file_mode);

	if (fp == NULL)
	{
			printf("open file error:%s\r\n",strerror(errno));
			char 	cwd_info[MSG_INFO];
			snprintf(cwd_info, MSG_INFO, "550 %s :%s\r\n",format_client_Info,strerror(errno));
  			send_client_info(client_sock, cwd_info, strlen(cwd_info));
  			close(t_data_sock);
   		    close(ftp_data_sock);
			return;
	}
	//把文件流指针转换成文件描述
   int cmd_sock=fileno(fp);
   memset(client_Data_Info, 0, MAX_INFO);
   if(strncmp("retr", client_Control_Info, 4) == 0||(strncmp("RETR", client_Control_Info, 4) == 0))
   	{//读文件
  		 while ((n = read(cmd_sock, client_Data_Info, MAX_INFO)) > 0)
		{
	    	if (write(t_data_sock, client_Data_Info, n) != n)
	    		{
				printf("retr transfer error\n");
				return;
	  		}
		}
	}
	else
	{
		while ((n = read(t_data_sock, client_Data_Info, MAX_INFO)) > 0)
		{
	    	if (write(cmd_sock, client_Data_Info, n) != n)
	    		{
				printf("stor transfer error\n");
				return;
	  		}
		}
	}

	fclose(fp);
   close(t_data_sock);
   close(ftp_data_sock);
}



//删除文件用unlink函数
void handle_del(int client_sock)
{
      	char    del_info[MSG_INFO];
        char    tmp_file[DIR_INFO];
        char client_dir[DIR_INFO];

        char t_dir[DIR_INFO];
        int dirlength=-1;
        int length=strlen(client_Control_Info);
        int i=0;
        for(i=5;i<length;i++)
                        format_client_Info[i-5]=client_Control_Info[i];
         format_client_Info[i-7]='\0';

                if(strncmp(getcwd(t_dir, DIR_INFO),format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-10)!=0)
                {
                getcwd(client_dir, DIR_INFO);
                dirlength=strlen(client_dir);
                client_dir[dirlength]='/';
                }


        for(i=5;i<length;i++)
        {
                client_dir[dirlength+i-4]=client_Control_Info[i];
         }
        client_dir[dirlength+i-6]='\0';

        if((strncmp(can_no_change1,client_dir,strlen(can_no_change1)-1)==0)||(strncmp(can_no_change2,client_dir,strlen(can_no_change2)-1)==0))
       {
                  snprintf(del_info, MSG_INFO, "550 this file can not be wrote or changed.\r\n");
                	send_client_info(client_sock, del_info, strlen(del_info));

         }
        else if (unlink(client_dir) >= 0)
        {
              printf( " \"%s\" is deleted successfully.\r\n", client_dir);
              send_client_info(client_sock, serverInfo250, strlen(serverInfo250));
        }
         else
         {
               snprintf(del_info, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
                perror("unlink():");
               send_client_info(client_sock, del_info, strlen(del_info));
         }

}

//===========还没处理好，所以SITE处理要注意==========
void handle_site(int client_sock)
{
	char    site_info[MSG_INFO];
    char    tmp_dir[DIR_INFO];
    char    client_dir[DIR_INFO];
    int    args_dir;
	        char t_dir[DIR_INFO];
	        int dirlength=-1;
	        int length=strlen(client_Control_Info);
	        int i=0;
	        for(i=5;i<length;i++)
	                   format_client_Info[i-5]=client_Control_Info[i];
	         format_client_Info[i-7]='\0';
	         printf("format_client_Info=%s\n",format_client_Info);
	         if((strncmp("chmod", format_client_Info,5) == 0)||(strncmp("CHMOD", format_client_Info,5) == 0))
	         {
	        	 printf("format_client_Info=%s\n",format_client_Info);
	        	 site_info[0]=client_Control_Info[11];
	        	 site_info[1]=client_Control_Info[12];
	        	 site_info[2]=client_Control_Info[13];
	        	 site_info[3]='\0';
	        	 args_dir=atoi(site_info);
	        	 for(i=15;i<length;i++){
	        		 format_client_Info[i-15]=client_Control_Info[i];
	        	 }
	        	 format_client_Info[i-17]='\0';
	        	 printf("format_client_Info=%s\n", format_client_Info);

		          if(strncmp(getcwd(t_dir, DIR_INFO), format_client_Info,strlen(getcwd(t_dir, DIR_INFO))-10)!=0)
		          {
		                getcwd(client_dir, DIR_INFO);
		                dirlength=strlen(client_dir);
		                client_dir[dirlength]='/';
		           }
		          printf("client_dir=%s\n", client_dir);
		        for(i=15;i<length;i++)
		          {
		                   client_dir[dirlength+i-14]=client_Control_Info[i];
		           }
		           client_dir[dirlength+i-16]='\0';
		           printf("client_dir=%s\n", client_dir);

		           if( (strncmp("777", site_info,3) == 0)&&(chmod(client_dir,0777) >= 0))
		           {
		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		           else if((strncmp("766", site_info,3) == 0)&&(chmod(client_dir,0766) >= 0))
		           {
		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		           else if((strncmp("761", site_info,3) == 0)&&(chmod(client_dir,0761) >= 0))
		            {
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		           else if((strncmp("751", site_info,3) == 0)&&(chmod(client_dir,0751) >= 0))
		           	{
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		            }
		           else if((strncmp("752", site_info,3) == 0)&&(chmod(client_dir,0752) >= 0))
		           {
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		            }
		           else if((strncmp("754", site_info,3) == 0)&&(chmod(client_dir,0754) >= 0))
		           {
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		           else if((strncmp("764", site_info,3) == 0)&&(chmod(client_dir,0764) >= 0))
		           {
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		             }
		           else if((strncmp("750", site_info,3) == 0)&&(chmod(client_dir,0750) >= 0))
		           		           {
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           		           }
		           else if((strncmp("742", site_info,3) == 0)&&(chmod(client_dir,0742) >= 0))
		           {
		          		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		           else if((strncmp("740", site_info,3) == 0)&&(chmod(client_dir,0740) >= 0))
		           {
		          		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		          	}
		           else if((strncmp("741", site_info,3) == 0)&&(chmod(client_dir,0741) >= 0))
		           {
		          		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		          	}
		           else if((strncmp("666", site_info,3) == 0)&&(chmod(client_dir,0666) >= 0))
		           	{
		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	}
		           else if((strncmp("661", site_info,3) == 0)&&(chmod(client_dir,0661) >= 0))
		            {
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	}
		           else if((strncmp("651", site_info,3) == 0)&&(chmod(client_dir,0651) >= 0))
		           {
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	   }
		           else if((strncmp("652", site_info,3) == 0)&&(chmod(client_dir,0652) >= 0))
		           	{
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	}
		           	else if((strncmp("654", site_info,3) == 0)&&(chmod(client_dir,0654) >= 0))
		           {
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	}
		           	else if((strncmp("664", site_info,3) == 0)&&(chmod(client_dir,0664) >= 0))
		           	{
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	}
		           	else if((strncmp("650", site_info,3) == 0)&&(chmod(client_dir,0650) >= 0))
		           	{
		           		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           }
		            else if((strncmp("642", site_info,3) == 0)&&(chmod(client_dir,0642) >= 0))
		            {
		           		          		           		        	   send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		           	 }
		        else if((strncmp("640", site_info,3) == 0)&&(chmod(client_dir,0640) >= 0))
		        {
		        	  send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		        }
		          else if((strncmp("641", site_info,3) == 0)&&(chmod(client_dir,0641) >= 0))
		         {
		           		    send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
		          }
		           else
		           {
		        	   snprintf(tmp_dir, MSG_INFO, "550 %s :%s\r\n",client_dir,strerror(errno));
		        	    perror("chmod():");
		        	    send_client_info(client_sock, tmp_dir, strlen(tmp_dir));
		           }

	         }
	         else
	         {
	        	 send_client_info(client_sock, serverInfo213, strlen(serverInfo213));
	         }
	                // printf("%s\r\n",client_dir);



}



//发送客户端信息，以及所需要的信息。
void send_client_info(int client_sock,char* info,int length)
{
   int len;
	if((len = send(client_sock, info, length,0))<0)
	{
			perror("send info error ");
			return;
	}
}
int recv_client_info(int client_sock)
{
	//printf("接到具体信息\n");
   	int num;
	if((num=recv(client_sock,client_Control_Info,MAX_INFO,0))<0)
	{
			perror("receive info error ");
			return 0;
	}
	client_Control_Info[num]='\0';
	printf("Client %d Message:%s\n",pthread_self(),client_Control_Info);
	if(strncmp("USER", client_Control_Info, 4) == 0||strncmp("user", client_Control_Info, 4) == 0)
		return 2;
	return 1;
 }

void read_ROOTPATH()
{
	FILE *fp;
	char tmp_dir[MAX_INFO];
	fp=fopen("source/Path.txt","r");
	if(fp==NULL){
		printf("打开文件错误\n");
		exit(0);
	}
	if(fgets(tmp_dir,MAX_INFO, fp)!=NULL){
		printf("tmp_dir=%s\n",tmp_dir);
	}
	int i;
	for(i=5;i<strlen(tmp_dir);i++)
	{
		ROOTPATH[i-5]=tmp_dir[i];
	}
	ROOTPATH[i-6]='\0';
	printf("ROOTPATH====%s\n", ROOTPATH);
	fclose(fp);
}
//void read_USER()
//{
//	FILE *fp;
//	char tmp_dir[MAX_INFO];
//	char real_dir[MAX_INFO];
//	fp=fopen("source/user.txt","r");
//	if(fp==NULL){
//		printf("打开文件错误\n");
//		exit(0);
//	}
//	int i = 1;
//	printf("%d",strlen(login_user));
//	printf("%s",login_user);
//	printf("%s",login_pass);
//	while(fgets(tmp_dir,MAX_INFO, fp)!=NULL){
//		//printf("%d",strlen(login_user));
//		for(i=5;i<strlen(tmp_dir);i++)
//		{
//			real_dir[i-5]=tmp_dir[i];
//		}
//		real_dir[i-6]='\0';
//		printf("tmp_dir=%s\n",real_dir);
//
//		if(strncmp(login_user, real_dir, strlen(login_user)) == 0)
//		{
//
//			printf("same=%s\n",real_dir);
//		}
//
//	}
//	fclose(fp);
//}
int read_system_user()
{
   	struct spwd *sp;
	sp= getspnam(login_user);
	  if(sp == NULL)
	  {
	          printf("set sp error\n");
	 }

    if((strcmp(sp->sp_pwdp, (char*)crypt(login_pass, sp->sp_pwdp)))==0)
    {
    	printf("correct\n");
    	return 1;
    }
    else
    {
         printf("user or password error\n");
         return 0;
    }


}

