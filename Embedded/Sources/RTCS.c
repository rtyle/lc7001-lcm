/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
* Copyright (c) 2004-2008 Embedded Access Inc.;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: RTCS.c$
* $Version : 3.8.40.0$
* $Date    : Jun-14-2012$
*
* Comments:
*
*   Example of HVAC using RTCS.
*
*END************************************************************************/

#include "includes.h"
//#include "httpd_types.h"
#include "tfs.h"

#if DEMOCFG_USE_WIFI
#include "string.h"
#endif

#if DEMOCFG_ENABLE_RTCS

#include  <ipcfg.h>

//#if (DEMOCFG_ENABLE_FTP_SERVER + DEMOCFG_ENABLE_TELNET_SERVER + DEMOCFG_ENABLE_WEBSERVER) < 1
//	#warning Please enable one of the network services. The restriction is only for RAM size limited devices.
//#endif

extern const SHELL_COMMAND_STRUCT Telnet_commands[];

static void Telnetd_shell_fn (pointer dummy) 
{  
	Shell(Telnet_commands,NULL);
}

const RTCS_TASK Telnetd_shell_template = {"Telnet_shell", 8, 2000, Telnetd_shell_fn, NULL};

//const HTTPD_ROOT_DIR_STRUCT root_dir[] = {
//	{ "", "tfs:" },
//	{ "usb", "c:" },
//	{ 0, 0 }
//};


void LCM1_initialize_networking(_enet_address  enet_address) 
{
	int_32                  error;
	IPCFG_IP_ADDRESS_DATA   ip_data;
//	_enet_address           enet_address;
//	_ip_address LWDNS_server_ipaddr;

	
//   boolean successFlag;
//   _ip_address          hostaddr;
//   char                 hostname[MAX_HOSTNAMESIZE];   
   
   
#if DEMOCFG_USE_POOLS && defined(DEMOCFG_RTCS_POOL_ADDR) && defined(DEMOCFG_RTCS_POOL_SIZE)
    /* use external RAM for RTCS buffers */
    _RTCS_mem_pool = _mem_create_pool((pointer)DEMOCFG_RTCS_POOL_ADDR, DEMOCFG_RTCS_POOL_SIZE);
#endif

//#if RTCS_MINIMUM_FOOTPRINT
    /* runtime RTCS configuration for devices with small RAM, for others the default BSP setting is used */
    _RTCSPCB_init = 15;
    _RTCSPCB_grow = 0;
    _RTCSPCB_max = 15;
    _RTCS_msgpool_init = 15;
    _RTCS_msgpool_grow = 0;
    _RTCS_msgpool_max  = 15;
    _RTCS_socket_part_init = 15;
    _RTCS_socket_part_grow = 0;
    _RTCS_socket_part_max  = 15;

    _RTCSTASK_stacksize = 4000;
    
//#endif

    error = RTCS_create();
    
#if RTCSCFG_ENABLE_LWDNS
    LWDNS_server_ipaddr = ENET_IPGATEWAY;
#endif
    
    ip_data.ip = ENET_IPADDR;
    ip_data.mask = ENET_IPMASK;
    ip_data.gateway = ENET_IPGATEWAY;
    
//    ENET_get_mac_address (DEMOCFG_DEFAULT_DEVICE, ENET_IPADDR, enet_address);
    error = ipcfg_init_device (DEMOCFG_DEFAULT_DEVICE, enet_address);    
//    LWDNS_server_ipaddr = 0x08080808;
//    ipcfg_add_dns_ip(BSP_DEFAULT_ENET_DEVICE,LWDNS_server_ipaddr);
    
    // check link status
    while(!ipcfg_get_link_active(BSP_DEFAULT_ENET_DEVICE))
    {
    };

    /* If DHCP Enabled, get IP address from DHCP server */
   if (DHCP_ENABLE)
   {
      do
      {
         error = ipcfg_bind_dhcp_wait(DEMOCFG_DEFAULT_DEVICE, 0, NULL);
      } while(error != IPCFG_ERROR_OK);
   }
   else
   {
      /* Else bind with static IP */
      printf ("\nStatic IP bind ... ");
      error = ipcfg_bind_staticip(DEMOCFG_DEFAULT_DEVICE, &ip_data);

      if (error != IPCFG_ERROR_OK)
      {
         printf("Error %08x!\n",error);
      }
      else
      {
         printf("Successful!\n");
      }
    }  
   
   if (error == IPCFG_ERROR_OK)
   {
      ipcfg_get_ip(BSP_DEFAULT_ENET_DEVICE, &ip_data);
   }

////   successFlag = RTCS_resolve_ip_address( "cnn.com", &hostaddr, hostname, MAX_HOSTNAMESIZE);
//   successFlag = RTCS_resolve_ip_address( "limitless-cove-7271.herokuapp.com", &hostaddr, hostname, MAX_HOSTNAMESIZE);
   
   
#if DEMOCFG_ENABLE_WEBSERVER
    {
        HTTPD_STRUCT *server;
        extern const HTTPD_CGI_LINK_STRUCT cgi_lnk_tbl[];
        extern const HTTPD_FN_LINK_STRUCT fn_lnk_tbl[];
        extern const TFS_DIR_ENTRY tfs_data[];
        
        error = _io_tfs_install("tfs:", tfs_data);
        if (error) printf("\nTFS install returned: %08x\n", error);

    #if RTCSCFG_ENABLE_IP4        
       /*old function*/
       /*server = httpd_server_init((HTTPD_ROOT_DIR_STRUCT*)root_dir, "\\mqx.html");*/
        
        server = httpd_server_init_af((HTTPD_ROOT_DIR_STRUCT*)root_dir, "\\index.htm",AF_INET);
        if(server)
        {
            HTTPD_SET_PARAM_CGI_TBL(server, (HTTPD_CGI_LINK_STRUCT*)cgi_lnk_tbl);
            httpd_server_run(server);
        }
        else
        {
            printf("Error: Server IPv4 init error.\n");
        }
    #endif
        
    #if RTCSCFG_ENABLE_IP6
        server = httpd_server_init_af((HTTPD_ROOT_DIR_STRUCT*)root_dir, "\\mqx.html",AF_INET6);

        if(server)
        {
            HTTPD_SET_PARAM_CGI_TBL(server, (HTTPD_CGI_LINK_STRUCT*)cgi_lnk_tbl);
            HTTPD_SET_PARAM_FN_TBL(server, (HTTPD_FN_LINK_STRUCT*)fn_lnk_tbl);
            httpd_server_run(server);
        }
        else
        {
            printf("Error: Server IPv6 init error.\n");
        }
    #endif

    }
#endif

#if DEMOCFG_ENABLE_FTP_SERVER
	FTPd_init("FTP_server", 7, 3000 );
#endif

#if DEMOCFG_ENABLE_TELNET_SERVER
	TELNETSRV_init("Telnet_server", 7, 2000, (RTCS_TASK_PTR) &Telnetd_shell_template );
#endif

#if DEMOCFG_ENABLE_KLOG && MQX_KERNEL_LOGGING
	RTCSLOG_enable(RTCSLOG_TYPE_FNENTRY);
	RTCSLOG_enable(RTCSLOG_TYPE_PCB);
#endif

}

#endif /* DEMOCFG_ENABLE_RTCS */
