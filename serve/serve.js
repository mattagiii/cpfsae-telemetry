#!/usr/bin/env node
// the above shebang line allows serve.js to be executed directly (instead of
// the command "node serve.js")

// serve.js
// Monitors CAN data values stored by ../acquire/acquire and updates/serves a
// webpage to display the values.
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

// load modules
var http = require('http');
var fs = require('fs');
var path = require('path');
var fileAvailable = 0;
var telemChannelFile = '../acquire/telemChannels.json';

// initialize server on port 80
var server = http.createServer(function (req, res) {
   // process URL and set default content type and encoding
   var file = '.' + ((req.url == '/') ? '/index.html' : req.url);
   var fileExtension = path.extname(file);
   var contentType = 'text/html';
   var encoding = 'utf8';

   // allow custom CSS and PNG images
   if (fileExtension == '.css') {
      contentType = 'text/css';
   }
   else if (fileExtension == '.png') {
      contentType = 'image/png';
      encoding = 'binary';
   }

   // make sure the file exists
   if (fs.existsSync(file)) {
      fs.readFile(file, function(error, content) {
         if (!error) {
            // write content
            res.writeHead(200, {'content-type':contentType});
            res.end(content, encoding);
         }
      })
   }
   else{
      // URL doesn't exist, so respond with 404
      res.writeHead(404);
      res.end('Page not found');
   }
}).listen(80);

// load socket.io module
var io = require('socket.io').listen(server);

// monitor the channel JSON file and send its data on the socket when modified.
// this is only a function so that it can call itself again in the event of an
// error. this is the easiest way to wait if acquire has not created the JSON
// file yet. currently no check is performed to make sure that the file contains
// valid data; it is assumed that acquire will only send a properly formatted
// file
function watchChannelFile () {
   try {
      fs.watch(telemChannelFile, (eventType, filename) => {

         var telemChannelText = fs.readFileSync(telemChannelFile, 'utf8');

         if (telemChannelText)
            io.emit('telemChannelData', telemChannelText);

      });
   } catch (err) {
      console.log("Channel file unavailable. Trying again in 1s...")
      setTimeout(watchChannelFile, 1000);
   }
}

watchChannelFile();
