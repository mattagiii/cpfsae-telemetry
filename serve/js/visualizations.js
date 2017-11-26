// visualizations.js
// Manages the data visualizations on the "Main" page of the interface.
// dygraph.js is used for line charts. Also handles UI interactions related to
// the data visualizations.
//
// Created by: Matt Rounds
// Last updated: Nov 19, 2017

var dygraphs = {};
var updateRate = 100;

// change both the HTML and the actual input value of the pause button
function handlePauseBtn(b) {
   $(b).html(b.value);
   b.value = b.value === "Pause" ? "Unpause" : "Pause";
}

// if the autoscale button is pressed, change the manual limits
function handleAutoscaleBtn(i) {
   var graphDiv = $(i).siblings("[data-vis]");
   $(graphDiv).siblings(".uBound").val();
   var g = dygraphs[$(graphDiv).attr("id")];
   var lBound = g.yAxisExtremes()[0][0];
   var uBound = g.yAxisExtremes()[0][1];
   g.updateOptions({"valueRange":[lBound,uBound]});
   $(graphDiv).siblings(".lBound").val(lBound);
   $(graphDiv).siblings(".uBound").val(uBound);
}

// if one of the bounds is changed, compute a new dygraph range and set it
function handleScaling(i) {
   var graphDiv = $(i).siblings("[data-vis]");
   var range = [$(graphDiv).siblings(".lBound").val(), $(graphDiv).siblings(".uBound").val()];
   var g = dygraphs[$(graphDiv).attr("id")];
   g.updateOptions({"valueRange":range});
}

// configures a dygraph for the given element.
function setUpLineChart(e, i, a) {
   var channelID = parseInt($(e).attr("data-channel")) - 1;
   var data = [];
   var o = $(e).data("opt");
   var g = new Dygraph(e, data, o);
   dygraphs[$(e).attr("id")] = g;
   var range = g.getOption("valueRange");
   // this prevents the graph from autoscaling on zoom out (see dygraph docs)
   // the bounds are also reset to stay in sync with the graph
   g.updateOptions( { 'zoomCallback': function() {
      g.updateOptions({"valueRange":range});
      $(e).siblings(".lBound").val(range[0]);
      $(e).siblings(".uBound").val(range[1]);
   } } );
   $(e).siblings(".lBound").val(range[0]);
   $(e).siblings(".uBound").val(range[1]);

   // iid = interval ID. stored with the dygraph element in case it needs to be
   // cleared later
   $(e).attr("iid", setInterval(function() {
      // data is left alone if the graph is paused. currently this means that
      // any valued received while paused will not be viewable. this could be
      // improved by displaying a temporary frozen version of the data while
      // still updating the real data. channelsSetUp is also checked here so
      // that graphs do not try to record data when telemChannelJSON hasn't
      // been populated yet
      if ($(e).siblings(".pauseBtn").val() === "Unpause" && channelsSetUp) {
         var x = new Date();  // current time
         var y = parseFloat(telemChannelJSON.channels[channelID].value);
         var tLim = x.getTime() - parseFloat($(e).siblings(".tLim").val()) * 1000;
         data.push([x, y]);

         // chop off the earlier data points based on the amount of time the
         // user has selected
         while (data[0][0].getTime() < tLim)
            data.shift();

         g.updateOptions( { 'file': data } );
      }
   }, updateRate));
}

$(function () {
   // grab the proper elements
   var lineDvs = $("[data-vis='linechart']");
   // make sure all number inputs have a step size of 5 (easier to adjust)
   $("input[type='number']").not("[step]").attr("step",5);

   // run the above function to actually prepare the graphs
   lineDvs.toArray().forEach(setUpLineChart);

   // different data visualization setups can go here.
});
