/*
 * ftp.h
 *
 *  Created on: 2018年1月8日
 *      Author: ginger
 */

#ifndef FTP_H_
#define FTP_H_

#define COMMAND_NUM 7
#define FILE_TRANS_MODE_ASIC 0
#define FILE_TRANS_MODE_BIN 1

char serverInfo220[]="220 Welcome to FTP service.\r\n";
char serverInfo250[]="250 File operation is successful.\r\n";
char serverInfo230[]="230 Login successful.\r\n";
char serverInfo331[]="331 User name okay, need password.\r\n";
char serverInfo221[]="221 The Service is closing your network\r\n";
char serverInfo150[]="150 File status okay; about to open data connection.\r\n";
char serverInfo226[]="226 Closing data connection.\r\n";
char serverInfo200[]="200 Command okay.\r\n";
char serverInfo215[]="215 Ubuntu 14.0.2\r\n";
char serverInfo213[]="213 File status.\r\n";
char serverInfo211[]="211 System status, or system help reply.\r\n";
char serverInfo350[]="350 Requested file action pending further information.\r\n";
char serverInfo530[]="530  无法登录.\r\n";
char serverInfo531[]="531 非root用户登录，匿名登录.\r\n";

char serverInfo[]="202 Command not implemented, superfluous at this site.\r\n";

/*
 *   sockaddr_in（在netinet/in.h中定义）：
 *   struct sockaddr_in {
 *   short int sin_family;                     协议族
 *   unsigned short int sin_port;         存储端口号
 *   struct in_addr sin_addr;               存储IP地址
 *   unsigned char sin_zero[8];           让sockaddr与sockaddr_in两个数据结构保持大小相同而保留的空字节。
 *   };
 *
 *  struct   termios {     tcflag_t这个类型应该就是unsigned short，cc_t是  unsigned char
 *  tcflag_t c_iflag; 输入模式
 *      tcflag_t c_oflag;           输入模式
 *      tcflag_t c_cflag;           输出模式
 *      tcflag_t c_lflag;            控制模式
 *      cc_t           c_cc[NCCS]; 特殊控制模式
 *  };
 *
 *
 *
 *
 */



#endif /* FTP_H_ */
