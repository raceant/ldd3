/*************************************

NAME:Rs485.c
COPYRIGHT:www.ribeto.com

*************************************/


#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "lordweb.h"

/****************************************
open_port:open a serial port
*****************************************/
int open_port(int port_no)
{
    int fd;
    char *dev[] = {"/dev/ribeto_serial0","/dev/ribeto_serial1","/dev/ribeto_serial2"};

    if ((port_no < 0) || (port_no > 2))
        return -1;
    //fd = open(dev[port_no],O_RDWR|O_NOCTTY|O_NDELAY);
    fd = open(dev[port_no],O_RDWR|O_NDELAY);
    //set this serial port in non-block mode
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl failure!\n");
        close(fd);
        return -1;
    }
    //if (0 == isatty(STDIN_FILENO)) {
    //    perror("Is not a terminal device!\n");
    //    close(fd);
    //    return -1;
    //}
    return fd;
}

//set parameters of the serial 
int set_opt(int fd, int nBits, int nSpeed, char nEvent, int nStop)
{
    struct termios term_info;
    speed_t speed;

    if (tcgetattr(fd, &term_info) != 0) {
        perror("Setup serial !\n");
        return -1;
    }

    //Set charactor size.
    term_info.c_cflag |= CLOCAL | CREAD;
    term_info.c_cflag &= ~CSIZE;
    switch(nBits) {
    case 5:
        term_info.c_cflag |= CS5;
        break;
    case 6:
        term_info.c_cflag |= CS6;
        break;
    case 7:
        term_info.c_cflag |= CS7;
        break;
    case 8:
        term_info.c_cflag |= CS8;
        break;
    default:
        return -1;
        break;
    }
    //Set parity checking
    if ((nEvent == 'O') || (nEvent == 'o')) {
        term_info.c_cflag |= PARENB;
        term_info.c_cflag |= PARODD;
        term_info.c_iflag |= INPCK;
    }
    else if ((nEvent == 'E') || (nEvent == 'e')) {
        term_info.c_cflag |= PARENB;
        term_info.c_cflag &= ~PARODD;
        term_info.c_iflag |= INPCK;
    }
    if ((nEvent == 'N') || (nEvent == 'n')) {
        term_info.c_cflag &= ~PARENB;
        term_info.c_iflag &= ~INPCK;
    }
    //Set baud drate
    switch(nSpeed) {
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        return -1;
        break;
    }
    cfsetispeed(&term_info,speed);
    cfsetospeed(&term_info,speed);

    //stop bit
    if (1 == nStop)
        term_info.c_cflag &= ~CSTOPB;
    else if (2 == nStop)
        term_info.c_cflag |= CSTOPB;
    else
        return -1;

    //Disable hardware flow control
    term_info.c_cflag &= ~CRTSCTS;
    //Disable software flow control
    term_info.c_iflag &=~(IXON | IXOFF | IXANY);
    //Rx in raw data mode
    term_info.c_lflag &=~(ICANON | ECHO | ECHOE | ISIG);
    //Disable TX process.(Enable raw tx mode)
    term_info.c_oflag &=~OPOST;

    term_info.c_cc[VTIME] = 0;
    term_info.c_cc[VMIN] = 0;
	term_info.c_iflag |= IGNPAR|ICRNL;
	//term_info.c_oflag |= OPOST; 
	//term_info.c_iflag &= ~(IXON|IXOFF|IXANY); 
    tcflush(fd,TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &term_info) != 0)
        return -1;
}

