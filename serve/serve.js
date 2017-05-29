/* serve.js
 * Monitors CAN data values stored by ../acquire/acquire and updates/serves a
 * webpage to display the values.
 *
 * Created by: Matt Rounds
 * Last Updated: May 19, 2017
*/

/* Load Modules */
var http = require('http');
var fs = require('fs');
var path = require('path');

var M400_file = '../acquire/M400_channels.json';

/* Initialize Server on Port 80 */
var server = http.createServer(function (req, res) {
   // process URL and set default content type and encoding
   console.log(req.url);
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

// print messages to console when users connect
io.on('connection', function (socket) {

   console.log('A user connected');

   socket.on('disconnect', function () {
      console.log('A user disconnected');
   });
});

// monitor the channel JSON file and send its data on the socket when modified
fs.watch(M400_file, (eventType, filename) => {

   var M400_text = fs.readFileSync(M400_file, 'utf8');

   io.emit('M400_data', M400_text);

});
