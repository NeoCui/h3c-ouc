
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

//检测网络链路
int GetNetState();
//检测进程
int checkprocess();

void print_help();
void getuname();
void getpwd();
void getdev();
//主函数
int main(int argc,char *argv[])
{
   int c=0,i,j;
   int opt;
   opterr = 0;
   for(i=0;i<argc;i++)
     {
      for(j=0;j<strlen(argv[i]);j++)
        if(argv[i][j]=='-')
          c++;
     }
   if(c<argc/2)
     {
      printf("参数前面必须有选项！\n请尝试执行“h3c_ouc --help”来获取更多信息。\n");
      exit(1);
     }
   struct option long_options[]={
       	{"help",0,NULL,'h'},
	{"username",1,NULL,'u'},
	{"password",1,NULL,'p'},
	{"device",1,NULL,'n'},
	{"logoff",1,NULL,'l'},       
	{NULL,0,NULL,0},
    };
    if(checkprocess()==-1)
       {
       printf("A process is already running!\n");
       exit(1);
       }
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
              if(argv[optind]==NULL)
		  {
                  getuname();
                  getpwd();
                  getdev();
                  }
	      else
		  {
		  if(argv[optind+1]==NULL)
                    {
                     username=(char *)malloc(100);
                     strcpy(username,argv[optind]);
                     getpwd(); getdev();
                    }
                  else
                   {
                   username=(char *)malloc(100);
                   strcpy(username,argv[optind]);
                   }
		  }
	      break;
              
             case 'p':
                if(username==NULL)
                    {
                    getuname();
                    getpwd();
                    getdev();
                    }
                else if(argv[optind]==NULL)
		  {
                    getpwd();
                    getdev();
                  }
                else if(argv[optind+1]==NULL)
                   {
                   password=(char *)malloc(100);
                   strcpy(password,argv[optind]);
                   getdev();
                   }
                else
                  {
                   password=(char *)malloc(100);
                   strcpy(password,argv[optind]);
                  }
              break;
	      
             case 'n':
               if(username==NULL)
                    {
                    getuname();
                    getpwd();
                    getdev();
                    }
               else if(password==NULL)
                    {
                    getpwd();
                    getdev();
                    }
               else if(argv[optind]==NULL)
                    getdev();
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
		  getdev();
              else
              {
                 devicename=(char *)malloc(100); 
                 strcpy(devicename,argv[optind]);
              }
              printf("DeviceName:%s.\n",devicename);
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
                exit(0);

        } 
    else
             printf("UserName & Password & DeviceName can not be empty!\n");

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
	}
void getuname()
{
     username=(char *)malloc(100);
     GetUname:
     printf("Please Input UserName:");
     gets(username);
     if(strlen(username)==0)
        {
         printf("UserName can't be empty!\n");
         goto GetUname;
        }
     if(strlen(username)>100)
        {
         printf("UserName is too long");
         goto GetUname;
        }
}
void getpwd()
{
     GetPwd:
     printf("Please Input Password:");
     password=(char *)malloc(100);
     gets(password);
     if(strlen(password)==0)
         {
         printf("Password can't be empty!\n");
         goto GetPwd;
         }
      if(strlen(password)>100)
        {
         printf("UserName is too long");
         goto GetPwd;
        }
}

void getdev()
{
  char *temp;  
     GetDevicename:      
     temp=(char *)malloc(100);
     devicename=(char *)malloc(100);
     printf("Please Input DeviceName(default for eth0):");
     gets(temp);
     if(strlen(temp)==0)
        strcpy(devicename,DefaultDevName);
     else if(strlen(temp)!=4)
        {
         printf("DeviceName Error!\n");
         goto GetDevicename;
        }
     else
        {
         strcpy(devicename,temp);
        }
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

            
