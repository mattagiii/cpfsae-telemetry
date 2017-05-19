#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "acquire.h"

int soc, read_can_port = 1, update = 1, all_channels_valid = 0;

channel channels[] = {
   { .offset = 0,  .nBytes = 2, .scaling = 1,    .precision = 0, .units = "RPM", .name = "RPM" },
   { .offset = 2,  .nBytes = 2, .scaling = 0.1,  .precision = 1, .units = "%",   .name = "ThrottlePosition" },
   { .offset = 4,  .nBytes = 2, .scaling = 0.1,  .precision = 1, .units = "kPa", .name = "ManifoldPressure" },
   { .offset = 44, .nBytes = 2, .scaling = 0.01, .precision = 2, .units = "V",   .name = "BatteryVoltage" }
};

int open_port(const char *port) {
   struct ifreq ifr;
   struct sockaddr_can addr;

   // open socket
   soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
   if(soc < 0) { return (-1); }

   addr.can_family = AF_CAN;
   strcpy(ifr.ifr_name, port);

   // get the interface index for port (can0)
   // the ioctl is the only way to get the actual interface number for a
   // networking interface (could be any number), and it requires passing
   // ANY open socket (though SIOCGIFINDEX will not use this), as well as the
   // ifreq structure containing the interface string name in ifr.ifr_name.
   // the real interface number is then placed into ifr.ifr_ifindex.
   if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) { return (-1); }
   addr.can_ifindex = ifr.ifr_ifindex;
   fcntl(soc, F_SETFL, O_NONBLOCK);
   if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) { return (-1); }

   return 0;
}

int send_port(struct can_frame *frame) {
   int retval;
   retval = write(soc, frame, sizeof(struct can_frame));
   if (retval != sizeof(struct can_frame)) {
      return (-1);
   }
   else {
      return (0);
   }
}

void * read_port(void *arg) {
   struct can_frame frame_rd;
   struct timeval timeout = {1, 0};
   // 1000000 too slow for bus - 17% CPU
   // 100000  seems good       - 22% CPU
   // 10000   to be safe       - 36% CPU
   struct timespec sleep_timeout = {0, 100000};
   fd_set readSet;
   int recvbytes = 0, i, byteOffset = -1, nextOffset, ch_idx = 0;
   int numChannels = sizeof(channels) / sizeof(channel);

   while (read_can_port) {
      FD_ZERO(&readSet);
      FD_SET(soc, &readSet);

      nanosleep(&sleep_timeout, NULL);

      if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0) {
         //printf("0"); // for checking that polling speed is fast enough
         if (!read_can_port) { break; }

         if (FD_ISSET(soc, &readSet)) {
            //printf("1"); // for checking that polling speed is fast enough
            recvbytes = read(soc, &frame_rd, sizeof(struct can_frame));

            if (recvbytes) {

               if (frame_rd.can_id == M400_ID) {
                  nextOffset = channels[ch_idx].offset;

                  // iterate through bytes of current frame
                  for (i = 0; i < frame_rd.can_dlc; i++) {

                     if (byteOffset == nextOffset) {
                        memcpy(&(channels[ch_idx].value), &(frame_rd.data[i]), channels[ch_idx].nBytes);
                        if (ch_idx == numChannels-1)
                           ch_idx = 0;
                        else
                           ch_idx++;

                        // tell update_file that it may begin writing
                        if (nextOffset == channels[numChannels].offset) {
                           all_channels_valid = 1;
                        }

                        nextOffset = channels[ch_idx].offset;
                     }
                     byteOffset++;
                  }

                  if (frame_rd.data[4] == 0xFC && \
                      frame_rd.data[5] == 0xFB && \
                      frame_rd.data[6] == 0xFA) {
                     byteOffset = 0;
                  }
               }
            }
         }
      }
   }
   pthread_exit(NULL);
}

int close_port() {
   close(soc);
   return 0;
}

void * update_file(void *arg) {
   struct timespec timeout = {1, 0};
   char ch_val_str[32];
   int i, e;
   int numChannels = sizeof(channels) / sizeof(channel);
   float channel_val;
   FILE *M400_channels_fp = fopen("/root/cpfsae-telemetry/M400_channels.txt", "w");

   if (M400_channels_fp == NULL) {
      perror("Failed to open file");
      pthread_exit(NULL);
   }

   while(update) {
      if (all_channels_valid) {
         e = nanosleep(&timeout, NULL);
         if (e) {
            printf("ERROR; nanosleep() interrupted. code: %d\n", e);
            pthread_exit(NULL);
         }

         for (i = 0; i < numChannels; i++) {
            channel_val = (float)((channels[i].value[0] << 8) + channels[i].value[1]) * channels[i].scaling;
            snprintf(ch_val_str, 32, "%.*f %s", channels[i].precision, channel_val, channels[i].units);
            fprintf(M400_channels_fp, "%s: %s\n", channels[i].name, ch_val_str);
         }
         rewind(M400_channels_fp);
      }
   }
   fclose(M400_channels_fp);
   pthread_exit(NULL);
}

int main(void) {
   int rc1, rc2;
   pthread_t tid1, tid2;

   open_port("can0");

   rc1 = pthread_create(&tid1, NULL, read_port, &rc1);
   if (rc1) {
      printf("ERROR; return code from pthread_create() is %d\n", rc1);
      exit(-1);
   }

   rc2 = pthread_create(&tid2, NULL, update_file, &rc2);
   if (rc2) {
      printf("ERROR; return code from pthread_create() is %d\n", rc2);
      exit(-1);
   }

   // exit properly so that worker threads continue to run
   pthread_exit(NULL);

   return 0;
}
