﻿
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

   const char DefaultDevName[]="eth0";
   char *username;
   char *password;
   char *devicename;


//检测进程
int checkprocess();

void print_help();
void getUserName();
void getPassword();
void getDevice();
//主函数
int main(int argc,char *argv[])
{
   int c=0,i,j;
   int opt;
   opterr = 0;
   for(i=0;i<argc;i++)
     {
      for(j=0;j<strlen(argv[i]);j++)
        if(argv[i][j]=='-'&&strlen(argv[i])!=1)
          c++;
     }
   if(c<argc/2)
     {
      printf("命令行输入错误！\n请尝试执行“h3c_ouc --help”来获取更多信息。\n");
      exit(1);
     }
   struct option long_options[]={
       	{"help",0,NULL,'h'},
	{"username",0,NULL,'u'},
	{"password",0,NULL,'p'},
	{"device",0,NULL,'n'},
	{"logoff",0,NULL,'l'},       
	{NULL,0,NULL,0},
    };
    static const char *options="u::p::n::l::h";
    if(argc==1)
	{
	      print_help();
              exit(1);
	}
   
    while((opt=getopt_long(argc,argv,options,long_options,NULL))!=-1)
    {
	switch(opt)
       {
             case 'u':
             if(checkprocess()==-1)
             {
              fprintf(stderr, "%s","用户已经登录！\n");
              exit(2);
             }
              if(argv[optind]==NULL)
		  {
                  getUserName();
                  getPassword();
                  getDevice();
                  }
	      else
		  {
		  if(argv[optind+1]==NULL)
                    {
                     username=(char *)malloc(100);
                     strcpy(username,argv[optind]);
                     getPassword(); 
                     getDevice();
                    }
                  else
                   {
                   username=(char *)malloc(100);
                   strcpy(username,argv[optind]);
                   }
		  }
	      break;
              
             case 'p':
                if(checkprocess()==-1)
                {
                fprintf(stderr, "%s\n","用户已经登录！\n");
                exit(2);
                }
                if(username==NULL)
                    {
                    getUserName();
                    getPassword();
                    getDevice();
                    }
                else if(argv[optind]==NULL)
		  {
                    getPassword();
                    getDevice();
                  }
                else if(argv[optind+1]==NULL)
                   {
                   password=(char *)malloc(100);
                   strcpy(password,argv[optind]);
                   getDevice();
                   }
                else
                  {
                   password=(char *)malloc(100);
                   strcpy(password,argv[optind]);
                  }
              break;
	      
             case 'n':
               if(checkprocess()==-1)
                {
                fprintf(stderr, "%s\n","用户已经登录！\n");
                exit(2);
                }
               if(username==NULL)
                    {
                    getUserName();
                    getPassword();
                    getDevice();
                    }
               else if(password==NULL)
                    {
                    getPassword();
                    getDevice();
                    }
               else if(argv[optind]==NULL)
                    getDevice();
               else
                 {  
                  devicename=(char *)malloc(100); 
                  strcpy(devicename,argv[optind]);
                 }
              break;
             case 'h':
              print_help();
	      exit(0);
              break;
             case 'l':
              if(argv[optind]==NULL)
		  getDevice();
              else
              {
                 devicename=(char *)malloc(100); 
                 strcpy(devicename,argv[optind]);
              }
              SendLogoffPkt(devicename);
              exit(0);
              break;
	     case '?':
              printf("未识别的选项!\n请尝试执行“h3c_ouc --help”来获取更多信息。\n");
              exit(1);
              break;
          }
        }
       
    if((strlen(username)!=0)&&(strlen(password)!=0)&&(strlen(devicename)!=0))
        {
		Authentication(username,password,devicename);
        } 
    else
             fprintf(stderr,"%s\n","用户名、密码和网卡名称不能为空！");

    exit(1);
    
}
void print_help()
	{
		printf("用法: h3c_ouc [选项] 参数\n");
		printf("选项:\n");
		printf("\t-u\t--username\t\t参数为用户名\n"); 
 	        printf("\t-p\t--password\t\t参数为密码\n");
                printf("\t-n\t--device\t\t参数为网卡名，默认为'eth0'\n");
		printf("\t-h\t--help\t\t\t使用方法\n");
     	        printf("\t-l\t--logoff\t\t注销\n");
                printf("举例:\n");
                printf("\th3c_ouc -u abc -p 1234 -n eth0\n");
                printf("\t也可以直接使用 h3c_ouc -u 按照提示输入。\n");
	}
void getUserName()
{
     char temp[100];
     username=(char *)malloc(100);
     GetUserName:
     printf("请输入用户名：");
     setbuf(stdin,NULL);                           //清除缓冲区(Linux),Windows下可以使用fflush或者rewind。
     fgets(temp,sizeof(char)*100,stdin);
     if(strlen(temp)==0||strlen(temp)==1&&temp[0]=='\n')
        {
         printf("用户名不能为空！\n");
         goto GetUserName;
        }
     else
         memcpy(username,temp,strlen(temp)-1);
}
void getPassword()
{
     char temp[100];
     password=(char *)malloc(100);
     GetPassword:
     printf("请输入密码：");
     setbuf(stdin,NULL);                           //清除缓冲区(Linux),Windows下可以使用fflush或者rewind。
     fgets(temp,sizeof(char)*100,stdin);
     if(strlen(temp)==0||strlen(temp)==1&&temp[0]=='\n')
         {
         printf("密码不能为空！\n");
         goto GetPassword;
         }
     else
         memcpy(password,temp,strlen(temp)-1);
}

void getDevice()
{
     char *temp;  
     temp=(char *)malloc(100);
     devicename=(char *)malloc(100);
     GetDeviceName:      
     printf("请输入网卡名称（默认为eth0）：");
     setbuf(stdin,NULL);                           //清除缓冲区(Linux),Windows下可以使用fflush或者rewind。
     fgets(temp,sizeof(char)*100,stdin);
     if(strlen(temp)==0||strlen(temp)==1&&temp[0]=='\n')
        {
         strcpy(devicename,DefaultDevName);
        }
     else
         memcpy(devicename,temp,strlen(temp)-1);
}

int checkprocess()
{
    char command[]="ps -e|grep -w h3c_ouc";
    FILE    *read_fp;
    int process_read;
    int   count=0;
    char   ch;
    read_fp=popen(command,"r");
    if(read_fp!=NULL)
    {
        while(fgetc(read_fp)!=EOF)
           {
            fseek(read_fp,sizeof(char),1);
            ch=fgetc(read_fp);
            if(ch=='\n')
              count++;
           }
        if(count>1)
           return -1;
        else
           return 1;
        pclose(read_fp);
    }
    else
    {
        printf("Shell command error!\n");
        exit(1);
    }
}

            