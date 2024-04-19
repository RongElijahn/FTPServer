/*
 * ftp_server.h
 *
 *  Created on: 2018年1月27日
 *      Author: root
 */

#ifndef SOURCE_FTP_SERVER_H_
#define SOURCE_FTP_SERVER_H_



#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <pthread.h>
#include <crypt.h>
#include<shadow.h>
#include <errno.h>
#include "ftp.h"


#define FTP_SERVER_PORT 8888  //FTP控制端口
#define MAX_INFO 1024
#define DIR_INFO 100
#define MSG_INFO 100
#define LISTEN_QENU 5

char login_user[MSG_INFO];
char login_pass[MSG_INFO];
const char can_no_change1[]="/home/ginger/eclipse-workspace/FTPServer/source";
const char can_no_change2[]="/home/ginger/eclipse-workspace/FTPServer/Debug";
const char	default_user[] = "root";    //用户
const char	default_pass[] = "zr1996root";  //密码
const char  anony_user[]="anonymous";   //游客登录的用户
const char  anony_pass[]="NcFTP@";   //游客登录的密码
char ROOTPATH[MAX_INFO];
int  ftp_server_sock;  //服务器描述符
int  ftp_data_sock;
char client_Control_Info[MAX_INFO];  //客户端传过来的命令
char client_Data_Info[MAX_INFO];
char format_client_Info[MAX_INFO];
int translate_data_mode=FILE_TRANS_MODE_ASIC;

void *Handle_Client_Request(void* arg);
struct ARG{
	int client_sock;
	struct sockaddr_in client;
};

int read_system_user();
void read_ROOTPATH();
void do_client_work(int client_sock,struct sockaddr_in client);
int login(int client_sock);
void handle_site(int client_sock);
void handle_cwd(int client_sock);
void handle_list(int client_sock);
void handle_pasv(int client_sock,struct sockaddr_in client);
void handle_file(int client_sock);
void handle_del(int client_sock);
void handle_mkd(int client_sock);
void handle_rmd(int client_sock);

//struct sockaddr_in create_date_sock();
void send_client_info(int client_sock,char* info,int length);
int recv_client_info(int client_sock);


#endif /* SOURCE_FTP_SERVER_H_ */
