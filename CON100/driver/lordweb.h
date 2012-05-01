#ifndef _LORDWEB_H_
#define _LORDWEB_H_

#define LORD_SEEK_SET       (1)  //reset write pointer(for background buff) to top left
#define LORD_END_WRITE      (2)  //background buff is prepared, swap with foreground buff
#define LORD_WRITE          (3)  //write buff(background)
#define LORD_IO_WRITE       (4)
#define LORD_VEHICLE_STAT   (5)  //inquire the vehicle presentation status

#define LORD_UP_ON      (0x00000001)
#define LORD_UP_OFF     (0x00000002)
#define LORD_DOWN_ON    (0x00000004)
#define LORD_DOWN_OFF   (0x00000008)
#define LORD_RS485_TX   (0x00000010)
#define LORD_RS485_RX   (0x00000020)


#endif
