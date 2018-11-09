
#include "app_uart.h"
#include "app_usbd.h"
#include "app_usbd_types.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_hid_generic.h"
#include "app_usbd_hid_mouse.h"
#include "app_usbd_hid_kbd.h"

#include "nrf_log.h"
#include "nrf_gpio.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"

#include "vortex2_usb.h"
#include "vortex2_global.h"
#include "vortex2_flash.h"

#include "sdk_errors.h"

#include "vortex2_radio.h"




#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define HID_GENERIC_INTERFACE  2
#define HID_GENERIC_EPIN       NRF_DRV_USBD_EPIN3

#define HID_DATA_EPOUT      NRF_DRV_USBD_EPOUT5

#define REPORT_IN_QUEUE_SIZE    1

#define REPORT_OUT_MAXSIZE 64

#define HID_GENERIC_EP_COUNT  2

#define ENDPOINT_LIST()                                      \
(                                                            \
        HID_GENERIC_EPIN                                     \
        ,NRF_DRV_USBD_EPOUT5                                  \
)

static bool m_report_pending;
#define APP_USBD_HID_VORTEX2_REPORT_DSC(dst){                            \
    0x05, 0x8C,       /* Usage Page (Generic Desktop),       */     \
    0x09, 0x06,       /* Usage (),                      */     \
    0xA1, 0x01,       /*  Collection (Application),          */     \
    0x09, 0x06,       /*   Usage (Pointer),                  */     \
    0x15, 0x00,                                                     \
    0x26, 0x00, 0xff,                                               \
    0x75, 0x08,       /* REPORT_SIZE (8) */                         \
    0x95, 0x40,       /* REPORT_COUNT (64) */                       \
    0x91, 0x82,                                                     \
    0x09, 0x06,                                                     \
    0x15, 0x00,                                                     \
    0x26, 0x00, 0xff,                                               \
    0x75, 0x08,       /* REPORT_SIZE (8) */                         \
    0x95, 0x40,       /* REPORT_COUNT (64) */                       \
    0x81, 0x82, /* INPUT(Data,Var,Abs,Vol) */                       \
    0xc0 /* END_COLLECTION */                                       \
}
APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(vortex_desc, APP_USBD_HID_VORTEX2_REPORT_DSC(0));
static const app_usbd_hid_subclass_desc_t * reps[] = {&vortex_desc};

static bool m_usb_connected = false;
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

static uint8_t senddata[64]={0x00};


static char m_cdc_data_array[CDC_MAX_DATA_LEN];
static char CDC_DATA_BUF[CDC_MAX_DATA_LEN];

static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event);
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);
/*
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic,
                                HID_GENERIC_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(),
                                reps,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_BOOT,
                                APP_USBD_HID_PROTO_MOUSE);*/
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic,
                                HID_GENERIC_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(), 
                                reps,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_BOOT,
                                APP_USBD_HID_PROTO_GENERIC);

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;

        case APP_USBD_EVT_DRV_RESUME:
            break;

        case APP_USBD_EVT_STARTED:
            break;

        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;

        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;

        case APP_USBD_EVT_POWER_REMOVED:
        {
            NRF_LOG_INFO("USB power removed");
            m_usb_connected = false;
            app_usbd_stop();
        }
            break;

        case APP_USBD_EVT_POWER_READY:
        {
            NRF_LOG_INFO("USB ready");
            m_usb_connected = true;
            app_usbd_start();
        }
            break;

        default:
            break;
    }
}
size_t datasize;
uint64_t len = 0;
uint8_t buf[64];
uint8_t sendbuf[240];
int eee=0;


static void sendData(app_usbd_hid_generic_t const * p_generic){
    app_usbd_hid_inst_t const * p_hinst = &p_generic->specific.inst.hid_inst;
    //NRF_LOG_INFO("APP_USBD_HID_USER_EVT_IN_REPORT_DONE");
    int len=0;
    uint8_t lastpage=0;
    memset(buf,0,64);
    memcpy(&len,p_hinst->p_rep_buffer_out->p_buff+1,1);
    if(len>128){
      lastpage=len>>7;
      len=len&0x7F;
    }
    memcpy(buf,p_hinst->p_rep_buffer_out->p_buff+2,len);


    if (!strcmp(buf,START_RADIO)){
      NRF_LOG_INFO("in start");
      int ret = radio_start();
      if (ret == 1){
        memset(buf,0,64);
        buf[0]=strlen(START_RADIO);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],START_RADIO,strlen(START_RADIO));
        memcpy(&buf[3+strlen(START_RADIO)],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }
    }else if(!strcmp(buf,SCAN_START)){
      radio_scan_start();
    }else if(!strcmp(buf,SCAN_STOP)){
      char scanbuf[220];
      memset(scanbuf,0,220);
      radio_scan_stop(scanbuf);
      char* pch;
      pch=strtok(scanbuf,"\r");
      if(pch == NULL){
        memset(buf,0,64);
        buf[0]=strlen(SCAN_STOP);  //命令
        buf[1]=strlen("false");         //成功或失败
        memcpy(&buf[3],SCAN_STOP,strlen(SCAN_STOP));
        memcpy(&buf[3+strlen(SCAN_STOP)],"false",strlen("false"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        return;
      }
      while(pch != NULL){
        NRF_LOG_INFO("%s\r\n",pch);
        NRF_LOG_INFO("%d\r\n",strlen(pch));

        memset(buf,0,64);
        buf[0]=strlen(SCAN_STOP);  //命令
        buf[1]=strlen("ok");         //成功或失败
        buf[2]=strlen(pch);                    //返回值
        memcpy(&buf[3],SCAN_STOP,strlen(SCAN_STOP));
        memcpy(&buf[3+strlen(SCAN_STOP)],"ok",strlen("ok"));
        memcpy(&buf[3+strlen(SCAN_STOP)+strlen("ok")],pch,strlen(pch));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        pch = strtok(NULL, "\r");
      }
    }else if(!strcmp(buf,MATCH_RADIO)){
      NRF_LOG_INFO("in match");
      int ret = radio_match();
      if(ret == 1){
        memset(buf,0,64);
        buf[0]=strlen(MATCH_RADIO);  //命令
        buf[1]=strlen("ok");         //成功或失败
        buf[2]=0;                    //返回值
        memcpy(&buf[3],MATCH_RADIO,strlen(MATCH_RADIO));
        memcpy(&buf[3+strlen(MATCH_RADIO)],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("match ok");
      }else if(ret == -100){
        memset(buf,0,64);
        buf[0]=strlen(MATCH_RADIO);  //命令
        buf[1]=strlen("false");         //成功或失败
        buf[2]=strlen("timeout");                    //返回值
        memcpy(&buf[3],MATCH_RADIO,strlen(MATCH_RADIO));
        memcpy(&buf[3+strlen(MATCH_RADIO)],"false",strlen("false"));
        memcpy(&buf[3+strlen(MATCH_RADIO)+strlen("false")],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("match timeout");
      }
    }else if(!strcmp(buf,CANCEL_MATCH)){

      uint32_t saveaddr=0xFF000;
      uint32_t savechannel=0xFF004;
      uint32_t baseaddr=0;
      uint8_t channel=0;
      char name[20];
      memset(name,0,20);
      uint8_t namelen=0;
      uint8_t allpage[1024];
      memset(allpage,0,1024);
      vortex2_flash_read_bytes(saveaddr,allpage,1024);
      baseaddr=allpage[0]<<24 | allpage[1]<<16 | allpage[2]<<8 | allpage[3]<<0;
      channel=allpage[4];
      namelen=allpage[8];
      memcpy(name,&allpage[9],namelen);
      char cancelbuf[50];
      memset(cancelbuf,0,50);

      sprintf(cancelbuf,"|1|3|0x%x|%d|%s|",baseaddr,channel,name);
      NRF_LOG_INFO("cancel match=%s",cancelbuf);
      NRF_LOG_INFO("name=%s",name);
      NRF_LOG_INFO("namelen=%d",namelen);

      int ret = radio_cancel_match(cancelbuf);
      if (ret==1){
        //sucess
        memset(buf,0,64);
        buf[0]=strlen(CANCEL_MATCH);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],CANCEL_MATCH,strlen(CANCEL_MATCH));
        memcpy(&buf[strlen(CANCEL_MATCH)+3],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("cancel match OK");
      }else if(ret==-1){
        //false
        memset(buf,0,64);
        buf[0]=strlen(CANCEL_MATCH);
        buf[1]=strlen("false");
        buf[2]=strlen("name mismatch");
        memcpy(&buf[3],CANCEL_MATCH,strlen(CANCEL_MATCH));
        memcpy(&buf[strlen(CANCEL_MATCH)+3],"false",strlen("false"));
        memcpy(&buf[strlen("false")+strlen(CANCEL_MATCH)+3],"name mismatch",strlen("name mismatch"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("cancel match FAIL");
      }else{
        memset(buf,0,64);
        buf[0]=strlen(CANCEL_MATCH);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],CANCEL_MATCH,strlen(CANCEL_MATCH));
        memcpy(&buf[strlen(CANCEL_MATCH)+3],"false",strlen("false"));
        memcpy(&buf[strlen("false")+strlen(CANCEL_MATCH)+3],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("cancel match FAIL");
      
      }
     
    }else if(!strcmp(buf,GET_STATUS)){
      NRF_LOG_INFO("in get status");
      int status = radio_status();
      NRF_LOG_INFO("status=%d",status);
      if (status==1){
        memset(buf,0,64);
        buf[0]=strlen(GET_STATUS);
        buf[1]=strlen("ok");
        buf[2]=strlen(STATUS_APP);
        memcpy(&buf[3],GET_STATUS,strlen(GET_STATUS));
        memcpy(&buf[3+strlen(GET_STATUS)],"ok",strlen("ok"));
        memcpy(&buf[3+strlen(GET_STATUS)+strlen("ok")],STATUS_APP,strlen(STATUS_APP));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else if(status==2){
        memset(buf,0,64);
        buf[0]=strlen(GET_STATUS);
        buf[1]=strlen("ok");
        buf[2]=strlen(STATUS_BOOT);
        memcpy(&buf[3],GET_STATUS,strlen(GET_STATUS));
        memcpy(&buf[3+strlen(GET_STATUS)],"ok",strlen("ok"));
        memcpy(&buf[3+strlen(GET_STATUS)+strlen("ok")],STATUS_BOOT,strlen(STATUS_BOOT));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else{
        memset(buf,0,64);
        buf[0]=strlen(GET_STATUS);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],GET_STATUS,strlen(GET_STATUS));
        memcpy(&buf[3+strlen(GET_STATUS)],"false",strlen("false"));
        memcpy(&buf[3+strlen(GET_STATUS)+strlen("false")],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }
    }else if(!strcmp(buf,GET_NAME)){
      char name[20];
      int ret = radio_getname(name);
      if(ret == 1){
        NRF_LOG_INFO("===%d",name[0]);
        NRF_LOG_INFO("===%d",strlen(name));
        memset(buf,0,64);
        buf[0]=strlen(GET_NAME);
        buf[1]=strlen("ok");
        buf[2]=strlen(name);
        memcpy(&buf[3],GET_NAME,strlen(GET_NAME));
        memcpy(&buf[strlen(GET_NAME)+3],"ok",strlen("ok"));
        memcpy(&buf[strlen("ok")+strlen(GET_NAME)+3],name,strlen(name));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else{
        memset(buf,0,64);
        buf[0]=strlen(GET_NAME);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],GET_NAME,strlen(GET_NAME));
        memcpy(&buf[strlen(GET_NAME)+3],"false",strlen("false"));
        memcpy(&buf[strlen("false")+strlen(GET_NAME)+3],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      
      }
    }else if(!strncmp(buf,SET_NAME,strlen(SET_NAME))){
      NRF_LOG_INFO("!!!!!!!  in set name.");
      char name[20];
      memset(name,0,20);
      memcpy(name,&buf[strlen(SET_NAME)],(len-strlen(SET_NAME)));
      
      NRF_LOG_INFO("%s",name);
      int ret = radio_setname(name);
      if(ret == 1){
        memset(buf,0,64);
        buf[0]=strlen(SET_NAME);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],SET_NAME,strlen(SET_NAME));
        memcpy(&buf[3+strlen(SET_NAME)],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else{
        memset(buf,0,64);
        buf[0]=strlen(SET_NAME);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],SET_NAME,strlen(SET_NAME));
        memcpy(&buf[3+strlen(SET_NAME)],"false",strlen("false"));
        memcpy(&buf[3+strlen(SET_NAME)+strlen("false")],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }
    }else if(!strncmp(buf,ERASE_RADIO,sizeof(ERASE_RADIO))){ 
      NRF_LOG_INFO("in erase");
      uint32_t eraselen=buf[11]<<24 | buf[12]<<16 | buf[13]<<8 | buf[14];
      int ret = radio_erase(eraselen);
      if(ret == 1){
        memset(buf,0,64);
        buf[0]=sizeof(ERASE_RADIO);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],ERASE_RADIO,sizeof(ERASE_RADIO));
        memcpy(&buf[3+sizeof(ERASE_RADIO)],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else if(ret==-1){
        memset(buf,0,64);
        buf[0]=sizeof(ERASE_RADIO);
        buf[1]=strlen("false");
        buf[2]=0;
        memcpy(&buf[3],ERASE_RADIO,sizeof(ERASE_RADIO));
        memcpy(&buf[3+sizeof(ERASE_RADIO)],"false",strlen("false"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }else{
        memset(buf,0,64);
        buf[0]=sizeof(ERASE_RADIO);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],ERASE_RADIO,sizeof(ERASE_RADIO));
        memcpy(&buf[3+sizeof(ERASE_RADIO)],"false",strlen("false"));
        memcpy(&buf[3+sizeof(ERASE_RADIO)+strlen("false")],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
      }
    }else if(!strcmp(buf,JUMP_BOOT)){
      NRF_LOG_INFO("in jump");
      vortex2_jump_to_boot();
    }else if(!strcmp(buf,READ_ADDR_CHANNEL)){
      NRF_LOG_INFO("in read addr channel");
      uint32_t saveaddr=0xFF000;
      uint32_t savechannel=0xFF004;
      uint8_t recvaddr1=0;
      uint8_t recvaddr2=0;
      uint8_t recvaddr3=0;
      uint8_t recvaddr4=0;
      uint8_t recvchannel=0;
      vortex2_flash_read_bytes(saveaddr,&recvaddr1,1);
      NRF_LOG_INFO("-0x%x",recvaddr1)
      vortex2_flash_read_bytes(saveaddr+1,&recvaddr2,1);
      NRF_LOG_INFO("-0x%x",recvaddr2)
      vortex2_flash_read_bytes(saveaddr+2,&recvaddr3,1);
      NRF_LOG_INFO("-0x%x",recvaddr3)
      vortex2_flash_read_bytes(saveaddr+3,&recvaddr4,1);
      NRF_LOG_INFO("-0x%x",recvaddr4)
      vortex2_flash_read_bytes(savechannel,&recvchannel,1);
      NRF_LOG_INFO("-0x%x",recvchannel)
      memset(buf,0,64);
      buf[0]=strlen(READ_ADDR_CHANNEL);
      buf[1]=strlen("ok");
      buf[2]=5;
      memcpy(&buf[3],READ_ADDR_CHANNEL,strlen(READ_ADDR_CHANNEL));
      memcpy(&buf[3+strlen(READ_ADDR_CHANNEL)],"ok",strlen("ok"));
      buf[strlen(READ_ADDR_CHANNEL)+strlen("ok")+3]=recvaddr1;
      buf[strlen(READ_ADDR_CHANNEL)+strlen("ok")+4]=recvaddr2;
      buf[strlen(READ_ADDR_CHANNEL)+strlen("ok")+5]=recvaddr3;
      buf[strlen(READ_ADDR_CHANNEL)+strlen("ok")+6]=recvaddr4;
      buf[strlen(READ_ADDR_CHANNEL)+strlen("ok")+7]=recvchannel;
      app_usbd_hid_generic_in_report_set(
            &m_app_hid_generic,
            buf,
            datasize);
      NRF_LOG_INFO("in read addr channel over");
    }else if(!strncmp(buf,WRITE_ADDR_CHANNEL,sizeof(WRITE_ADDR_CHANNEL))){
      NRF_LOG_INFO("in write addr channel");
      uint32_t writeaddr=(buf[sizeof(WRITE_ADDR_CHANNEL)]<<24) | (buf[sizeof(WRITE_ADDR_CHANNEL)+1]<<16) | (buf[sizeof(WRITE_ADDR_CHANNEL)+2]<<8) | (buf[sizeof(WRITE_ADDR_CHANNEL)+3]);
      uint8_t channel=buf[sizeof(WRITE_ADDR_CHANNEL)+4]&0xFF;
      NRF_LOG_INFO("addr=%d",writeaddr);
      NRF_LOG_INFO("channel=%d",channel);
      NRF_LOG_INFO("%d",buf[sizeof(WRITE_ADDR_CHANNEL)]);
      NRF_LOG_INFO("%d",buf[sizeof(WRITE_ADDR_CHANNEL)+1]);
      NRF_LOG_INFO("%d",buf[sizeof(WRITE_ADDR_CHANNEL)+2]);
      NRF_LOG_INFO("%d",buf[sizeof(WRITE_ADDR_CHANNEL)+3]);
      NRF_LOG_INFO("%d",buf[sizeof(WRITE_ADDR_CHANNEL)+4]);
      NRF_LOG_INFO("sizeof=%d",sizeof(WRITE_ADDR_CHANNEL));

      uint32_t saveaddr=0xFF000;
      uint32_t savechannel=0xFF004;
      uint8_t allpage[1024];
      memset(allpage,0,1024);
      vortex2_flash_read_bytes(saveaddr,allpage,1024);
      
      vortex2_flash_page_erase(saveaddr);
      allpage[0]=buf[sizeof(WRITE_ADDR_CHANNEL)];
      allpage[1]=buf[sizeof(WRITE_ADDR_CHANNEL)+1];
      allpage[2]=buf[sizeof(WRITE_ADDR_CHANNEL)+2];
      allpage[3]=buf[sizeof(WRITE_ADDR_CHANNEL)+3];
      allpage[4]=buf[sizeof(WRITE_ADDR_CHANNEL)+4];
      allpage[5]=0;
      allpage[6]=0;
      allpage[7]=0;
  
      vortex2_flash_write_bytes(saveaddr,allpage,1024);

      memset(buf,0,64);
      buf[0]=sizeof(WRITE_ADDR_CHANNEL);
      buf[1]=strlen("ok");
      buf[2]=0;
      memcpy(&buf[3],WRITE_ADDR_CHANNEL,sizeof(WRITE_ADDR_CHANNEL));
      memcpy(&buf[3+strlen(WRITE_ADDR_CHANNEL)],"ok",strlen("ok"));
      app_usbd_hid_generic_in_report_set(
            &m_app_hid_generic,
            buf,
            datasize);
    }else if(!strncmp(buf,SET_FLASH_NAME,strlen(SET_FLASH_NAME))){
      NRF_LOG_INFO("in set flash.");
      char name[20];
      memset(name,0,20);
      memcpy(name,&buf[strlen(SET_FLASH_NAME)],len-strlen(SET_FLASH_NAME));
      uint32_t saveaddr=0xFF000;
      uint32_t savechannel=0xFF004;
      uint8_t allpage[1024];
      memset(allpage,0,1024);
      vortex2_flash_read_bytes(saveaddr,allpage,1024);
      vortex2_flash_page_erase(saveaddr);
      memset(&allpage[8],0xFF,1024-8);
      allpage[8]=len-strlen(SET_FLASH_NAME);
      memcpy(&allpage[9],name,allpage[8]);
      vortex2_flash_write_bytes(saveaddr,allpage,1024);
      memset(buf,0,64);
      buf[0]=strlen(SET_FLASH_NAME);
      buf[1]=strlen("ok");
      buf[2]=0;
      memcpy(&buf[3],SET_FLASH_NAME,strlen(SET_FLASH_NAME));
      memcpy(&buf[3+strlen(SET_FLASH_NAME)],"ok",strlen("ok"));
      app_usbd_hid_generic_in_report_set(
            &m_app_hid_generic,
            buf,
            datasize);
      NRF_LOG_INFO("name=%s",name);
      NRF_LOG_INFO("len=%d",allpage[8]);
    }else if(!strcmp(buf,APP_JUMP_BOOT)){
      NRF_LOG_INFO("app->boot");
      int ret = radio_app_to_boot();
      if (ret==1){
        //sucess
        memset(buf,0,64);
        buf[0]=strlen(APP_JUMP_BOOT);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],APP_JUMP_BOOT,strlen(APP_JUMP_BOOT));
        memcpy(&buf[strlen(APP_JUMP_BOOT)+3],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("app jump to boot OK");
      }else if(ret == -1){
        //false
        memset(buf,0,64);
        buf[0]=strlen(APP_JUMP_BOOT);
        buf[1]=strlen("false");
        buf[2]=0;
        memcpy(&buf[3],APP_JUMP_BOOT,strlen(APP_JUMP_BOOT));
        memcpy(&buf[strlen(APP_JUMP_BOOT)+3],"false",strlen("false"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("app jump to boot FAIL");
      }else{
        memset(buf,0,64);
        buf[0]=strlen(APP_JUMP_BOOT);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],APP_JUMP_BOOT,strlen(APP_JUMP_BOOT));
        memcpy(&buf[strlen(APP_JUMP_BOOT)+3],"false",strlen("false"));
        memcpy(&buf[strlen("false")+strlen(APP_JUMP_BOOT)+3],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("app jump to boot timeout");
      }
    }else if(!strcmp(buf,BOOT_JUMP_APP)){
      NRF_LOG_INFO("boot->app");
      int ret=radio_boot_to_app();
      if(ret == 1){
        //sucess
        memset(buf,0,64);
        buf[0]=strlen(BOOT_JUMP_APP);
        buf[1]=strlen("ok");
        buf[2]=0;
        memcpy(&buf[3],BOOT_JUMP_APP,strlen(BOOT_JUMP_APP));
        memcpy(&buf[strlen(BOOT_JUMP_APP)+3],"ok",strlen("ok"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("boot jump to app OK");
      }else if(ret==-1){
        //fail
        memset(buf,0,64);
        buf[0]=strlen(BOOT_JUMP_APP);
        buf[1]=strlen("false");
        buf[2]=0;
        memcpy(&buf[3],BOOT_JUMP_APP,strlen(BOOT_JUMP_APP));
        memcpy(&buf[strlen(BOOT_JUMP_APP)+3],"false",strlen("false"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("boot jump to app FAIL");
      }else{
        memset(buf,0,64);
        buf[0]=strlen(BOOT_JUMP_APP);
        buf[1]=strlen("false");
        buf[2]=strlen("timeout");
        memcpy(&buf[3],BOOT_JUMP_APP,strlen(BOOT_JUMP_APP));
        memcpy(&buf[strlen(BOOT_JUMP_APP)+3],"false",strlen("false"));
        memcpy(&buf[strlen("false")+strlen(BOOT_JUMP_APP)+3],"timeout",strlen("timeout"));
        app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        NRF_LOG_INFO("boot jump to app FAIL");
      }
    }else if(!strncmp(buf,"BN",2)){
      //NRF_LOG_INFO("in burn");
      //len=len&0x7F;
      //uint8_t lastpage=len>>7;
      uint32_t crc0=buf[len-1];
      uint32_t crc1=0;
      uint8_t buf2[60];
      memcpy(buf2,&buf[2],len-3);
      memcpy(&sendbuf[eee*60],buf2,len-3);
      if(eee==3){
        crc1=crc32_check((uint8_t *)sendbuf,eee*60+len-3)&0xFF;
        if(crc0==crc1){
          int burnreturn = radio_burn(sendbuf,eee*60+len-3,lastpage);
          memset(sendbuf,0,240);
          memset(buf,0,64);
          eee=0;
          if (burnreturn==1){
            buf[0]=strlen("BN");
            buf[1]=strlen("ok");
            buf[2]=0;
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"ok",strlen("ok"));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-1){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_ERASE_FLASH);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_ERASE_FLASH,strlen(BURN_RETURN_ERASE_FLASH));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-2){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_CONNECT);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_CONNECT,strlen(BURN_RETURN_CONNECT));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-3){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_ERASE_AND_CONNECT);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_ERASE_AND_CONNECT,strlen(BURN_RETURN_ERASE_AND_CONNECT));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==2){
            buf[0]=strlen("BN");
            buf[1]=strlen("ok");
            buf[2]=strlen(BURN_OVER);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"ok",strlen("ok"));
            memcpy(&buf[3+strlen("BN")+strlen("ok")],BURN_OVER,strlen(BURN_OVER));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-5){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_OVER_ERR);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_OVER_ERR,strlen(BURN_OVER_ERR));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-100){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen("timeout");
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],"timeout",strlen("timeout"));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else{
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_UNKOWNERR);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_UNKOWNERR,strlen(BURN_RETURN_UNKOWNERR));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }
        }else{
          memset(buf,0,64);
          eee=0;
          memset(sendbuf,0,240);
          buf[0]=strlen("BN");
          buf[1]=strlen("false");
          buf[2]=strlen(BURN_CRC_ERROR);
          memcpy(&buf[3],"BN",strlen("BN"));
          memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
          memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_CRC_ERROR,strlen(BURN_CRC_ERROR));
          app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        }
      }else if((len-3)!=60){
        crc1=crc32_check((uint8_t *)sendbuf,eee*60+len-3)&0xFF;
        if(crc0==crc1){
          SEGGER_RTT_printf(0,"last one\r\n");
          int burnreturn = radio_burn(sendbuf,eee*60+len-3,lastpage);
          memset(sendbuf,0,240);
          memset(buf,0,64);
          eee=0;
          if (burnreturn==1){
            buf[0]=strlen("BN");
            buf[1]=strlen("ok");
            buf[2]=0;
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"ok",strlen("ok"));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-1){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_ERASE_FLASH);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_ERASE_FLASH,strlen(BURN_RETURN_ERASE_FLASH));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-2){
             buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_CONNECT);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_CONNECT,strlen(BURN_RETURN_CONNECT));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-3){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_ERASE_AND_CONNECT);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_ERASE_AND_CONNECT,strlen(BURN_RETURN_ERASE_AND_CONNECT));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==2){
            buf[0]=strlen("BN");
            buf[1]=strlen("ok");
            buf[2]=strlen(BURN_OVER);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"ok",strlen("ok"));
            memcpy(&buf[3+strlen("BN")+strlen("ok")],BURN_OVER,strlen(BURN_OVER));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-5){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_OVER_ERR);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_OVER_ERR,strlen(BURN_OVER_ERR));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else if(burnreturn==-100){
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen("timeout");
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],"timeout",strlen("timeout"));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }else{
            buf[0]=strlen("BN");
            buf[1]=strlen("false");
            buf[2]=strlen(BURN_RETURN_UNKOWNERR);
            memcpy(&buf[3],"BN",strlen("BN"));
            memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
            memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_RETURN_UNKOWNERR,strlen(BURN_RETURN_UNKOWNERR));
            app_usbd_hid_generic_in_report_set(
                &m_app_hid_generic,
                buf,
                datasize);
          }
        }else{
          memset(buf,0,64);
          eee=0;
          memset(sendbuf,0,240);
          buf[0]=strlen("BN");
          buf[1]=strlen("false");
          buf[2]=strlen(BURN_CRC_ERROR);
          memcpy(&buf[3],"BN",strlen("BN"));
          memcpy(&buf[3+strlen("BN")],"false",strlen("false"));
          memcpy(&buf[3+strlen("BN")+strlen("false")],BURN_CRC_ERROR,strlen(BURN_CRC_ERROR));
          app_usbd_hid_generic_in_report_set(
              &m_app_hid_generic,
              buf,
              datasize);
        }
      }else{
        eee++;
      }
    }else{
      NRF_LOG_INFO("err cmd\r\n");
    }

    
}

static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event)
{
    switch (event)
    {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
        {
            app_usbd_hid_generic_out_report_get(&m_app_hid_generic,&datasize);
            sendData(&m_app_hid_generic);
            ASSERT(0);
            break;
        }
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
        {
            //NRF_LOG_INFO("APP_USBD_HID_USER_EVT_IN_REPORT_DONE");
            //NRF_LOG_INFO("APP_USBD_HID_USER_EVT_IN_REPORT_DONE");
            m_report_pending = false;
            //hid_generic_mouse_process_state();
            //bsp_board_led_invert(LED_HID_REP_IN);
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        {
            NRF_LOG_INFO("SET_BOOT_PROTO");
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
        {
            NRF_LOG_INFO("SET_REPORT_PROTO");
            break;
        }
        default:
            NRF_LOG_INFO("default");
            break;
    }
}


static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            /*Setup first transfer*/
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_cdc_data_array,
                                                   1);
            UNUSED_VARIABLE(ret);
            NRF_LOG_INFO("CDC ACM port opened");
            break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            NRF_LOG_INFO("CDC ACM port closed");
            if (m_usb_connected)
            {
            }
            break;

        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            break;

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            static uint8_t index = 0;
            if(index > 0){
              uint8_t temp = m_cdc_data_array[0];
              memset(m_cdc_data_array,0,index);
              m_cdc_data_array[0] = temp;
              index = 0;
            }
            index++;

            do
            {
                size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                app_uart_put(m_cdc_data_array[index-1]);
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                            &m_cdc_data_array[index],
                                            1);
                if (ret == NRF_SUCCESS)
                {
                    index++;
                }
            }
            while (ret == NRF_SUCCESS);
            break;
        }
        default:
            NRF_LOG_INFO("default");
            break;
    }
}
static const app_usbd_config_t usbd_config = {
    .ev_state_proc = usbd_user_ev_handler
};

static void init_usb_cdc(){
    ret_code_t ret;
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
}

static void init_usb_hid(){
    ret_code_t ret;
    app_usbd_class_inst_t const * class_inst_generic;
    class_inst_generic = app_usbd_hid_generic_class_inst_get(&m_app_hid_generic);
    ret = app_usbd_class_append(class_inst_generic);
    APP_ERROR_CHECK(ret);
}

void init_vortex2_usb(void){
    ret_code_t ret;
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    
    //app_usbd_serial_num_generate();
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    //init_usb_cdc();
    init_usb_hid();
    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);

}
void uninit_vortex2_usb(){
    ret_code_t ret;
    ret = app_usbd_uninit();
    APP_ERROR_CHECK(ret);
    app_usbd_stop();
    app_usbd_disable();
    nrf_drv_power_usbevt_uninit();
}
