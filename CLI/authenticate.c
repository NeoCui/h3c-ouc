﻿#include"authenticate.h"
/**
 * 函数：Authentication()
 *
 * 使用以太网进行802.1X认证(802.1X Authentication)
 * 该函数将不断循环，应答802.1X认证会话，直到遇到错误后才退出
 */
//常量

//认证主函数
int Authentication(char *UserName,char *Password,char *DeviceName)
{
    char	errbuf[PCAP_ERRBUF_SIZE];
    pcap_t	*adhandle; // adapter handle
    uint8_t	MAC[6];
    char	FilterStr[100];
    struct bpf_program	fcode;
    const int DefaultTimeout=1000;//设置接收超时参数，单位ms

     //检查网线是否已插好,网线插口可能接触不良
    if(GetNetState(DeviceName)==-1)
       {
       fprintf(stderr, "%s\n", "网卡异常！请检查网卡名称是否正确，网线是否插好！");
       exit(1);
       }
    /* 打开适配器(网卡) */
    adhandle = pcap_open_live(DeviceName,65536,1,DefaultTimeout,errbuf);
    if (adhandle==NULL) {
        fprintf(stderr, "%s\n", errbuf);
        exit(1);
    }
   
     
    /* 查询本机MAC地址 */
    GetMacFromDevice(MAC, DeviceName);

        /*
         * 设置过滤器：
         * 初始情况下只捕获发往本机的802.1X认证会话，不接收多播信息（避免误捕获其他客户端发出的多播信息）
         * 进入循环体前可以重设过滤器，那时再开始接收多播信息
         */
    sprintf(FilterStr, "(ether proto 0x888e) and (ether dst host %02x:%02x:%02x:%02x:%02x:%02x)",
            MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5]);
    pcap_compile(adhandle, &fcode, FilterStr, 1, 0xff);
    pcap_setfilter(adhandle, &fcode);

    //开始认证
    START_AUTHENTICATION:
    {
        int retcode;
        struct pcap_pkthdr *header;
        const uint8_t	*captured;
        uint8_t	ethhdr[14]={0}; // ethernet header
        uint8_t	ip[4]={0};	// ip address
        int flag=0;
        /* 主动发起认证会话 */
        SendStartPkt(adhandle, MAC);

        /* 等待认证服务器的回应 */
        bool serverIsFound = false;
        while (!serverIsFound)
        {
            retcode = pcap_next_ex(adhandle, &header, &captured);
            if (retcode==1 && (EAP_Code)captured[18]==REQUEST)
                serverIsFound = true;
            else
            {	// 延时后重试
		if(flag>3)
                   {
                   fprintf(stderr, "%s\n", "服务器未响应。");
                   exit(1);
                   }
                fprintf(stderr, "%s\n", "等待服务器响应......");
                sleep(1); 
                flag++;
                SendStartPkt(adhandle, MAC);
                if(GetNetState(DeviceName)==-1)
                 {
                  fprintf(stderr, "%s\n", "网卡异常！请检查网卡名称是否正确，网线是否插好！");
                  exit(1);
                 }
                // 检查网线是否接触不良或已被拔下
            }
        }

        // 填写应答包的报头(以后无须再修改)
        // 默认以单播方式应答802.1X认证设备发来的Request
        memcpy(DstMAC+0, captured+6,6); //拷贝交换机MAC
        memcpy(ethhdr+0, captured+6, 6);//拷贝交换机MAC至发包前6位
        memcpy(ethhdr+6, MAC, 6);//接下来6位为本机MAC
        ethhdr[12] = 0x88;
        ethhdr[13] = 0x8e;

        // 回答Response Identity
        if ((EAP_Type)captured[22] == IDENTITY)
        {	
            ip[0]=0x00;
            ip[1]=0x00;
            ip[2]=0x00;
            ip[3]=0x00;
            SendResponseIdentity(adhandle, captured, ethhdr, ip, UserName);
        }
        else
            goto START_AUTHENTICATION;
        // 重设过滤器，只捕获华为802.1X认证设备发来的包（包括多播Request Identity / Request AVAILABLE）
        sprintf(FilterStr, "(ether proto 0x888e) and (ether src host %02x:%02x:%02x:%02x:%02x:%02x)",
                captured[6],captured[7],captured[8],captured[9],captured[10],captured[11]);
        pcap_compile(adhandle, &fcode, FilterStr, 1, 0xff);
        pcap_setfilter(adhandle, &fcode);

        // 进入循环体
        LOOP:
       {
       if(GetNetState(DeviceName)==-1)
       {
       fprintf(stderr, "%s\n", "网卡异常！请检查网卡名称是否正确，网线是否插好！");
       exit(1);
       }
        for (;;)
        {
            // 调用pcap_next_ex()函数捕获数据包
            while (pcap_next_ex(adhandle, &header, &captured) != 1)
            {
                sleep(1);     // 直到成功捕获到一个数据包后再跳出
            if(GetNetState(DeviceName)==-1)
             {
              fprintf(stderr, "%s\n", "网卡异常！请检查网卡名称是否正确，网线是否插好！");
              exit(1);
             }
                // 检查网线是否已被拔下或插口接触不良
            }

            // 根据收到的Request，回复相应的Response包
            if ((EAP_Code)captured[18] == REQUEST)
            {
                switch ((EAP_Type)captured[22])
                {
                case IDENTITY:
                    GetIpFromDevice(ip, DeviceName);
                    SendResponseIdentity(adhandle, captured, ethhdr, ip, UserName);
                    break;
                case MD5:
                    SendResponseMD5(adhandle, captured, ethhdr, UserName, Password);
                    break;
                default:
                    printf("[%d] Server: Request (type:%d)!\n", (EAP_ID)captured[19], (EAP_Type)captured[22]);
                    printf("Error! Unexpected request type\n");
                    exit(-1);
                    break;
                }
            }
            else if ((EAP_Code)captured[18] == FAILURE)
            {	// 处理认证失败信息
                uint8_t errtype = captured[22];
                uint8_t msgsize = captured[23];
                const char *msg = (const char*) &captured[24];
                 if(errtype==0x08)      
                   {
                    exit(0);
                   }
                 else
                 {
                  printf("[%d] Server: 认证失败。\n", (EAP_ID)captured[19]);
                  if (errtype==0x09 && msgsize>0)
                   {	// 输出错误提示消息
                    fprintf(stderr, "%s\n", msg);
                    // 已知的几种错误如下
                    // E2531:用户名不存在
                    // E2535:Service is paused
                    // E2542:该用户帐号已经在别处登录
                    // E2547:接入时段限制
                    // E2553:密码错误
                    // E2602:认证会话不存在
                    // E3137:客户端版本号无效
                    exit(1);
                   }
                
                  else
                   {
                    printf("errtype=0x%02x\n", errtype);
                    exit(1);
                   }
              }
            }
            //认证成功
            else if ((EAP_Code)captured[18] == SUCCESS)
            {
		 char cmd[30];
		 strcpy(cmd,"dhclient ");
		 strcat(cmd,DeviceName);
		 system(cmd);
                 //建立子进程，后台运行循环体
                 pid_t pid;
                 pid=fork();
                   if(pid<0)
                      exit(1); 
                  else if(pid==0)
                     {
                        goto LOOP;  
                     }
                  else
		    {
			printf("认证成功。\n");
		     //fprintf(stderr,"%s\n","success");
                        exit(0);    
		    }
            }
            else
            {
                printf("[%d] Server: (H3C data)\n", captured[19]);
                // TODO: 这里没有处理华为自定义数据包
            }
        }
       }
    }
    return 0;
}

//获取设备的MAC地址
static
void GetMacFromDevice(uint8_t mac[6], const char *devicename)
{
    int	sock;
    struct ifreq ifreq;
    sock=socket(AF_INET,SOCK_STREAM,0);
    strcpy(ifreq.ifr_name,devicename);
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq)==0)
    {
    mac[0]=(uint8_t)ifreq.ifr_hwaddr.sa_data[0],
    mac[1]=(uint8_t)ifreq.ifr_hwaddr.sa_data[1],
    mac[2]=(uint8_t)ifreq.ifr_hwaddr.sa_data[2],
    mac[3]=(uint8_t)ifreq.ifr_hwaddr.sa_data[3],
    mac[4]=(uint8_t)ifreq.ifr_hwaddr.sa_data[4],
    mac[5]=(uint8_t)ifreq.ifr_hwaddr.sa_data[5]; 
    }
    else
    {
     printf("获取MAC地址失败！\n");
     exit(1);
    }  
    close(sock);
}
//发送EAP-START开始认证包
static
void SendStartPkt(pcap_t *handle, const uint8_t localmac[])
{
    uint8_t packet[18];

    memcpy(packet, MultcastAddr, 6);
    memcpy(packet+6, localmac,   6);
    packet[12] = 0x88;
    packet[13] = 0x8e;

    // EAPOL (4 Bytes)
    packet[14] = 0x01;	// Version=1
    packet[15] = 0x01;	// Type=Start
    packet[16] = packet[17] =0x00;// Length=0x0000

    // 为了兼容不同院校的网络配置，这里发送两遍Start包
    // 1、广播发送Strat包
    pcap_sendpacket(handle, packet, sizeof(packet));
    // 2、多播发送Strat包
    memcpy(packet, MultcastAddr, 6);
}



//发送用户名
static
void SendResponseIdentity(pcap_t *adhandle, const uint8_t request[], const uint8_t ethhdr[], const uint8_t ip[4], const char username[])
{

    uint8_t	response[128];
    size_t i;
    uint16_t eaplen;
    int usernamelen;

    assert((EAP_Code)request[18] == REQUEST);
    assert((EAP_Type)request[22] == IDENTITY);

    // Fill Ethernet header
    memcpy(response, ethhdr, 14);

    // 802,1X Authentication
    // {
    response[14] = 0x1;	// 802.1X Version 1
    response[15] = 0x0;	// Type=0 (EAP Packet)
    //response[16~17]留空	// Length

    // Extensible Authentication Protocol

    response[18] = (EAP_Code) RESPONSE;	// Code
    response[19] = request[19];		// ID
    //response[20~21]留空			// Length
    response[22] = (EAP_Type) IDENTITY;	// Type
    // Type-Data
    i = 23;
    response[i++] = 0x15;	  // 上传IP地址
    response[i++] = 0x04;	  //
    memcpy(response+i, ip, 4);//
    i += 4;			  
    
    usernamelen = strlen(username); //末尾添加用户名
    memcpy(response+i, username, usernamelen);
    i += usernamelen;
    assert(i <= sizeof(response));


    // 补填前面留空的两处Length
    eaplen = htons(i-18);
    memcpy(response+16, &eaplen, sizeof(eaplen));
    memcpy(response+20, &eaplen, sizeof(eaplen));

    for(;i<=59;i++)
       response[i]=0x00;

    // 发送
    pcap_sendpacket(adhandle, response, i);
    return;
}

//发送加密后的密码
static
void SendResponseMD5(pcap_t *handle, const uint8_t request[], const uint8_t ethhdr[], const char username[], const char passwd[])
{
    uint16_t eaplen;
    size_t   usernamelen;
    size_t   packetlen;
    uint8_t  response[128];
    int i;

    assert((EAP_Code)request[18] == REQUEST);
    assert((EAP_Type)request[22] == MD5);

    usernamelen = strlen(username);
    eaplen = htons(22+usernamelen);
    packetlen = 14+4+22+usernamelen; // ethhdr+EAPOL+EAP+usernamelen

    // Fill Ethernet header
    memcpy(response, ethhdr, 14);

    // 802,1X Authentication
    // {
    response[14] = 0x1;	// 802.1X Version 1
    response[15] = 0x0;	// Type=0 (EAP Packet)
    memcpy(response+16, &eaplen, sizeof(eaplen));	// Length

    // Extensible Authentication Protocol
    // {
    response[18] = (EAP_Code) RESPONSE;// Code
    response[19] = request[19];	// ID
    response[20] = response[16];	// Length
    response[21] = response[17];	//
    response[22] = (EAP_Type) MD5;	// Type
    response[23] = 16;		// Value-Size: 16 Bytes
    FillMD5Area(response+24, request[19], passwd, request+24);
    memcpy(response+40, username, usernamelen);
         i=40+usernamelen;
    for(;i<=59;i++)
       response[i]=0x00;
    // }
    // }

    pcap_sendpacket(handle, response, packetlen);
}

//注销
void SendLogoffPkt(char *DeviceName)
{
    uint8_t packet[18];
    pcap_t	*adhandle; // adapter handle
    const int DefaultTimeout=60000;//设置接收超时参数，单位ms
    char	errbuf[PCAP_ERRBUF_SIZE];
    uint8_t	MAC[6];
    /* 打开适配器(网卡) */
    adhandle = pcap_open_live(DeviceName,65536,1,DefaultTimeout,errbuf);
    if (adhandle==NULL) {
        fprintf(stderr, "%s\n", errbuf);
        exit(1);
    }
    
    GetMacFromDevice(MAC, DeviceName);
    // Ethernet Header (14 Bytes)
    memcpy(packet, MultcastAddr, 6);
    memcpy(packet+6, MAC,   6);
    packet[12] = 0x88;
    packet[13] = 0x8e;

    // EAPOL (4 Bytes)
    packet[14] = 0x01;	// Version=1
    packet[15] = 0x02;	// Type=Logoff
    packet[16] = packet[17] =0x00;// Length=0x0000

    // 发包
    printf("注销成功。\n");
    pcap_sendpacket(adhandle, packet, sizeof(packet));
    exit(0);
}


// 函数: XOR(data[], datalen, key[], keylen)
//
// 使用密钥key[]对数据data[]进行异或加密
//（注：该函数也可反向用于解密）
static
void XOR(uint8_t data[], unsigned dlen, const char key[], unsigned klen)
{
    unsigned int	i,j;

    // 先按正序处理一遍
    for (i=0; i<dlen; i++)
        data[i] ^= key[i%klen];
    // 再按倒序处理第二遍
    for (i=dlen-1,j=0;  j<dlen;  i--,j++)
        data[i] ^= key[j%klen];
}


static
void FillClientVersionArea(uint8_t area[20])
{
    uint32_t random;
    char	 RandomKey[8+1];

    random = (uint32_t) time(NULL);    // 注：可以选任意32位整数
    sprintf(RandomKey, "%08x", random);// 生成RandomKey[]字符串

    // 第一轮异或运算，以RandomKey为密钥加密16字节
    memcpy(area, H3C_VERSION, sizeof(H3C_VERSION));
    XOR(area, 16, RandomKey, strlen(RandomKey));

    // 此16字节加上4字节的random，组成总计20字节
    random = htonl(random); // （需调整为网络字节序）
    memcpy(area+16, &random, 4);

    // 第二轮异或运算，以H3C_KEY为密钥加密前面生成的20字节
    XOR(area, 20, H3C_KEY, strlen(H3C_KEY));
}

void FillMD5Area(uint8_t digest[], uint8_t id, const char passwd[], const uint8_t srcMD5[])
{
    uint8_t	msgbuf[128]; //msgbuf = ‘id‘ + ‘passwd’ + ‘srcMD5’
     //由于Mac里没有gcrypto加密库，我们使用Qt的加密库
        size_t	msglen;
        size_t	passlen;
        passlen = strlen(passwd);
        msglen = 1 + passlen + 16;
        assert(sizeof(msgbuf) >= msglen);
        msgbuf[0] = id;
        memcpy(msgbuf+1, passwd, passlen);
        memcpy(msgbuf+1+passlen, srcMD5, 16);
    // Calls libgcrypt function for MD5 generation
       gcry_md_hash_buffer(GCRY_MD_MD5, digest, msgbuf, msglen);


}
//从MAC地址获取IP
void GetIpFromDevice(uint8_t ip[4], const char DeviceName[])
{
    int fd;
    struct ifreq ifr;

    assert(strlen(DeviceName) <= IFNAMSIZ);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd>0);

    strncpy(ifr.ifr_name, DeviceName, IFNAMSIZ);
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        struct sockaddr_in *p = (void*) &(ifr.ifr_addr);
        memcpy(ip, &(p->sin_addr), 4);
    }
    else
    {
        // 查询不到IP时默认填零处理
        memset(ip, 0x00, 4);
    }

    close(fd);
    return;
}

//获取网络状态：网线是否插好
int GetNetState(char *devicename)
{
    char    buffer[BUFSIZ];
    FILE    *read_fp;
    int     chars_read;
    int     ret;
    char    command[100];
    strcpy(command,"sudo ifconfig ");
    strcat(command,devicename);
    strcat(command," |grep RUNNING");
    memset( buffer, 0, BUFSIZ );
    
    read_fp = popen(command , "r");
    if ( read_fp != NULL )
    {
        chars_read = fread(buffer, sizeof(char), BUFSIZ-1, read_fp);
        if (chars_read > 0)
        {
            ret = 1;
        }
        else
        {
            ret = -1;
        }
        pclose(read_fp);
    }
    else
    {
        ret = -1;
    }

    return ret;
}

