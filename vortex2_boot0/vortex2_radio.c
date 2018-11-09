#include "vortex2_radio.h"


uint8_t         *rxBuf; 
uint8_t         mybuf[220];
uint8_t         radio_group;
uint32_t        radio_address;
int             radio_power; 
int             radio_frequencyBand; 
int             radio_speed; 
int             radio_maxSize;

uint32_t length  = 19872;
uint32_t length2  = 98136;
uint32_t crc_n = 0;

int matchOK=0;
int eraseOK=0;
int burnOK=0;
int statusOK=0;
int getnameOK=0;
int setnameOK=0;
int apptobootOK=0;
int boottoappOK=0;
int cancelmatchOK=0;

int startscan=0;
int scannum = 0;

uint32_t irqBaseaddr=0;
uint8_t irqChannel=0;
uint8_t defaultname[20];

uint8_t cmpMatch[50];


radio_callback_t m_data_pkt_cb;
FrameBuffer mybufa;
Packets packet;

int timeoutNum=0;
int timeoutReal=-100;

uint8_t         scan_buf[220];

void clock_initialization()
{
  /* Start 16 MHz crystal oscillator */
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
  {
        // Do nothing.
  }

    /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
  NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART    = 1;

  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
  {
        // Do nothing.
  }
}

void wait_event(char* scc)
{
  //SEGGER_RTT_printf(0,"wait_event=%s\r\n",scc);
  if(!strcmp(scc,MATCH_RADIO_OK)){
    matchOK=1;
  }else if(!strcmp(scc,ERASE_RADIO_OK)){
    eraseOK=1;
  }else if(!strcmp(scc,ERASE_RADIO_FALSE)){
    eraseOK=-1;
  }else if(!strcmp(scc,BURN_RADIO_ONE_OK)){
    burnOK=1;
  }else if(!strcmp(scc,BURN_RETURN_ERASE_FLASH)){
    burnOK=-1;
    NRF_LOG_INFO("burn once err1!!!!");
  }else if(!strcmp(scc,BURN_RETURN_CONNECT)){
    burnOK=-2;
    NRF_LOG_INFO("burn once err2!!!!");
  }else if(!strcmp(scc,BURN_RETURN_ERASE_AND_CONNECT)){
    burnOK=-3;
    NRF_LOG_INFO("burn once err3!!!!");
  }else if(!strcmp(scc,BURN_RETURN_UNKOWNERR)){
    burnOK=-4;
    NRF_LOG_INFO("burn once err4!!!!");
  }else if(!strcmp(scc,BURN_OVER)){
    NRF_LOG_INFO("burn over!!!!");
    burnOK=2;
  }else if(!strcmp(scc,BURN_OVER_ERR)){
    NRF_LOG_INFO("burn over err!!!!");
    burnOK=-5;
  }else if(!strcmp(scc,STATUS_APP)){
    statusOK=1;
  }else if(!strcmp(scc,STATUS_BOOT)){
    statusOK=2;
  }else if(!strcmp(scc,GET_NAME)){
    NRF_LOG_INFO("GET_NAME ok");
    getnameOK=1;
  }else if(!strcmp(scc,SET_NAME)){
    NRF_LOG_INFO("SET_NAME ok");
    setnameOK=1;
  }else if(!strcmp(scc,APP_JUMP_BOOT_OK)){
    apptobootOK=1;
  }else if(!strcmp(scc,APP_JUMP_BOOT_FAIL)){
    apptobootOK=-1;
  }else if(!strcmp(scc,BOOT_JUMP_APP_OK)){
    boottoappOK=1;
  }else if(!strcmp(scc,BOOT_JUMP_APP_FAIL)){
    boottoappOK=-1;
  }else if(!strcmp(scc,CANCEL_MATCH_OK)){
    cancelmatchOK=1;
  }else if(!strcmp(scc,CANCEL_MATCH_FAIL)){
    cancelmatchOK=-1;
  }
}

int radio_start()
{
  memset(scan_buf,0,220);

  clock_initialization();
  
  uint32_t saveaddr=0xFF000;
  uint32_t savechannel=0xFF004;
  uint8_t recvaddr1=0;
  uint8_t recvaddr2=0;
  uint8_t recvaddr3=0;
  uint8_t recvaddr4=0;
  uint8_t recvchannel=0;
  vortex2_flash_read_bytes(saveaddr,&recvaddr1,1);
  vortex2_flash_read_bytes(saveaddr+1,&recvaddr2,1);
  vortex2_flash_read_bytes(saveaddr+2,&recvaddr3,1);
  vortex2_flash_read_bytes(saveaddr+3,&recvaddr4,1);
  vortex2_flash_read_bytes(savechannel,&recvchannel,1);
  uint32_t baseaddr=recvaddr1<<24 | recvaddr2<<16 | recvaddr3<<8 | recvaddr4;
  uint8_t channel=recvchannel;
   
  uint8_t namelen=0;
  vortex2_flash_read_bytes(saveaddr+8,&namelen,1);
  NRF_LOG_INFO("start len=%d",namelen);

  if(namelen<=20){
    char name[20];
    memset(name,0,20);
    vortex2_flash_read_bytes(saveaddr+9,(uint8_t*)name,namelen);
  }
    
  irqBaseaddr=baseaddr;
  irqChannel=channel;

  memset(cmpMatch,0,sizeof(cmpMatch));
  sprintf(cmpMatch,"|1|0|0x%x|%d|",irqBaseaddr,irqChannel);
  NRF_LOG_INFO("cmpMatch=%s",cmpMatch);

  RadioConfig userRadioConfig;
  userRadioConfig.baseAddress=irqBaseaddr;
  userRadioConfig.channel=irqChannel;
  userRadioConfig.frequencyBand = MICROBIT_RADIO_DEFAULT_FREQUENCY;
  userRadioConfig.power = MICROBIT_RADIO_DEFAULT_TX_POWER;
  userRadioConfig.speed = RADIO_MODE_MODE_Nrf_2Mbit;
  userRadioConfig.maxSize = MICROBIT_RADIO_MAX_PACKET_SIZE;

  radio_init(&userRadioConfig);
  radio_disable();
  radio_enable();
  radio_register_callback(&wait_event);

  return 1;
}

int radio_match()
{
  matchOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)"|1|0|",strlen("|1|0|"));
  int k=0;
  while(matchOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)"|1|0|",strlen("|1|0|"));
      k=0;
    }
    if(timeoutNum >= 1000){
      timeoutNum = 0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum += 1;
    }
  }
  return 1;
}


void radio_scan_stop(char* msg)
{
  if(strlen(scan_buf) == 0){
    ;
  }else{
    memcpy(msg,scan_buf,strlen(scan_buf));
  }

  startscan = 0;
  scannum = 0;
}

void radio_scan_start()
{
  memset(scan_buf,0,220);
  startscan = 1;
}

int radio_cancel_match(char *cancelbuf)
{
  cancelmatchOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)cancelbuf,strlen(cancelbuf));
  int k=0;
   while(cancelmatchOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)cancelbuf,strlen(cancelbuf));
      k=0;
    }
    if(timeoutNum >= 1000){
      timeoutNum = 0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum=0;
    }
  }
  return cancelmatchOK;
}

int radio_status()
{
  statusOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)"|3|2|",strlen("|3|2|"));
  int k=0;
  while(statusOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)"|3|2|",strlen("|3|2|"));
      k=0;
    }
    if(timeoutNum >= 1000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum += 1;
    }
  }
  return statusOK;
}

int radio_app_to_boot()
{
  apptobootOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)"|3|1|",strlen("|3|1|"));
  int k=0;
  while(apptobootOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)"|3|1|",strlen("|3|1|"));
      k=0;
    }
    if(timeoutNum>=1000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  return apptobootOK;
}

int radio_boot_to_app()
{
  boottoappOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)"|3|0|",strlen("|3|0|"));
  int k=0;
  while(boottoappOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)"|3|0|",strlen("|3|0|"));
      k=0;
    }
    if(timeoutNum>=1000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  return boottoappOK;
}

int radio_getname(char* name)
{
  getnameOK=0;
  timeoutNum=0;
  radio_send((uint8_t *)"|2|2|",strlen("|2|2|"));
  int k=0;
  while(getnameOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)"|2|2|",strlen("|2|2|"));
      k=0;
    }
    if(timeoutNum>=1000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  memcpy(name,defaultName,strlen(defaultName));
  return 1;
}

int radio_setname(char *name)
{
  setnameOK=0;
  timeoutNum=0;
  char buf[50];
  memset(buf,0,50);
  sprintf(buf,"|2|1|%s|",name);
  NRF_LOG_INFO("%s",buf);
  radio_send((uint8_t *)buf,strlen(buf));
  int k=0;
  while(setnameOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)buf,strlen(buf));
      k=0;
    }
    if(timeoutNum>=1000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  return 1;
}

int radio_erase(uint32_t eraselen)
{
  eraseOK=0;
  char buf[50];
  timeoutNum=0;
  sprintf(buf,"|1|1|%d|",eraselen);
  radio_send((uint8_t *)buf,strlen(buf));
  int k=0;
  while(eraseOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)buf,strlen(buf));
      k=0;
    }
    if(timeoutNum>=10000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  return eraseOK;
  
}

int radio_burn(uint8_t* buf,uint8_t len,uint8_t lastpage)
{
  burnOK=0;
  timeoutNum=0;
  if((len==240) && (lastpage==0)){
    packet.head=0x55;
  }else if((len==240) && (lastpage==1)){
    packet.head=0x66;
  }else{
    packet.head=0x66;
  }
  packet.length=len;
  memcpy(packet.payload,buf,len);
  packet.crc=crc32_check(buf,len);
  crc_n++;
  packet.crcn=crc_n;
  radio_send((uint8_t *)&packet,240+9);
  int k=0;
  while(burnOK==0){
    k++;
    if(k>520){
      radio_send((uint8_t *)&packet,240+9);
      k=0;
    }
    if(timeoutNum>=10000){
      timeoutNum=0;
      return timeoutReal;
    }
    if(k>500){
      nrf_delay_ms(1);
      timeoutNum+=1;
    }
  }
  return burnOK;
  
}



void radio_init(RadioConfig *config)
{
  radio_group = config->channel;
  radio_address = config->baseAddress;
  radio_power = config->power;
  radio_frequencyBand = config->frequencyBand;
  radio_speed = config->speed;
  radio_maxSize = config->maxSize;
  m_data_pkt_cb = NULL;
  rxBuf = NULL;
}

void radio_enable()
{
  if (rxBuf==NULL){
    rxBuf=mybuf;
  }
  NRF_RADIO->TXPOWER = (uint32_t)RADIO_TXPOWER_TXPOWER_Pos4dBm<<RADIO_TXPOWER_TXPOWER_Pos;
  NRF_RADIO->FREQUENCY = (uint32_t)radio_frequencyBand;
  NRF_RADIO->MODE = radio_speed;
  NRF_RADIO->BASE0 = radio_address;
  NRF_RADIO->PREFIX0 = (uint32_t)radio_group;
  NRF_RADIO->TXADDRESS = 0;
  NRF_RADIO->RXADDRESSES = 1;
  NRF_RADIO->PCNF0 = (0UL     << RADIO_PCNF0_S1LEN_Pos) |
                     (0UL     << RADIO_PCNF0_S0LEN_Pos) |
                     (8UL << RADIO_PCNF0_LFLEN_Pos);
  NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
                     (1UL      << RADIO_PCNF1_ENDIAN_Pos)  |
                     (4UL  << RADIO_PCNF1_BALEN_Pos)   |
                     (0UL        << RADIO_PCNF1_STATLEN_Pos) |
                     (radio_maxSize               << RADIO_PCNF1_MAXLEN_Pos);
  NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos);
  NRF_RADIO->CRCINIT = 0xFFFFUL;
  NRF_RADIO->CRCPOLY = 0x11021UL;
  NRF_RADIO->DATAWHITEIV = 0x18;
  NRF_RADIO->PACKETPTR = (uint32_t)&mybufa;
  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos;

  NVIC_ClearPendingIRQ(RADIO_IRQn);
  //NVIC_SetPriority(RADIO_IRQn, 3);
  NVIC_EnableIRQ(RADIO_IRQn);

  NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(NRF_RADIO->EVENTS_READY == 0);
  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;
}

void radio_disable()
{
    // Disable interrupts and STOP any ongoing packet reception.
    NVIC_DisableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0);
}

int radio_send_config(FrameBuffer *buffer)
{
  if (buffer == NULL){
    return -1;
  }

  NVIC_ClearPendingIRQ(RADIO_IRQn);
  NVIC_DisableIRQ(RADIO_IRQn);

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(NRF_RADIO->EVENTS_DISABLED == 0);

  NRF_RADIO->PACKETPTR = (uint32_t) buffer;

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_TXEN = 1;
  while (NRF_RADIO->EVENTS_READY == 0);

  NRF_RADIO->TASKS_START = 1;
  NRF_RADIO->EVENTS_END = 0;
  while(NRF_RADIO->EVENTS_END == 0);

  memset(&mybufa,0,sizeof(mybufa));
  NRF_RADIO->PACKETPTR = (uint32_t)&mybufa;

  NRF_RADIO->EVENTS_DISABLED = 0;
  NRF_RADIO->TASKS_DISABLE = 1;
  while(NRF_RADIO->EVENTS_DISABLED == 0);

  NRF_RADIO->EVENTS_READY = 0;
  NRF_RADIO->TASKS_RXEN = 1;
  while(NRF_RADIO->EVENTS_READY == 0);

  NRF_RADIO->EVENTS_END = 0;
  NRF_RADIO->TASKS_START = 1;

  NVIC_ClearPendingIRQ(RADIO_IRQn);
  //NVIC_SetPriority(RADIO_IRQn, 3);
  NVIC_EnableIRQ(RADIO_IRQn);
}

int radio_send(uint8_t *buffer, uint8_t len)
{
  FrameBuffer buf;
  buf.length = len + MICROBIT_RADIO_HEADER_SIZE - 1;
  buf.version = 1;
  buf.group = 0;
  buf.protocol = MICROBIT_RADIO_PROTOCOL_DATAGRAM;
  memcpy(buf.payload, buffer, len);

  return radio_send_config(&buf);
}


void radio_register_callback(radio_callback_t callback_handler)
{
  m_data_pkt_cb = callback_handler;
}

void RADIO_IRQHandler(void)
{
  if(NRF_RADIO->EVENTS_READY)
  {
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_START = 1;
  }
  if(NRF_RADIO->EVENTS_END){
    NRF_RADIO->EVENTS_END = 0;
    if(NRF_RADIO->CRCSTATUS == 1){
      //NRF_LOG_INFO("===333===");
      //NRF_LOG_INFO("%s",mybufa.payload);
      //NRF_LOG_INFO("===333===");
      if(!strncmp(mybufa.payload,cmpMatch,strlen(cmpMatch))){
        NRF_LOG_INFO("===2222===");
        if(m_data_pkt_cb) m_data_pkt_cb(MATCH_RADIO_OK);
      }else if(!strcmp(mybufa.payload,"|1|3|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(CANCEL_MATCH_OK);
      }else if(!strcmp(mybufa.payload,"|1|3|-8|name mismatch|")){
        if(m_data_pkt_cb) m_data_pkt_cb(CANCEL_MATCH_FAIL);
      }else if(!strcmp(mybufa.payload,"|1|1|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(ERASE_RADIO_OK);
        NRF_LOG_INFO("erase ok!!!!");
      }else if(!strcmp(mybufa.payload,"|1|1|-4|erase flash failure|")){
        if(m_data_pkt_cb) m_data_pkt_cb(ERASE_RADIO_FALSE);
      }else if(!strcmp(mybufa.payload,"|1|2|ok|")){
        //NRF_LOG_INFO("burn once ok!!!!");
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_RADIO_ONE_OK);
        //burnOK=1;
      }else if(!strcmp(mybufa.payload,"|1|2|-1|please erase flash|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_RETURN_ERASE_FLASH); 
      }else if(!strcmp(mybufa.payload,"|1|2|-2|please connect|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_RETURN_CONNECT);
      }else if(!strcmp(mybufa.payload,"|1|2|-3|please erase flash and connect|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_RETURN_ERASE_AND_CONNECT);
      }else if(!strcmp(mybufa.payload,"|1|2|-4|unknown error|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_RETURN_UNKOWNERR);
      }else if(!strcmp(mybufa.payload,"|1|4|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_OVER);
      }else if(!strcmp(mybufa.payload,"|1|4|-5|burning failure|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BURN_OVER_ERR);
      }else if(!strcmp(mybufa.payload,"|3|2|app|")){
        if(m_data_pkt_cb) m_data_pkt_cb(STATUS_APP);
      }else if(!strcmp(mybufa.payload,"|3|2|boot|")){
        if(m_data_pkt_cb) m_data_pkt_cb(STATUS_BOOT);
      }else if(!strcmp(mybufa.payload,"|3|1|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(APP_JUMP_BOOT_OK);
      }else if(!strcmp(mybufa.payload,"|3|1|jump failure|")){
        if(m_data_pkt_cb) m_data_pkt_cb(APP_JUMP_BOOT_FAIL);
      }else if(!strcmp(mybufa.payload,"|3|0|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BOOT_JUMP_APP_OK);
      }else if(!strcmp(mybufa.payload,"|3|0|failure|")){
        if(m_data_pkt_cb) m_data_pkt_cb(BOOT_JUMP_APP_FAIL);
      }else if(!strncmp(mybufa.payload,"|2|2|",5)){
        memset(defaultName,0,sizeof(defaultName));
        NRF_LOG_INFO("|2|2|-%d",mybufa.length);
        if(!strcmp(defaultName,"default")){
          NRF_LOG_INFO("11");
        }else{
          NRF_LOG_INFO("22");
        }
        memset(defaultName,0,sizeof(defaultName));
        memcpy(defaultName,&mybufa.payload[5],mybufa.length-9);
    
        if(!strcmp(defaultName,"default")){
          NRF_LOG_INFO("33");
        }else{
          NRF_LOG_INFO("55");
        }

        if(m_data_pkt_cb) m_data_pkt_cb(GET_NAME);
      }else if(!strcmp(mybufa.payload,"|2|1|ok|")){
        if(m_data_pkt_cb) m_data_pkt_cb(SET_NAME);
      }else if(!strncmp(mybufa.payload,"|2|0|",5)){
        if(startscan == 1){
          if(scannum >= 5){
            ;
          }else{
            if(strstr(scan_buf,&mybufa.payload[4]) == NULL){
              memcpy(&scan_buf[strlen(scan_buf)],&mybufa.payload[4],mybufa.length-7);
              memcpy(&scan_buf[strlen(scan_buf)+1],"\r",1);
              scannum+=1;
              NRF_LOG_INFO("1--%s",scan_buf);
              NRF_LOG_INFO("2--%s",&mybufa.payload[4]);
            }else{
              ;
            }
          }
        }else{
          NRF_LOG_INFO("radio_scaned=%s",mybufa.payload);;
        }
      }else{
        NRF_LOG_INFO("radio_recv=%s",mybufa.payload);
      }
    }else{
      NRF_LOG_INFO("crc error");
    }
    NRF_RADIO->TASKS_START = 1;
  }
}

uint32_t crc32_check(uint8_t *p_buf, int len)
{
	uint32_t i, crc;
	crc = 0xFFFFFFFF;
	for (i = 0; i < len; i++)
	  crc = crc32tab[(crc ^ p_buf[i]) & 0xff] ^ (crc >> 8);
	return crc^0xFFFFFFFF;
}