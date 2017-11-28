// telemetryClient.js
// Main orchestration for the client side. Primarily this sets up the global
// JSON data structure and keeps it updated.
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

var telemChannelJSON;
var channelsSetUp = 0;

// creates the appropriate elements for displaying the raw channel values in a
// table view on the "All Channels" page
function setUpAllChannels() {
   // the row structure is pre-existing in the HTML
   var defaultRow = $("#defaultRow");
   var lastRow = defaultRow;
   var newRow;
   var numChannels = telemChannelJSON.channels.length;

   // add and configure a table row for each channel
   for (i = 0; i < numChannels; i++) {
      name = telemChannelJSON.channels[i].name;
      newRow = lastRow.clone();
      newRow.attr("id", name);
      newRow.find("th").html(i+1);
      newRow.find(".chan_name").html(name);
      newRow.insertAfter(lastRow);
      lastRow = newRow;
   }
   defaultRow.remove();
}

// run this function when the page is loaded
$(function () {
   var socket = io(); // set up socket client
   var name; // current channel name
   var val; // current channel value
   var numChannels;

   // receive data from server
   socket.on('telemChannelData', function(telemChannelText) {
      try {
         telemChannelJSON = JSON.parse(telemChannelText); // parse text as JSON
         if (!channelsSetUp) {
            setUpAllChannels();
            channelsSetUp = 1;
         }
         var numChannels = telemChannelJSON.channels.length;

         // update the HTML with values from the JSON data. for simplicity this
         // is done every time new data arrives, but all other data-dependent
         // UI updates should be scheduled elsewhere with appropriate refresh
         // rates instead of being driven by the arrival of socket data
         for (i = 0; i < numChannels; i++) {
            name = telemChannelJSON.channels[i].name;
            val = telemChannelJSON.channels[i].value;
            $("#" + name + " .chan_val").html(val);
         }
      } catch(e) {
         console.log(e);
      }
   });
});
