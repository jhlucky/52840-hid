import pywinusb.hid as hid
import time
import os

filepath0="C:/Users/Administrator/Desktop/aaaaaaa/hid/app.bin"
#filepath0="C:/Users/Administrator/Desktop/aaaaaaa/hid/app1.bin"

crc32tab=[
 0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d 
]

cmdStartRadio="start radio"
startRadioOK  ="start ok"
cmdMatch     ='match radio'
matchOK      ='match ok'
matchFalse   ="match false"

cmdScan      ='scan start'
cmdScanStop  ='scan stop'



cmdErase     ='erase radio'
eraseOK      ='erase ok'
cmdBurn      ="burn radio"
cmdJumpBoot  ="jump boot"
burnOneOK    ="burn one ok"
burnReturnEraseFlash  ="please erase flash"
burnReturnConnect = "please connect"
burnReturnEraseAndConnect="please erase flash and connect"
burnReturnUnkown="unknown error"
burnCrcError="crc error"
burnOver="burn over"
burnOverErr="burning failure"
readAddrChannel="read addr and channel"
readAddrChannellen=len(readAddrChannel)
writeAddrChannel="write addr and channel"
cmdGetStatus="get status"
cmdGetStatuslen=len(cmdGetStatus)
cmdGetName="get name"
cmdSetNameLen=len(cmdGetName)
cmdSetName="set name"
cmdSetNameLen=len(cmdSetName)
cmdSetFlashName="set flash name"
cmdSetFlashNameOK=False
cmdAppJumpBoot="app jump boot"
cmdAppJumpBootOK=False
cmdBootJumpApp="boot jump app"
cmdBootJumpAppOK=False
cmdCancelMatch="cancel match"
cmdCancelMatchOK=False



print("readAddrChannellen=%d"%readAddrChannellen)
cmdStartRadioOK = False
cmdMatchRadioOK = False
cmdEraseRadioOK = False
cmdBurnRadioBack  = False
cmdReadAddrChannelOK=False
cmdWriteAddrChannelOK=False
cmdGetStatusOK=False
cmdGetNameOK=False
cmdSetNameOK=False
cmdScanOK=False

burnBackErrNum=0

sector1= [0x0]*65
sector2= [0x0]*65
sector3= [0x0]*65
sector4= [0x0]*65

def crc32_check(pbuf,len):
    crc=0xFFFFFFFF
    for i in range(0,len):
        crc=crc32tab[(crc ^ (pbuf[i])) & 0xff] ^ (crc >> 8)
    return crc^0xFFFFFFFF

def write_addr_and_channel():
    baseaddr=0x87654321  
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(writeAddrChannel)+5
    for i in range(0,len(writeAddrChannel)):
        buffer[i+2]=ord(writeAddrChannel[i])
    buffer[len(writeAddrChannel)+2]=(baseaddr>>24)&0xFF
    buffer[len(writeAddrChannel)+3]=(baseaddr>>16)&0xFF
    buffer[len(writeAddrChannel)+4]=(baseaddr>>8)&0xFF
    buffer[len(writeAddrChannel)+5]=(baseaddr>>0)&0xFF
    buffer[len(writeAddrChannel)+6]=0xFF
    report[0].set_raw_data(buffer)
    report[0].send()

def read_addr_and_channel():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(readAddrChannel)
    for i in range(0,len(readAddrChannel)):
        buffer[i+2]=ord(readAddrChannel[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def set_52840_saved_microbit_name():
    buffer = [0x0]*65
    buffer[0]=0
    sendname="default"
    buffer[1] = len(cmdSetFlashName)+len(sendname) #设置名字的位置
    for i in range(0,len(cmdSetFlashName)):
        buffer[i+2]=ord(cmdSetFlashName[i])
    for i in range(0,len(sendname)):
        buffer[i+2+len(cmdSetFlashName)]=ord(sendname[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def start_radio():
    buffer = [0x0]*65
    buffer[0] = 0
    buffer[1] = len(cmdStartRadio)
    for i in range(0,len(cmdStartRadio)):
        buffer[i+2]=ord(cmdStartRadio[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def matchRadio():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdMatch)
    for i in range(0,len(cmdMatch)):
        buffer[i+2]=ord(cmdMatch[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def get_microbit_status():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdGetStatus)
    for i in range(0,len(cmdGetStatus)):
        buffer[i+2]=ord(cmdGetStatus[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def app_jump_to_boot():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdAppJumpBoot)
    for i in range(0,len(cmdAppJumpBoot)):
        buffer[i+2]=ord(cmdAppJumpBoot[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def boot_jump_to_app():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdBootJumpApp)
    for i in range(0,len(cmdBootJumpApp)):
        buffer[i+2]=ord(cmdBootJumpApp[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def set_microbit_name():
    myname="default"
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdSetName)+len(myname)
    for i in range(0,len(cmdSetName)):
        buffer[i+2]=ord(cmdSetName[i])
    for i in range(0,len(myname)):
        buffer[i+2+len(cmdSetName)]=ord(myname[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def get_microbit_name():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdGetName)
    for i in range(0,len(cmdGetName)):
        buffer[i+2]=ord(cmdGetName[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def erase_microbit():
    cmderaselen=len(cmdErase)
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1]=cmderaselen+4
    for i in range(0,cmderaselen):
        buffer[i+2]=ord(cmdErase[i])
    buffer[cmderaselen+2]=int((filelen>>24)&0xFF)
    buffer[cmderaselen+3]=int((filelen>>16)&0xFF)
    buffer[cmderaselen+4]=int((filelen>>8)&0xFF)
    buffer[cmderaselen+5]=int((filelen)&0xFF)
    report[0].set_raw_data(buffer)
    report[0].send()

def cancel_match():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdCancelMatch)
    for i in range(0,len(cmdCancelMatch)):
        buffer[i+2]=ord(cmdCancelMatch[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def scan():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdScan)
    for i in range(0,len(cmdScan)):
        buffer[i+2]=ord(cmdScan[i])
    report[0].set_raw_data(buffer)
    report[0].send()

def scanstop():
    buffer = [0x0]*65
    buffer[0]=0
    buffer[1] = len(cmdScanStop)
    for i in range(0,len(cmdScanStop)):
        buffer[i+2]=ord(cmdScanStop[i])
    report[0].set_raw_data(buffer)
    report[0].send()




def sample_handler(data):
    global cmdStartRadioOK
    global cmdMatchRadioOK
    global cmdEraseRadioOK
    global cmdBurnRadioBack
    global burnBackErrNum
    global readAddrChannel
    global cmdReadAddrChannelOK
    global cmdWriteAddrChannelOK
    global cmdGetNameOK
    global cmdGetStatusOK
    global cmdSetNameOK
    global cmdSetFlashNameOK
    global cmdSetFlashName
    global cmdAppJumpBootOK
    global cmdBootJumpAppOK
    global cmdCancelMatchOK
    global cmdScanOK
    len0=data[1]
    len1=data[2]
    len2=data[3]
    cmdname=""
    cmdright=""
    cmdreturn=""
    for i in range(0,len0):
        cmdname+=chr(data[i+4])
    for i in range(0,len1):
        cmdright+=chr(data[i+4+len0])
    #if len2>0:
    #    for i in range(0,len2):
    #        cmdreturn+=chr(data[i+4+len0+len1])
    #print(cmdname)
    #print(cmdright)


    if cmdname==writeAddrChannel:
        if cmdright=="ok":
            cmdWriteAddrChannelOK=True
        else:
            write_addr_and_channel()
    elif cmdname==readAddrChannel:
        if cmdright=="ok":
            print("0x%x"%data[len0+len1+4])
            print("0x%x"%data[len0+len1+5])
            print("0x%x"%data[len0+len1+6])
            print("0x%x"%data[len0+len1+7])
            print("0x%x"%data[len0+len1+8])
            cmdReadAddrChannelOK=True
        else:
            read_addr_and_channel()
    elif cmdname==cmdSetFlashName:
        if cmdright=="ok":
            cmdSetFlashNameOK=True
        else:
            set_52840_saved_microbit_name()
    elif cmdname==cmdStartRadio:
        if cmdright=="ok":
            cmdStartRadioOK=True
        else:
            start_radio()
    elif cmdname==cmdMatch:
        if cmdright=="ok":
            cmdMatchRadioOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdMatch = %s"%cmdreturn)
            matchRadio()
    elif cmdname==cmdGetStatus:
        if cmdright=="ok":
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("microbit status = %s"%cmdreturn)
                cmdGetStatusOK=True
            else:
                get_microbit_status()
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdGetStatus = %s"%cmdreturn)
            get_microbit_status()
    elif cmdname==cmdAppJumpBoot:
        if cmdright=="ok":
            cmdAppJumpBootOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdAppJumpBoot = %s"%cmdreturn)
            app_jump_to_boot()
    elif cmdname==cmdBootJumpApp:
        if cmdright=="ok":
            cmdBootJumpAppOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdBootJumpApp = %s"%cmdreturn)
            boot_jump_to_app()
    elif cmdname==cmdSetName:
        if cmdright=="ok":
            cmdSetNameOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdSetName = %s"%cmdreturn)
            app_jump_to_boot()
    elif cmdname==cmdGetName:
        if cmdright=="ok":
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdGetName = %s"%cmdreturn)
            cmdGetNameOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdGetName = %s"%cmdreturn)
            get_microbit_name()
    elif cmdname==cmdErase:
        if cmdright=="ok":
            cmdEraseRadioOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdErase = %s"%cmdreturn)
            erase_microbit()
    elif cmdname=="BN":
        if cmdright=="ok":
            burnBackErrNum=1
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("burn = %s"%cmdreturn)
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("burn err = %s"%cmdreturn)
        cmdBurnRadioBack=True
    elif cmdname==cmdCancelMatch:
        if cmdright=="ok":
            cmdCancelMatchOK=True
        else:
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("cmdCancelMatch err = %s"%cmdreturn)
            cancel_match()
    elif cmdname==cmdScanStop:
        if cmdright=="ok":
            cmdScanOK=True
            if len2>0:
                for i in range(0,len2):
                    cmdreturn+=chr(data[i+4+len0+len1])
                print("scan = %s\r\n"%cmdreturn)
        else:
            print("scan false\r\n")
            
            


            
        
def burn_one_sector():
    global sector1
    global sector2
    global sector3
    global sector4
    global cmdBurnRadioBack
    global burnBackErrNum
    report[0].set_raw_data(sector1)
    report[0].send()
    report[0].set_raw_data(sector2)
    report[0].send()
    report[0].set_raw_data(sector3)
    report[0].send()
    report[0].set_raw_data(sector4)
    report[0].send()
    while not cmdBurnRadioBack:
        pass
    cmdBurnRadioBack=False
    if burnBackErrNum==1:
        sector1= [0x0]*65
        sector2= [0x0]*65
        sector3= [0x0]*65
        sector4= [0x0]*65
    else:
        time.sleep(0.01)
        burn_one_sector()
    burnBackErrNum=0

def lastOne():
    global sector1
    global sector2
    global sector3
    global sector4
    global cmdBurnRadioBack
    global burnBackErrNum
    report[0].set_raw_data(sector1)
    report[0].send()
    while not cmdBurnRadioBack:
        pass
    cmdBurnRadioBack=False
    if burnBackErrNum==1:
        sector1= [0x0]*65
    else:
        time.sleep(0.01)
        lastOne()
    burnBackErrNum=0

def lastTwo():
    global sector1
    global sector2
    global sector3
    global sector4
    global cmdBurnRadioBack
    global burnBackErrNum
    report[0].set_raw_data(sector1)
    report[0].send()
    report[0].set_raw_data(sector2)
    report[0].send()
    while not cmdBurnRadioBack:
        pass
    cmdBurnRadioBack=False
    if burnBackErrNum==1:
        sector1= [0x0]*65
        sector2= [0x0]*65
    else:
        time.sleep(0.01)
        lastTwo()
    burnBackErrNum=0

def lastThree():
    global sector1
    global sector2
    global sector3
    global sector4
    global cmdBurnRadioBack
    global burnBackErrNum
    report[0].set_raw_data(sector1)
    report[0].send()
    report[0].set_raw_data(sector2)
    report[0].send()
    report[0].set_raw_data(sector3)
    report[0].send()
    while not cmdBurnRadioBack:
        pass
    cmdBurnRadioBack=False
    if burnBackErrNum==1:
        sector1= [0x0]*65
        sector2= [0x0]*65
        sector3= [0x0]*65
    else:
        time.sleep(0.01)
        lastThree()
    burnBackErrNum=0

def lastFour():
    global sector1
    global sector2
    global sector3
    global sector4
    global cmdBurnRadioBack
    global burnBackErrNum
    report[0].set_raw_data(sector1)
    report[0].send()
    report[0].set_raw_data(sector2)
    report[0].send()
    report[0].set_raw_data(sector3)
    report[0].send()
    report[0].set_raw_data(sector4)
    report[0].send()
    while not cmdBurnRadioBack:
        pass
    cmdBurnRadioBack=False
    if burnBackErrNum==1:
        sector1= [0x0]*65
        sector2= [0x0]*65
        sector3= [0x0]*65
        sector4= [0x0]*65
    else:
        time.sleep(0.01)
        lastFour()
    burnBackErrNum=0

#config hid
print("=====config hid======start")
filter = hid.HidDeviceFilter(vendor_id = 0x1915, product_id = 0x521a)
hid_device = filter.get_devices()
print(hid_device)
device = hid_device[0]
device.open()
print(hid_device)
target_usage = hid.get_full_usage_id(0x00, 0x3f)
device.set_raw_data_handler(sample_handler)#读
print("target_usage=%d"%target_usage )
report = device.find_output_reports()
print(report)
print(report[0])
print("=====over======\r\n\r\n")

#设置地址和通道
print("=====设置地址和通道======start")
cmdWriteAddrChannelOK=False
write_addr_and_channel()
while not cmdWriteAddrChannelOK:
    pass
print("=====over======\r\n\r\n")
    
#读取地址和通道
print("=====读取地址和通道======start")
cmdReadAddrChannelOK = False
read_addr_and_channel()
while not cmdReadAddrChannelOK:
    pass
print("=====over======\r\n\r\n")

#hid设置microbit名称（存在52840flash中的）
print("=====hid设置microbit名称======")
cmdSetFlashNameOK = False
set_52840_saved_microbit_name()
while not cmdSetFlashNameOK:
    pass
print("=====over======\r\n\r\n")

#start
print("=====start=====")
cmdStartRadioOK = False
start_radio()
while not cmdStartRadioOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)

print("scan.....start.......")
cmdScanOK=False
scan()
time.sleep(3)


print("scan.....stop.......")

scanstop()
print("over")
while True:
    pass


   
#match
print("=====match=====")
matchRadio()
while not cmdMatchRadioOK:
    pass  
print("=====over======\r\n\r\n")
#time.sleep(3)

#获取microbit运行状态
print("=====获取microbit运行状态=======")
cmdGetStatusOK = False
get_microbit_status()
while not cmdGetStatusOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)

#从app跳转到bootloader
print("=====从app跳转到bootloader=======")
cmdAppJumpBootOK = False
app_jump_to_boot()
while not cmdAppJumpBootOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)





#设置microbit名称
print("=====设置microbit名称=======")
cmdSetNameOK = False
set_microbit_name()
while not cmdSetNameOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)

#获取microbit名称
print("=====获取microbit名称=======")
cmdGetNameOK = False
get_microbit_name()
while not cmdGetNameOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)


myfile=open(filepath0,"rb")
mymsg=myfile.read()
myfile.close()
filelen=len(mymsg)

#erase
print("=====erase=======")
cmdEraseRadioOK = False
erase_microbit()
while not cmdEraseRadioOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)


    
#burn
print("=====start burn=====")
#file=open(filepath0,'rb')
filepage=0
allpage=int(filelen/240)
lastpagelen=filelen%240
for j in range(0,allpage):
    msg=mymsg[j*240:(j+1)*240]
    if lastpagelen==0 and i==(allpage-1):
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=63
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,60):
            sector1[i+4]=msg[i]
        sector1[60+4]=0xFF

        sector2[0x0]*65
        sector2[0]=0
        sector2[1]=63
        sector2[2]=0x42#"B"
        sector2[3]=0x4E#"N"
        for i in range(0,60):
            sector2[i+4]=msg[i+60]
        sector2[60+4]=0xFF

        sector3[0x0]*65
        sector3[0]=0
        sector3[1]=63
        sector3[2]=0x42#"B"
        sector3[3]=0x4E#"N"
        for i in range(0,60):
            sector3[i+4]=msg[i+120]
        sector3[60+4]=0xFF

        sector4[0x0]*65
        sector4[0]=0
        sector4[1]=191        #63->00111111  10111111->191
        sector4[2]=0x42#"B"
        sector4[3]=0x4E#"N"
        for i in range(0,60):
            sector4[i+4]=msg[i+180]
        sector4[60+4]=crc32_check(msg,240)&0xFF

        burn_one_sector()
    else:
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=63
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,60):
            sector1[i+4]=msg[i]
        sector1[60+4]=0xFF

        sector2[0x0]*65
        sector2[0]=0
        sector2[1]=63
        sector2[2]=0x42#"B"
        sector2[3]=0x4E#"N"
        for i in range(0,60):
            sector2[i+4]=msg[i+60]
        sector2[60+4]=0xFF

        sector3[0x0]*65
        sector3[0]=0
        sector3[1]=63
        sector3[2]=0x42#"B"
        sector3[3]=0x4E#"N"
        for i in range(0,60):
            sector3[i+4]=msg[i+120]
        sector3[60+4]=0xFF

        sector4[0x0]*65
        sector4[0]=0
        sector4[1]=63
        sector4[2]=0x42#"B"
        sector4[3]=0x4E#"N"
        for i in range(0,60):
            sector4[i+4]=msg[i+180]
        sector4[60+4]=crc32_check(msg,240)&0xFF

        burn_one_sector()
if lastpagelen>0:
    msg=mymsg[allpage*240:]
    if int(lastpagelen/60)==0:
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=lastpagelen+3
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,lastpagelen):
            sector1[i+4]=msg[i]
        sector1[lastpagelen+4]=crc32_check(msg,lastpagelen)&0xFF
        lastOne()
    elif int(lastpagelen/60)==1:
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=63
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,60):
            sector1[i+4]=msg[i]
        sector1[60+4]=0xFF

        sector2[0x0]*65
        sector2[0]=0
        sector2[1]=lastpagelen%60+3
        sector2[2]=0x42#"B"
        sector2[3]=0x4E#"N"
        for i in range(0,lastpagelen%60):
            sector2[i+4]=msg[i+60]
        sector2[lastpagelen%60+4]=crc32_check(msg,lastpagelen)&0xFF
        lastTwo()
    elif int(lastpagelen/60)==2:
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=63
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,60):
            sector1[i+4]=msg[i]
        sector1[60+4]=0xFF

        sector2[0x0]*65
        sector2[0]=0
        sector2[1]=63
        sector2[2]=0x42#"B"
        sector2[3]=0x4E#"N"
        for i in range(0,60):
            sector2[i+4]=msg[i+60]
        sector2[60+4]=0xFF

        sector3[0x0]*65
        sector3[0]=0
        sector3[1]=lastpagelen%60+3
        sector3[2]=0x42#"B"
        sector3[3]=0x4E#"N"
        for i in range(0,lastpagelen%60):
            sector3[i+4]=msg[i+120]
        sector3[lastpagelen%60+4]=crc32_check(msg,lastpagelen)&0xFF
        lastThree()
    else:
        sector1[0x0]*65
        sector1[0]=0
        sector1[1]=63
        sector1[2]=0x42#"B"
        sector1[3]=0x4E#"N"
        for i in range(0,60):
            sector1[i+4]=msg[i]
        sector1[60+4]=0xFF

        sector2[0x0]*65
        sector2[0]=0
        sector2[1]=63
        sector2[2]=0x42#"B"
        sector2[3]=0x4E#"N"
        for i in range(0,60):
            sector2[i+4]=msg[i+60]
        sector2[60+4]=0xFF

        sector3[0x0]*65
        sector3[0]=0
        sector3[1]=63
        sector3[2]=0x42#"B"
        sector3[3]=0x4E#"N"
        for i in range(0,60):
            sector3[i+4]=msg[i+120]
        sector3[60+4]=0xFF

        sector4[0x0]*65
        sector4[0]=0
        sector4[1]=lastpagelen%60+3
        sector4[2]=0x42#"B"
        sector4[3]=0x4E#"N"
        for i in range(0,lastpagelen%60):
            sector4[i+4]=msg[i+180]
        sector4[lastpagelen%60+4]=crc32_check(msg,lastpagelen)&0xFF
        lastFour()




#从app跳转到bootloader
time.sleep(3)
print("=====从app跳转到bootloader=======")
cmdAppJumpBootOK = False
app_jump_to_boot()
while not cmdAppJumpBootOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)



time.sleep(3)
#从bootloader跳转到app运行   
cmdBootJumpAppOK=False
boot_jump_to_app()
while not cmdBootJumpAppOK:
    pass


time.sleep(3)
#从app跳转到bootloader
print("=====从app跳转到bootloader=======")
cmdAppJumpBootOK = False
app_jump_to_boot()
while not cmdAppJumpBootOK:
    pass
print("=====over======\r\n\r\n")


##取消配对
print("=====取消配对=====")
cmdCancelMatchOK = False
cancel_match()
while not cmdCancelMatchOK:
    pass
print("=====over======\r\n\r\n")
#time.sleep(3)

'''
#jump boot
print("jump boot")
buffer = [0x0]*65
buffer[0]=0
buffer[1] = len(cmdJumpBoot)
for i in range(0,len(cmdJumpBoot)):
    buffer[i+2]=ord(cmdJumpBoot[i])
report[0].set_raw_data(buffer)
report[0].send()
'''
device.close()
print('over')
