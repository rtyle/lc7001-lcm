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
* $FileName: Command_Lists.c$
* $Version : 3.7.17.0$
* $Date    : Mar-15-2011$
*
* Comments:
*
*   
*
*END************************************************************************/

#include "includes.h"

const SHELL_COMMAND_STRUCT Shell_commands[] = {
#if DEMOCFG_ENABLE_USB_FILESYSTEM 
   { "cd",        Shell_cd },      
   { "copy",      Shell_copy },     
   { "del",       Shell_del },       
   { "dir",       Shell_dir },      
   { "mkdir",     Shell_mkdir },     
   { "pwd",       Shell_pwd },       
   { "read",      Shell_read },      
   { "ren",       Shell_rename },    
   { "rmdir",     Shell_rmdir },
   { "type",      Shell_type },
   { "write",     Shell_write }, 
#endif
   { "exit",      Shell_exit },      
   { "help",      Shell_help }, 

#if DEMOCFG_ENABLE_RTCS
   { "netstat",   Shell_netstat },  
   { "ipconfig",  Shell_ipconfig },
#if DEMOCFG_USE_WIFI   
   { "iwconfig",  Shell_iwconfig },
#endif      
#if RTCSCFG_ENABLE_ICMP
   { "ping",      Shell_ping },      
#endif   
#endif
   { "?",         Shell_command_list },     
   { NULL,        NULL } 
};

const SHELL_COMMAND_STRUCT Telnet_commands[] = {
   { "exit",      Shell_exit },      
   { "help",      Shell_help }, 

#if DEMOCFG_ENABLE_RTCS
#if RTCSCFG_ENABLE_ICMP
   { "ping",      Shell_ping },      
#endif   
#endif   

   { "?",         Shell_command_list },     
   
   { NULL,        NULL } 
};


#if DEMOCFG_ENABLE_RTCS && DEMOCFG_ENABLE_FTP_SERVER

// ftp root dir
char FTPd_rootdir[] = {"c:\\"}; 

//ftp commands
const FTPd_COMMAND_STRUCT FTPd_commands[] = {
   { "abor", FTPd_unimplemented },
   { "acct", FTPd_unimplemented },
   { "cdup", FTPd_cdup },        
   { "cwd",  FTPd_cd   },        
   { "feat", FTPd_feat },       
   { "help", FTPd_help },       
   { "dele", FTPd_dele },       
   { "list", FTPd_list },       
   { "mkd",  FTPd_mkdir},       
   { "noop", FTPd_noop },
   { "nlst", FTPd_nlst },       
   { "opts", FTPd_opts },
   { "pass", FTPd_pass },
   { "pasv", FTPd_pasv },
   { "port", FTPd_port },
   { "pwd",  FTPd_pwd  },       
   { "quit", FTPd_quit },
   { "rnfr", FTPd_rnfr },
   { "rnto", FTPd_rnto },
   { "retr", FTPd_retr },
   { "stor", FTPd_stor },
   { "rmd",  FTPd_rmdir},       
   { "site", FTPd_site },
   { "size", FTPd_size },
   { "syst", FTPd_syst },
   { "type", FTPd_type },
   { "user", FTPd_user },
   { "xcwd", FTPd_cd    },        
   { "xmkd", FTPd_mkdir },       
   { "xpwd", FTPd_pwd   },       
   { "xrmd", FTPd_rmdir },       
   { NULL,   NULL } 
};


#endif
/* EOF */
