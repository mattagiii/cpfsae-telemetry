// acquire.h
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

// A channel includes one or more bytes representing a data value, along with
// the necessary "metadata" to specify where it appears within frames sent
// from a given CAN ID.
// offset:
//    number of bytes from the start of the first frame sent from the
//    channel's ID to the channel's data bytes (e.g. if ID 0x7FF sends frames
//    with data [00000000] and [0000XX00], the offset for the byte of interest
//    XX is 7)
// n_bytes:
//    number of bytes taken by this channel
// value:
//    the current value for this channel (it is not deemed worthwhile to vary
//    the length of the struct to accommodate arbitrarily large channel values,
//    as these are rare. channel values spanning more than a standard frame can
//    be treated as multiple channels at this level and joined/processed
//    elsewhere)
// scaling:
//    for non-integer or large, low-resolution data values, a multiplicative
//    scaling factor can be associated here
// precision:
//    the number of decimal places (after scaling) that this channel's data
//    value requires
// units:
//    a string representing the units for the channel's value (e.g. kPa, rad/s)
// name:
//    the channel's name (use CamelCase)
typedef struct channel {
   uint32_t offset;
   size_t n_bytes;
   int64_t value;
   float scaling;
   uint8_t precision;
   char units[8];
   char name[64];
} channel;
