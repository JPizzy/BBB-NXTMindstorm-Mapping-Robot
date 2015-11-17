"use strict";
/*
 * Respond to commands over a websocket to relay UDP commands to a local program
 */

var fs = require('fs');
var socketio = require('socket.io');
var io;
var nxtDisconnectedErrorShown = false;
var dgram = require('dgram');

exports.listen = function(server) {
	io = socketio.listen(server);
	io.set('log level 1');
	io.sockets.on('connection', function(socket) {
		handleCommand(socket);
	});
};

function handleCommand(socket) {
	// Pased string of command to relay
	socket.on('nxt', function(data) {
		console.log('nxt udp command: ' + data);
		
		// Info for connecting to the local process via UDP
		var PORT = 12345;
		//var HOST = '192.168.7.2';
		var HOST = '127.0.0.1';
		var buffer = new Buffer(data);

		var client = dgram.createSocket('udp4');
		client.send(buffer, 0, buffer.length, PORT, HOST, function(err, bytes) {
		    if (err) 
		    	throw err;
		    console.log('UDP message sent to ' + HOST +':'+ PORT);
		});

		// Set a timer for nxt connection error
		var nxtDisconnectedTimer = setTimeout(function() {
			socket.emit('serverErr', "SERVER ERROR: No response from nxt application. Is it running?");
			nxtDisconnectedErrorShown = true;
		}, 1000);
		if(nxtDisconnectedErrorShown) {
			clearTimeout(nxtDisconnectedTimer);
		}

		client.on('listening', function () {
		    var address = client.address();
		    console.log('UDP Client: listening on ' + address.address + ":" + address.port);
		});
		
		// Handle an incoming message over the UDP from the local application.
		client.on('message', function (message, remote) {
			// Clear the nxt disconnected error timer since it is connected
			clearTimeout(nxtDisconnectedTimer);
			socket.emit('nxtOn');
			nxtDisconnectedErrorShown = false;

		    console.log("UDP Client: message Rx" + remote.address + ':' + remote.port +' - ' + message);
		    
		    var reply = message.toString('utf8')

		    var buffer = {
			buffName: data.toString('utf8'),
			buffMsg: message.toString('utf8')
			}

		    socket.emit('commandReply', buffer);
		    client.close();

		});
		
		client.on("UDP Client: close", function() {
		    console.log("closed");
		});
		client.on("UDP Client: error", function(err) {
		    console.log("error: ",err);
		});
	});

	socket.on('proc', function(fileName) {
		// NOTE: Very unsafe? Why?
		// Hint: think of ../
		var absPath = "/proc/" + fileName;
		console.log('accessing ' + absPath);
		
		fs.exists(absPath, function(exists) {
			if (exists) {
				// Can use 2nd param: 'utf8',
				fs.readFile(absPath, function(err, fileData) {
					if (err) {
						emitSocketData(socket, fileName, 
								"ERROR: Unable to read file " + absPath);
					} else {
						emitSocketData(socket, fileName, 
								fileData.toString('utf8'));
					}
				});
			} else {
				emitSocketData(socket, fileName, 
						"ERROR: File " + absPath + " not found.");
			}
		});
	});
};

function emitSocketData(socket, fileName, contents) {
	var result = {
			fileName: fileName,
			contents: contents
	}
	socket.emit('fileContents', result);	
}
