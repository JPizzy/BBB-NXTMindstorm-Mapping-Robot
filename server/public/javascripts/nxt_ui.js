"use strict";
// Client-side interactions with the browser.

var serverNotConnectedErrorShown = false;
var mapData = 0;

var canvas;
var ctx;

// Make connection to server when web page is fully loaded.
var socket = io.connect();
$(document).ready(function() {

	canvas = document.getElementById('map-canvas');
	ctx = canvas.getContext('2d');

	// Continously check for map data and connection status
	window.setInterval(function() {
		sendCommand('getMapData');
		sendCommand('getPower');
		sendRequest('uptime');
		checkServerConnection(socket);
	}, 1000);
	
	// Set up buttons
	$('#beginMapping').click(function() {
		sendCommand("beginMapping");
	});
	$('#moveForward').click(function() {
		sendCommand("moveForward");
	});
	$('#moveBackward').click(function() {
		sendCommand("moveBackward");
	});
	$('#moveLeft').click(function() {
		sendCommand("moveLeft");
	});
	$('#moveRight').click(function() {
		sendCommand("moveRight");
	});
	
	socket.on('commandReply', function(result) {
		var buffMessage = result.buffMsg;
		console.log("Recieved reply\n");

		if(result.buffName == "getMapData") {
			mapData = buffMessage;
			var data = mapData.split(' ');
			//console.log("(" + data[1] + "," + data[2] + ")");
			mapDistance(data[1], data[2]);
		}
		
		if(result.buffName == "getPower") {
			$('#powerid').val(buffMessage);
		}
		
		});

	// Handle data coming back from the server
	socket.on('fileContents', function(result) {
		var fileName = result.fileName;
		var contents = result.contents;
	
		var timeArray = contents.split(" ");
		var timeInSeconds = parseInt(timeArray);
		
		var hours = parseInt((timeInSeconds / 60) / 60);
		var minutes = parseInt((timeInSeconds - (hours * 60 * 60)) / 60);
		var seconds = parseInt(timeInSeconds - (hours * 60 * 60) - (minutes * 60));
	
		$('#status').html("<p>Device up for:<br>" + hours + ":" + minutes + ":" + seconds + "(H:M:S)</p>");
	});

	socket.on("serverErr", function(errMsg) {
		$('#error-box').show();
		$('#error-text').text(errMsg);
		setErrorBoxHideTimer();
	});

	socket.on("nxtOn", function(errMsg) {
		$('#error-box').hide();
	});

});

function sendCommand(message) {
	socket.emit('nxt', message);
};

function sendRequest(file) {
	//console.log("Requesting '" + file + "'");
	socket.emit('proc', file);
}

function setErrorBoxHideTimer() {
	setTimeout(function() {
		$('#error-box').hide();
	}, 10000);
}

function checkServerConnection(socket) {
	if(socket.connected == false && serverNotConnectedErrorShown == false) {
		serverNotConnectedErrorShown = true;
		$('#error-box').show();
		$('#error-text').text("SERVER ERROR: No response from node.js server. Is it running?");
		setErrorBoxHideTimer();
	} 
	else if(socket.connected == true && serverNotConnectedErrorShown == true) {
		$('#error-box').hide();
		if(serverNotConnectedErrorShown == true) {
			serverNotConnectedErrorShown = false;
		}
	}
}

//////////////////////////////////////////////////////////////
// M A P P I N G
//////////////////////////////////////////////////////////////

function mapDistance(xVal, yVal) {
	var nxtX = canvas.width/2;
	var nxtY = canvas.height/2;
	var x = centimetersToPx(xVal) + nxtX;
	var y = (-1) * centimetersToPx(yVal) + nxtY;
	console.log("(" + x + "," + y + ")");
	drawDataPoint(x, y);
}

function centimetersToPx(centimeters) {
	var nxtMaxRange = 255;
	return ((canvas.height/2) * (centimeters/nxtMaxRange));
}

function drawDataPoint(x, y) {
	var pxSize = 4;
	var offset = pxSize/2;
	ctx.fillRect((x-offset), (y-offset), pxSize, pxSize);
}
