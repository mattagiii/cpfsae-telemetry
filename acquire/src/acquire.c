// acquire.c
// Reads CAN frames from vehicle bus, filtering based on ID and updating values
// in the channel array. Channel details are stored as JSON for reading by
// ../serve/serve.js, which updates the channel values on a live webpage.
//
// Copyright (C) 2017 Matt Rounds
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <endian.h>
#include <errno.h>
#include <linux/can.h>
#include <net/if.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "acquire.h"

#define M400_ID 0x05F0
#define BITS_PER_BYTE 8
#define MAX_CH_STR 32
#define MS100_TO_NS 100000000

int s; // descriptor for the CAN socket
uint8_t new_data = 0; // flag indicating data ready to be written to the file

// An array of "channels", which each represent a measured value from one of
// the devices on the vehicle network
channel channels[] = {
   { .offset = 0,  .n_bytes = 2, .scaling = 1,    .precision = 0, .units = "RPM", .name = "RPM" },
   { .offset = 2,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "%",   .name = "ThrottlePosition" },
   { .offset = 4,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kPa", .name = "ManifoldPressure" },
   { .offset = 6,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "C", .name = "AirTemperature" },
   { .offset = 8,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "C", .name = "EngineTemperature" },
   { .offset = 10,  .n_bytes = 2, .scaling = 0.001,  .precision = 3, .units = "La", .name = "Lambda" },
   { .offset = 22,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "C", .name = "OilTemperature" },
   { .offset = 24,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kPa", .name = "OilPressure" },
   { .offset = 44,  .n_bytes = 2, .scaling = 0.01,  .precision = 2, .units = "V", .name = "BatteryVoltage" },
   { .offset = 46,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "C", .name = "ECUTemperature" },
   { .offset = 48,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "GroundSpeedLeft" },
   { .offset = 50,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "GroundSpeedRight" },
   { .offset = 52,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "DriveSpeedLeft" },
   { .offset = 54,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "DriveSpeedRight" },
   { .offset = 56,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "DriveSpeed" },
   { .offset = 58,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "GroundSpeed" },
   { .offset = 60,  .n_bytes = 2, .scaling = 0.1,  .precision = 1, .units = "kmh", .name = "Slip" },
   { .offset = 88, .n_bytes = 2, .scaling = 0.01, .precision = 2, .units = "",   .name = "FuelUsed" }
};

// Opens a CAN socket and binds it to the network interface described by "soc"
int open_soc(const char *soc) {
   char b;
   struct sockaddr_can addr;
   struct ifreq ifr;

   errno = 0;
   s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
   if (s < 0) {
      printf("Error creating socket: %s\n", strerror(errno));
      exit(-1);
   }

   strcpy(ifr.ifr_name, soc);
   // obtain the interface index based on the soc string
   ioctl(s, SIOCGIFINDEX, &ifr);

   addr.can_family = AF_CAN;
   addr.can_ifindex = ifr.ifr_ifindex;
   //addr.can_ifindex = 0; // use only to listen on ALL interfaces

   errno = 0;
   b = bind(s, (struct sockaddr *)&addr, sizeof(addr));
   if (b != 0) {
      printf("Error binding: %s\n", strerror(errno));
      exit(-1);
   }
   return 0;
}

// Stub for future bidirectional communication. Currently no frames are sent.
uint8_t send_soc(struct can_frame *frame) {
   int retval;
   retval = write(s, frame, sizeof(struct can_frame));
   if (retval != sizeof(struct can_frame))
      return -1;
   else
      return 0;
}

// read_soc() makes use of the can_frame structure (can.h) and the established
// socket to read data frames from the CAN controller. read() blocks until
// there is data, so this thread will wait patiently without consuming CPU
// cycles. Frames are filtered by CAN ID, though currently listening to only
// one device (M400 engine control unit). The channel array is updated
// accordingly.
void * read_soc(void *arg) {
   struct can_frame frame_rd;
   uint16_t recvbytes = 0, i, byteOffset = 0, nextOffset, ch_idx = 0;
   uint32_t numChannels = sizeof(channels) / sizeof(channel);
   int64_t beData = 0; // big-endian version of data value

   // loop until this thread is killed
   while (1) {
      errno = 0;
      recvbytes = read(s, &frame_rd, sizeof(struct can_frame));
      if (recvbytes < 0)
         printf("Error reading: %s\n", strerror(errno));

      // this if clause supports the MoTeC M400 ECU CAN protocol, specifically
      // the following:
      // - all channels rotate on the same ID 0x5F0 (see MoTeC docs)
      // - 1-8 byte data segment length
      // - 1-8 bytes per channel
      // - any byte-aligned offset for a channel
      // - signed values
      // not directly supported (can be processed elsewhere)
      // - sub-byte channels (e.g. bit flags)
      // - non-numeric data (channels are stored as signed int64)
      // if bit flags are needed, metadata for signed/unsigned should be added
      // to the channel struct to conditionally avoid the int64 cast below
      if (frame_rd.can_id == M400_ID) {
         nextOffset = channels[ch_idx].offset;
         // iterate through bytes of current frame
         for (i = 0; i < frame_rd.can_dlc; i++) {
            // nextOffset will always be the offset of the next channel, so we
            // only copy data if we have arrived at that byteOffset
            if (byteOffset == nextOffset) {
               // grab this channel's bytes from data
               memcpy(&beData, &(frame_rd.data[i]), channels[ch_idx].n_bytes);
               // swap to little-endian and shift. note the cast to signed so
               // that negative values are preserved during the right-shift
               channels[ch_idx].value = (int64_t)be64toh(beData) >> (BITS_PER_BYTE*(CAN_MAX_DLEN - channels[ch_idx].n_bytes));

               if (ch_idx == numChannels-1)
                  ch_idx = 0;
               else
                  ch_idx++;

               nextOffset = channels[ch_idx].offset;
               // allow update_file to write the newly received data
               new_data = 1;
            }
            byteOffset++;
         }

         // the M400 CAN protocol adds these bytes at the end of its frame
         // cycle, so although the number of frames is predictable, we can
         // reset our offsets/indices here for the best reliability
         if (frame_rd.data[4] == 0xFC && \
             frame_rd.data[5] == 0xFB && \
             frame_rd.data[6] == 0xFA) {
            byteOffset = 0;
            ch_idx = 0;
            nextOffset = channels[ch_idx].offset;
         }
      } // frame_rd.can_id == M400_ID
      // else if clauses for other devices and CAN IDs go here
   } // while(1)
   pthread_exit(NULL);
}

// Closes the CAN socket
int close_soc() {
   close(s);
   return 0;
}

// Stores the data in the channel array as a JSON file
void * update_file(void *arg) {
   struct timespec timeout = {0, MS100_TO_NS}; // (s, ns) (100ms)
   char ch_val_str[MAX_CH_STR];
   int16_t i, e;
   uint16_t numChannels = sizeof(channels) / sizeof(channel);
   float channel_val;
   FILE *telem_channels_fp;
   telem_channels_fp = fopen("telemChannels.json", "w");

   if (telem_channels_fp == NULL) {
      perror("Failed to open file");
      pthread_exit(NULL);
   }

   // loop until this thread is killed
   while(1) {
      // first check that there is new channel data to be written to the file
      if (new_data) {
         // open the file each time so as to overwrite instead of append.
         telem_channels_fp = fopen("telemChannels.json", "w");

         // output JSON formatting (yes, there are libraries for this)
         fprintf(telem_channels_fp, "{\"channels\" : [\n");
         for (i = 0; i < numChannels; i++) {
            channel_val = (float)(channels[i].value) * channels[i].scaling;
            snprintf(ch_val_str, MAX_CH_STR, "%.*f %s", channels[i].precision, channel_val, channels[i].units);
            fprintf(telem_channels_fp, "   {\n      \"name\" : \"%s\",\n      \"value\" : \"%s\"\n   }", channels[i].name, ch_val_str);
            if (i < numChannels-1) fprintf(telem_channels_fp, ",");
            fprintf(telem_channels_fp, "\n");
         }
         fprintf(telem_channels_fp, "]}\n");
         fclose(telem_channels_fp);

         // reset new_data now that the file is up to date
         new_data = 0;
      }

      // nanosleep is used here to limit the data update frequency. whereas the
      // read() call in read_soc will block execution when there is no bus
      // data, this loop (thus the thread) will spin-wait and eat up CPU if not
      // inhibited. while multi-thread orchestration is often achieved through
      // pthread conditions, signals, etc., sleeping is actually suitable for
      // this situation because it delivers a consistent periodic update rate
      // regardless of presence or quantity of data on the CAN bus.
      e = nanosleep(&timeout, NULL);
      if (e) {
         printf("ERROR; nanosleep() interrupted. code: %d\n", e);
         pthread_exit(NULL);
      }
   }
   pthread_exit(NULL);
}

int main(int argc, char **argv) {
   int rc1, rc2;
   char *if_name;
   pthread_t tid1, tid2;

   // use specified CAN interface name, default is "can0"
   if_name = argc > 1 ? argv[1] : "can0";

   open_soc(if_name);

   // Run read_soc() as a thread, allowing it to alternate with update_file()
   // as needed.
   rc1 = pthread_create(&tid1, NULL, read_soc, &rc1);
   if (rc1) {
      printf("ERROR; return code from pthread_create() is %d\n", rc1);
      exit(-1);
   }

   // Run update_file() as a thread, allowing it to alternate with read_soc()
   // as needed.
   rc2 = pthread_create(&tid2, NULL, update_file, &rc2);
   if (rc2) {
      printf("ERROR; return code from pthread_create() is %d\n", rc2);
      exit(-1);
   }

   // exit properly so that worker threads continue to run
   pthread_exit(NULL);

   return 0;
}
