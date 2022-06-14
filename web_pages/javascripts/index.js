$(document).ready(function () {
  var timeData = [],
    temperatureData = [],
    heartrateData = [];
  var heartbeatData = [];
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
        label: 'Heart Rate',
        yAxisID: 'Heart Rate',
        borderColor: "rgba(24, 120, 240, 1)",
        pointBoarderColor: "rgba(24, 120, 240, 1)",
        backgroundColor: "rgba(24, 120, 240, 0.4)",
        pointHoverBackgroundColor: "rgba(24, 120, 240, 1)",
        pointHoverBorderColor: "rgba(24, 120, 240, 1)",
        data: heartrateData
      }
    ]
  }
  
    var dataset= {
    labels: index,
    datasets: [{ 
        data: heartbeatData,
        label: "heart beats",
		borderColor: "rgba(24, 120, 240, 1)",
        pointBoarderColor: "rgba(24, 120, 240, 1)",
        backgroundColor: "rgba(24, 120, 240, 0.4)",
        pointHoverBackgroundColor: "rgba(24, 120, 240, 1)",
        pointHoverBorderColor: "rgba(24, 120, 240, 1)",
		pointRadius: 0,
        fill: false
      }
    ]
  }


  var basicOption = {
    title: {
      display: true,
      text: 'Heart Rate & Temperature Real-time Data',
      fontSize: 36
    },
    scales: {
      yAxes: [{
        id: 'Temperature',
        type: 'linear',
		ticks: {
                max: 50,
                min: 10,
                stepSize: 10
            },
        scaleLabel: {
          labelString: 'Temperature(C)',
          display: true
        },
        position: 'right',
      }, {
          id: 'Heart Rate',
          type: 'linear',
		  ticks: {
                max: 160,
                min: 0,
                stepSize: 20
            },
          scaleLabel: {
            labelString: 'Heart Rate (PPM)',
            display: true
          },
          position: 'left'
        }]
    }
  }

  var basicOptionHeartBeats = {
    title: {
      display: true,
      text: 'Heart beats Real-time Data',
      fontSize: 36
    },
	scales: {
      yAxes: [{
        id: 'beats',
        type: 'linear',
		ticks: {
                max: 500,
                min: -500,
                stepSize: 100
            }
      }]
	}
  }
  //Get the context of the canvas element we want to select
  var ctx = document.getElementById("myChart").getContext("2d");
  var ctx2 = document.getElementById("myChart2").getContext("2d");
  
  var optionsNoAnimation = { animation: false }
  var myLineChart = new Chart(ctx, {
    type: 'line',
    data: data,
    options: basicOption
  });
  var myLineChart2 = new Chart(ctx2, {
    type: 'line',
    data: dataset,
    options: basicOptionHeartBeats
  });
  
  //var ws = new WebSocket('wss://' + location.host);
  var ws = new WebSocket('ws://192.168.1.1:8000');
  ws.onopen = function () {
    console.log('Successfully connect WebSocket');
  }
  ws.onmessage = function (message) {
    console.log('receive message' + message.data);
    try {
      var obj = JSON.parse(message.data);
      if(!obj.time || !obj.temperature || !obj.id ) {
        return;
      }
      if(obj.id == "1")
      {
        timeData.push(obj.time);
        temperatureData.push(obj.temperature);
        // only keep no more than 50 points in the line chart
        const maxLen = 30;
        var len = timeData.length;
        if (len > maxLen) {
          timeData.shift();
          temperatureData.shift();
        }

        if (obj.heartRate || (obj.heartRate == 0)) {
          heartrateData.push(obj.heartRate);
        }
        if (heartrateData.length > maxLen) {
          heartrateData.shift();
        }

    // only keep no more than 50 points in the line chart
        const beatmaxLen = 1000;

        if (obj.beats) {
          for (property in obj.beats) {
            index.push(counter);
            heartbeatData.push(obj.beats[property]);
            counter = counter + 1;
          }
        }
        if (heartbeatData.length > beatmaxLen) {
          for (property in obj.beats) 
          {
            heartbeatData.shift();
            index.shift();
          }
        }

        myLineChart.update();
      
        myLineChart2.update();
      }
    } catch (err) {
      console.error(err);
    }
  }
});
