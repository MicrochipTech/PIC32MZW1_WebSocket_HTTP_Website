$(document).ready(function () {
  var timeData = [],
    temperatureData = [],
    lightIntensity = [];
  var index = [];
  var counter = 0; 
  
  var data = {
    labels: timeData,
    datasets: [
      {
        fill: false,
        label: 'Temperature',
        yAxisID: 'Temperature',
        borderColor: "rgba(255, 204, 0, 1)",
        pointBoarderColor: "rgba(255, 204, 0, 1)",
        backgroundColor: "rgba(255, 204, 0, 0.4)",
        pointHoverBackgroundColor: "rgba(255, 204, 0, 1)",
        pointHoverBorderColor: "rgba(255, 204, 0, 1)",
        data: temperatureData
      },
      {
        fill: false,
        label: 'Light Intensity',
        yAxisID: 'Light Intensity',
        borderColor: "rgba(0, 128, 0, 1)",
        pointBoarderColor: "rgba(0, 128, 0, 1)",
        backgroundColor: "rgba(0, 128, 0, 0.4)",
        pointHoverBackgroundColor: "rgba(0, 128, 0, 1)",
        pointHoverBorderColor: "rgba(0, 128, 0, 1)",
        data: lightIntensity
      }
    ]
  }
  



  var basicOption = {
    title: {
      display: true,
      text: 'Temperature & Light Intensity Real-time Data',
      fontSize: 36
    },
    scales: {
      yAxes: [{
        id: 'Temperature',
        type: 'linear',
		ticks: {
                max: 60,
                min: 10,
                stepSize: 10
            },
        scaleLabel: {
          labelString: 'Temperature(C)',
          display: true
        },
        position: 'left',
      }, {
          id: 'Light Intensity',
          type: 'linear',
		  ticks: {
                max: 600,
                min: 0,
                stepSize: 20
            },
          scaleLabel: {
            labelString: 'Light Intensity (lux)',
            display: true
          },
          position: 'right'
        }]
    }
  }

  //Get the context of the canvas element we want to select
  var ctx = document.getElementById("temperatureChart").getContext("2d");
  
  var optionsNoAnimation = { animation: false }
  var myLineChart = new Chart(ctx, {
    type: 'line',
    data: data,
    options: basicOption
  });

  console.log('location.hostname ' + location.hostname);
  
  var ws = new WebSocket('ws://' + location.hostname + ':8000');
  

  ws.onopen = function () {
    console.log('Successfully connect WebSocket');
  }
  ws.onmessage = function (message) {
    console.log('receive message' + message.data);
    try {
      var obj = JSON.parse(message.data);
      if(!obj.time || !obj.temperature || !obj.lightIntensity ) {
        return;
      }

	  if (obj.temperature > 60)
	    obj.temperature = 60;
		
	  timeData.push(obj.time);
	  temperatureData.push(obj.temperature);
	  // only keep no more than 50 points in the line chart
	  const maxLen = 30;
	  var len = timeData.length;
	  if (len > maxLen) {
		timeData.shift();
		temperatureData.shift();
	  }
		
	  if (obj.lightIntensity > 600)
	    obj.lightIntensity = 600;
	  if (obj.lightIntensity || (obj.lightIntensity == 0)) {
	    lightIntensity.push(obj.lightIntensity);
	  }
	  if (lightIntensity.length > maxLen) {
	    lightIntensity.shift();
	  }

	  myLineChart.update();

      
    } catch (err) {
      console.error(err);
    }
  }
});
