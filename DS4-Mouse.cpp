#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <pthread.h>
#include <memory.h>

char buffer[256] = {0}, 
    path[29] = "sudo chmod a+rw /dev/hidraw0" ,  
    command[25] = "sudo xdotool mousedown 0",
    pre_buffer= 0,
    pre_click=0;
int fd =0,
    hold_counter=0,
    x_pre_data = 0,
    x_last_change =0,
    x = 0,
    x_boundary = 0,
    y_pre_data = 0,
    y_last_change =0,    
    y =0,
    y_boundary = 0;
unsigned char output[9] = {0};
bool status = false,
    holding = false;
pthread_t threads;
Display* display ;

void* Light(void*){
    while(true){
        if(status){
            for (int i = 255; i >= 0; i--)
            {
                output[7] = i;
                output[8] = i;  
                flock(fd , LOCK_EX);
                write(fd , output , 9);
                flock(fd ,LOCK_UN);         
                usleep(10000);
            }
            for (int i = 0; i < 256; i++)
            {
                output[7] = i;
                output[8] = i;    
                flock(fd , LOCK_EX);
                write(fd , output , 9);
                flock(fd ,LOCK_UN);        
                usleep(10000);
            }
        }
        else{
            output[6] = 0xff;
            output[7] = 0xff;
            output[8] = 0xff;
            flock(fd , LOCK_EX);
            write(fd , output , 9);
            flock(fd ,LOCK_UN);        
            usleep(500000);            
        }
    }
}
int new_x_change(int new_data){
    if(!(new_data < 0 && x_last_change > 0 && x_pre_data > 0)){
        if(x-new_data/25 > x_boundary || x-new_data/25 < 0){
            x_last_change = new_data - x_pre_data;
            x_pre_data = new_data;
            return 0;
        }
        else{
            x_last_change = new_data - x_pre_data;
            x_pre_data = new_data;
            x -= new_data /25;
            return new_data /-25;            
        }
    } 
    else    return 0;
}
int new_y_change(int new_data){
    if(!(new_data < 0 && y_last_change > 0 && y_pre_data > 0)){
        if(y-new_data/70 > y_boundary || y-new_data/70 < 0){
            y_last_change = new_data - y_pre_data;
            y_pre_data = new_data;
            return 0;
        }
        else{
            y_last_change = new_data - y_pre_data;
            y_pre_data = new_data;
            y -= new_data /70;
            return new_data / -70;            
        }
    } 
    else    return 0;
}
void hold(int button){
    snprintf(&command[13] , 12 ,"mousedown %d" , button);
    system(command);
}
void press(int button){
    snprintf(&command[13] , 12 ,"click %d\0\0\0\0" , button);
    system(command);
}
void release(int button){
    snprintf(&command[13] , 12 ,"mouseup %d\0\0" , button);
    system(command);
}
void initial_pos(){
    XEvent first;
    XQueryPointer (display, RootWindow(display, DefaultScreen(display)),
        &first.xbutton.root, &first.xbutton.window,
        &first.xbutton.x_root, &first.xbutton.y_root,
        &first.xbutton.x, &first.xbutton.y,
        &first.xbutton.state);  
    x = first.xbutton.x;
    y = first.xbutton.y;  
}
void set_boundaries(){
    x_boundary = XDisplayWidth(display , 0);
    y_boundary = XDisplayHeight(display , 0);
}
int main(int argc, char** argv){
    struct stat file;
    
    for(int i=1;;i++){
        if(stat(&path[16] , &file) == 0){
            system(path);
            fd = open(&path[16] ,  O_RDWR | O_NONBLOCK);
            ioctl(fd , HIDIOCGRAWNAME(256) , buffer);
            std::string name = "Sony Interactive Entertainment Wireless Controller";
            if(name.compare(buffer) == 0){ 
                memset(buffer, 0, 256);
                break;
            }
            else{
                close(fd);
                path[12] = '-';
                system(path);
                snprintf(&path[27] , 2 , "%d" , i);
                path[12] = '+';
            }
        }
        else if(i < 6)
            path[27] = i;
        else{
            printf("DS4 device not found.");
            return -1;            
        }
    }
    output[0] = 0x5;
    output[1] = 0x2;
    output[6] = 0xff;
    output[7] = 0xff;
    output[8] = 0xff;
    write(fd , output , 9);
    while(true){
        usleep(4000);
        read(fd , buffer , 64);
        if(buffer[7] & 1){
            pthread_create(&threads , NULL , Light , NULL);
            break;  
        }
    }
    display =  XOpenDisplay(NULL);
    initial_pos();
    set_boundaries();
    while(true){
        usleep(4000);
        flock(fd , LOCK_EX);
        read(fd , buffer , 64);
        flock(fd,LOCK_UN);
        if(buffer[7] & 1)   if(pre_buffer == 0) status = !status;
        if(status){
            if((buffer[14] << 8 | buffer[13]) > 30 ||(buffer[14] << 8 | buffer[13]) < -30)      XWarpPointer(display, None, None, 0, 0, 0, 0, 0, new_y_change(buffer[14] << 8 | buffer[13]));
            if((buffer[16] << 8 | buffer[15]) > 30 ||(buffer[16] << 8 | buffer[15]) < -50)      XWarpPointer(display, None, None, 0, 0, 0, 0, new_x_change(buffer[16] << 8 | buffer[15]), 0);
            XFlush (display);
            if(buffer[5] >> 5 & 1){
                if(hold_counter < 60)
                    hold_counter++;
                else if(!holding){
                    holding = true;
                    hold(Button1);
                }
            }      
            else{
                if((pre_click >> 5 & 1) == 1){
                    if(holding){
                        release(Button1);
                        holding =false;
                    }
                    else if(hold_counter > 20)
                        press(Button1);
                    hold_counter = 0;
                }
            }
            if(buffer[5] >> 6 & 1){
                if(hold_counter < 60)
                    hold_counter++;
                else if(!holding){
                    holding = true;
                    hold(Button3);
                }
            }      
            else{
                if((pre_click >> 6 & 1) == 1){
                    if(holding){
                        release(Button3);
                        holding =false;
                    }
                    else if(hold_counter > 20)
                        press(Button3);
                    hold_counter = 0;
                }
            }
        }
        pre_buffer = buffer[7] & 1; 
        pre_click = buffer[5];
    }
}