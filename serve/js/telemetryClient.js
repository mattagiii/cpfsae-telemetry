// run this function when the page is loaded
$(function () {
   var socket = io(); // set up socket client
   var name; // current channel name
   var val; // current channel value
   var numChannels;

   // receive data from server
   socket.on('M400_data', function(M400_text) {
      console.log(M400_text);
      var M400_json = JSON.parse(M400_text); // parse text as JSON
      var numChannels = M400_json.channels.length;

      // update the HTML with values from the JSON data
      for (i = 0; i < numChannels; i++) {
         name = M400_json.channels[i].name;
         val = M400_json.channels[i].value;

         $("#" + name + " .chan_name").html(name);
         $("#" + name + " .chan_val").html(val);
      }
   });
});
